/**
 * Disable discrete graphics (currently nvidia only)
 *
 * Based on:
 * byo-switcheroo   Alex Williamson <alex.williamson@redhat.com>
 * acpi_call        Michal Kottman
 */
#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/module.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Toggle the discrete graphics card");

extern struct proc_dir_entry *acpi_root_dir;

static const char acpi_optimus_dsm_muid[] = {
    0xF8, 0xD8, 0x86, 0xA4, 0xDA, 0x0B, 0x1B, 0x47,
    0xA7, 0x2B, 0x60, 0x42, 0xA6, 0xB5, 0xBE, 0xE0,
};

static struct pci_dev *dis_dev;
static acpi_handle dis_handle;
static int dis_enabled = 1;

/* shamelessly taken from nouveau_acpi.c */
static int acpi_optimus_dsm(acpi_handle handle, int func, char *args,
    uint32_t *result) {
    struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
    struct acpi_object_list input;
    union acpi_object params[4];
    union acpi_object *obj;
    int err;

    input.count = 4;
    input.pointer = params;
    params[0].type = ACPI_TYPE_BUFFER;
    params[0].buffer.length = sizeof(acpi_optimus_dsm_muid);
    params[0].buffer.pointer = (char *)acpi_optimus_dsm_muid;
    params[1].type = ACPI_TYPE_INTEGER;
    params[1].integer.value = 0x00000100;
    params[2].type = ACPI_TYPE_INTEGER;
    params[2].integer.value = func;
    params[3].type = ACPI_TYPE_BUFFER;
    params[3].buffer.length = sizeof(args);
    params[3].buffer.pointer = args;

    err = acpi_evaluate_object(handle, "_DSM", &input, &output);
    if (err) {
        printk(KERN_WARNING "bbswitch: failed to evaluate _DSM: %s\n",
            acpi_format_exception(err));
        return err;
    }

    obj = (union acpi_object *)output.pointer;

    if (obj->type == ACPI_TYPE_INTEGER)
        // REVS (revision number) not found, possibly not an Optimus?
        if (obj->integer.value == 0x80000002) {
            printk(KERN_INFO "bbswitch: Optimus function not found\n");
            return -ENODEV;
        }

    if (obj->type == ACPI_TYPE_BUFFER) {
        if (obj->buffer.length == 4 && result) {
            *result = 0;
            *result |= obj->buffer.pointer[0];
            *result |= (obj->buffer.pointer[1] << 8);
            *result |= (obj->buffer.pointer[2] << 16);
            *result |= (obj->buffer.pointer[3] << 24);
        }
    }

    kfree(output.pointer);
    return 0;
}

static void bbswitch_acpi_off(void) {
    char args[] = {1, 0, 0, 3};
    u32 result = 0;

    if (!acpi_optimus_dsm(dis_handle, 0x1A, args, &result)) {
        printk(KERN_INFO "bbswitch: Result of _DSM call: %08X\n", result);
    }
}
static void bbswitch_acpi_on(void) {
    
}

static void bbswitch_off(void) {
    if (!dis_enabled)
        return;

    printk(KERN_INFO "bbswitch: disabling discrete graphics\n");

    pci_save_state(dis_dev);
    pci_clear_master(dis_dev);
    pci_disable_device(dis_dev);
    pci_set_power_state(dis_dev, PCI_D3hot);

    bbswitch_acpi_off();

    dis_enabled = 0;
}

static void bbswitch_on(void) {
    if (dis_enabled)
        return;

    printk(KERN_INFO "bbswitch: enabling discrete graphics\n");

    bbswitch_acpi_on();

    pci_set_power_state(dis_dev, PCI_D0);
    pci_restore_state(dis_dev);
    if (pci_enable_device(dis_dev))
        printk(KERN_WARNING "bbswitch: failed to enable %s\n",
            dev_name(&dis_dev->dev));
    pci_set_master(dis_dev);

    dis_enabled = 1;
}

static int bbswitch_write(struct file *filp, const char __user *buff,
    unsigned long len, void *data) {
    char cmd[8];

    if (len >= sizeof(cmd)) {
        printk(KERN_ERR "bbswitch: Input too large (%lu)\n", len);
        return -ENOSPC;
    }

    if (copy_from_user(cmd, buff, len))
        return -EFAULT;

    if (strncmp(cmd, "OFF", 3) == 0)
        bbswitch_off();

    if (strncmp(cmd, "ON", 2) == 0)
        bbswitch_on();

    return len;
}

static int bbswitch_read(char *page, char **start, off_t off,
    int count, int *eof, void *data) {
    int len = 0;
    return len;
}

static int __init bbswitch_init(void) {
    struct proc_dir_entry *acpi_entry;
    struct pci_dev *pdev = NULL;
    int class = PCI_CLASS_DISPLAY_VGA << 8;

    while ((pdev = pci_get_class(class, pdev)) != NULL) {
        struct acpi_buffer buf = { ACPI_ALLOCATE_BUFFER, NULL };
        acpi_handle handle;

        handle = DEVICE_ACPI_HANDLE(&pdev->dev);
        if (!handle)
            continue;

        if (pdev->vendor != PCI_VENDOR_ID_INTEL) {
            dis_dev = pdev;
            dis_handle = handle;
            acpi_get_name(handle, ACPI_FULL_PATHNAME, &buf);
            printk(KERN_INFO "bbswitch: Found discrete VGA device %s: %s\n",
                dev_name(&pdev->dev), (char *)buf.pointer);
        }
        kfree(buf.pointer);
    }

    if (dis_dev == NULL) {
        printk(KERN_ERR "bbswitch: No discrete VGA device found\n");
        return -ENODEV;
    }

    acpi_entry = create_proc_entry("bbswitch", 0660, acpi_root_dir);
    if (acpi_entry == NULL) {
        printk(KERN_ERR "bbswitch: Couldn't create proc entry\n");
        return -ENOMEM;
    }

    acpi_entry->write_proc = bbswitch_write;
    acpi_entry->read_proc = bbswitch_read;

    return 0;
}

static void __exit bbswitch_exit(void) {
    remove_proc_entry("bbswitch", acpi_root_dir);
}

module_init(bbswitch_init);
module_exit(bbswitch_exit);
