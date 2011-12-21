/**
 * Disable discrete graphics (currently nvidia only)
 *
 * Usage:
 * Disable discrete card
 * # echo OFF > /proc/acpi/bbswitch
 * Enable discrete card
 * # echo ON > /proc/acpi/bbswitch
 * Get status
 * # cat /proc/acpi/bbswitch
 */
#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/suspend.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Toggle the discrete graphics card");
MODULE_AUTHOR("Peter Lekensteyn <lekensteyn@gmail.com>");
MODULE_VERSION("0.1");

extern struct proc_dir_entry *acpi_root_dir;

static const char acpi_optimus_dsm_muid[16] = {
    0xF8, 0xD8, 0x86, 0xA4, 0xDA, 0x0B, 0x1B, 0x47,
    0xA7, 0x2B, 0x60, 0x42, 0xA6, 0xB5, 0xBE, 0xE0,
};

static const char acpi_nvidia_dsm_muid[16] = {
    0xA0, 0xA0, 0x95, 0x9D, 0x60, 0x00, 0x48, 0x4D,
    0xB3, 0x4D, 0x7E, 0x5F, 0xEA, 0x12, 0x9F, 0xD4
};

/*
The next UUID has been found as well in
https://bugs.launchpad.net/lpbugreporter/+bug/752542:

0xD3, 0x73, 0xD8, 0x7E, 0xD0, 0xC2, 0x4F, 0x4E,
0xA8, 0x54, 0x0F, 0x13, 0x17, 0xB0, 0x1C, 0x2C 
It looks like something for Intel GPU:
http://lxr.linux.no/#linux+v3.1.5/drivers/gpu/drm/i915/intel_acpi.c
 */

#define DSM_TYPE_UNSUPPORTED    0
#define DSM_TYPE_OPTIMUS        1
#define DSM_TYPE_NVIDIA         2
static int dsm_type = DSM_TYPE_UNSUPPORTED;

static struct pci_dev *dis_dev;
static acpi_handle dis_handle;

/* used for keeping the PM event handler */
static struct notifier_block nb;
/* whether the card was off before suspend or not; on: 0, off: 1 */
int dis_before_suspend_disabled;

static char *buffer_to_string(const char buffer[], char *target) {
    int i;
    for (i=0; i<sizeof(buffer); i++) {
        sprintf(target + i * 5, "%02X,", buffer[i]);
    }
    target[sizeof(buffer) * 5] = '\0';
    return target;
}

// Returns 0 if the call succeeded and non-zero otherwise. If the call
// succeeded, the result is stored in "result" providing that the result is an
// integer or a buffer containing 4 values
static int acpi_call_dsm(acpi_handle handle, const char muid[16], int revid,
    int func, char args[4], uint32_t *result) {
    struct acpi_buffer output = { ACPI_ALLOCATE_BUFFER, NULL };
    struct acpi_object_list input;
    union acpi_object params[4];
    union acpi_object *obj;
    int err;

    input.count = 4;
    input.pointer = params;
    params[0].type = ACPI_TYPE_BUFFER;
    params[0].buffer.length = 16;
    params[0].buffer.pointer = (char *)muid;
    params[1].type = ACPI_TYPE_INTEGER;
    params[1].integer.value = revid;
    params[2].type = ACPI_TYPE_INTEGER;
    params[2].integer.value = func;
    params[3].type = ACPI_TYPE_BUFFER;
    params[3].buffer.length = 4;
    if (args) {
        params[3].buffer.pointer = args;
    } else {
        // Some implementations (Asus U36SD) seem to check the args before the
        // function ID and crash if it is not a buffer.
        params[3].buffer.pointer = (char[4]){0, 0, 0, 0};
    }

    err = acpi_evaluate_object(handle, "_DSM", &input, &output);
    if (err) {
        char tmp[5 * max(sizeof(muid), sizeof(args))];

        printk(KERN_WARNING "bbswitch: failed to evaluate _DSM {%s} %X %X"
            " {%s}: %s\n",
            buffer_to_string(muid, tmp), revid, func,
            buffer_to_string(args, tmp), acpi_format_exception(err));
        return err;
    }

    obj = (union acpi_object *)output.pointer;

    if (obj->type == ACPI_TYPE_INTEGER && result) {
        *result = obj->integer.value;
    } else if (obj->type == ACPI_TYPE_BUFFER) {
        if (obj->buffer.length == 4 && result) {
            *result = 0;
            *result |= obj->buffer.pointer[0];
            *result |= (obj->buffer.pointer[1] << 8);
            *result |= (obj->buffer.pointer[2] << 16);
            *result |= (obj->buffer.pointer[3] << 24);
        }
    } else {
        printk(KERN_WARNING "bbswitch: _DSM call yields an unsupported result"
            " type: %X\n", obj->type);
    }

    kfree(output.pointer);
    return 0;
}

// Returns 1 if a _DSM function and its function index exists and 0 otherwise
static int has_dsm_func(const char muid[16], int revid, int sfnc) {
    u32 result = 0;

    // fail if the _DSM call failed
    if (acpi_call_dsm(dis_handle, muid, revid, 0, 0, &result))
        return 0;

    // ACPI Spec v4 9.14.1: if bit 0 is zero, no function is supported. If
    // the n-th bit is enabled, function n is supported
    return result & 1 && result & (1 << sfnc);
}

static int bbswitch_optimus_dsm(void) {
    char args[] = {1, 0, 0, 3};
    u32 result = 0;

    if (!acpi_call_dsm(dis_handle, acpi_optimus_dsm_muid, 0x100, 0x1A, args,
        &result)) {
        printk(KERN_INFO "bbswitch: Result of Optimus _DSM call: %08X\n",
            result);
        return 0;
    }
    // failure
    return 1;
}

static int bbswitch_acpi_off(void) {
    if (dsm_type == DSM_TYPE_NVIDIA) {
        char args[] = {2, 0, 0, 0};
        u32 result = 0;

        if (!acpi_call_dsm(dis_handle, acpi_nvidia_dsm_muid, 0x102, 0x3, args,
            &result)) {
            // failure
            return 1;
        }
        printk(KERN_INFO "bbswitch: Result of _DSM call for OFF: %08X\n",
            result);
    }
    return 0;
}

static int bbswitch_acpi_on(void) {
    if (dsm_type == DSM_TYPE_NVIDIA) {
        char args[] = {1, 0, 0, 0};
        u32 result = 0;

        if (!acpi_call_dsm(dis_handle, acpi_nvidia_dsm_muid, 0x102, 0x3, args,
            &result)) {
            // failure
            return 1;
        }
        printk(KERN_INFO "bbswitch: Result of _DSM call for ON: %08X\n",
            result);
    }
    return 0;
}

// Returns 1 if the card is disabled, 0 if enabled
static int is_card_disabled(void) {
    u32 cfg_word;
    // read first config word which contains Vendor and Device ID. If all bits
    // are enabled, the device is assumed to be off
    pci_read_config_dword(dis_dev, 0, &cfg_word);
    // if one of the bits is not enabled (the card is enabled), the inverted
    // result will be non-zero and hence logical not will make it 0 ("false")
    return !~cfg_word;
}

static void bbswitch_off(void) {
    if (is_card_disabled())
        return;

    // to prevent the system from possibly locking up, don't disable the device
    // if it's still in use by a driver (i.e. nouveau or nvidia)
    if (dis_dev->driver) {
        printk(KERN_WARNING "bbswitch: device %s is in use by driver '%s', "
            "refusing OFF\n", dev_name(&dis_dev->dev), dis_dev->driver->name);
        return;
    }

    printk(KERN_INFO "bbswitch: disabling discrete graphics\n");

    if (dsm_type == DSM_TYPE_OPTIMUS && bbswitch_optimus_dsm()) {
        printk(KERN_WARNING "bbswitch: ACPI call failed, the device is not"
            " disabled\n");
        return;
    }

    pci_save_state(dis_dev);
    pci_clear_master(dis_dev);
    pci_disable_device(dis_dev);
    pci_set_power_state(dis_dev, PCI_D3hot);

    if (bbswitch_acpi_off())
        printk(KERN_WARNING "bbswitch: The discrete card could not be disabled"
                " by a _DSM call\n");
}

static void bbswitch_on(void) {
    if (!is_card_disabled())
        return;

    printk(KERN_INFO "bbswitch: enabling discrete graphics\n");

    if (bbswitch_acpi_on())
        printk(KERN_WARNING "bbswitch: The discrete card could not be enabled"
            " by a _DSM call\n");

    pci_set_power_state(dis_dev, PCI_D0);
    pci_restore_state(dis_dev);
    if (pci_enable_device(dis_dev))
        printk(KERN_WARNING "bbswitch: failed to enable %s\n",
            dev_name(&dis_dev->dev));
    pci_set_master(dis_dev);
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
    // show the card state. Example output: 0000:01:00:00 ON
    return snprintf(page, count, "%s %s\n", dev_name(&dis_dev->dev),
             is_card_disabled() ? "OFF" : "ON");
}

static int bbswitch_pm_handler(struct notifier_block *nbp,
    unsigned long event_type, void *p) {
    switch (event_type) {
    case PM_HIBERNATION_PREPARE:
    case PM_SUSPEND_PREPARE:
        dis_before_suspend_disabled = is_card_disabled();
        // enable the device before suspend to avoid the PCI config space from
        // being saved incorrectly
        if (dis_before_suspend_disabled)
            bbswitch_on();
        break;
    case PM_POST_HIBERNATION:
    case PM_POST_SUSPEND:
    case PM_POST_RESTORE:
        // after suspend, the card is on, but if it was off before suspend,
        // disable it again
        if (dis_before_suspend_disabled)
            bbswitch_off();
        break;
    case PM_RESTORE_PREPARE:
        // deliberately don't do anything as it does not occur before suspend
        // nor hibernate, but before restoring a saved image. In that case,
        // either PM_POST_HIBERNATION or PM_POST_RESTORE will be called
        break;
    }
    return 0;
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

    if (has_dsm_func(acpi_optimus_dsm_muid, 0x100, 0x1A)) {
        dsm_type = DSM_TYPE_OPTIMUS;
        printk(KERN_INFO "bbswitch: detected an Optimus _DSM function\n");
    } else if (has_dsm_func(acpi_nvidia_dsm_muid, 0x102, 0x3)) {
        dsm_type = DSM_TYPE_NVIDIA;
        printk(KERN_INFO "bbswitch: detected a nVidia _DSM function\n");
    } else {
        printk(KERN_ERR "bbswitch: No suitable _DSM call found.\n");
        return -ENODEV;
    }

    acpi_entry = create_proc_entry("bbswitch", 0660, acpi_root_dir);
    if (acpi_entry == NULL) {
        printk(KERN_ERR "bbswitch: Couldn't create proc entry\n");
        return -ENOMEM;
    }

    printk(KERN_INFO "bbswitch: Succesfully loaded. Discrete card %s is %s\n",
        dev_name(&dis_dev->dev), is_card_disabled() ? "off" : "on");

    acpi_entry->write_proc = bbswitch_write;
    acpi_entry->read_proc = bbswitch_read;

    nb.notifier_call = &bbswitch_pm_handler;
    register_pm_notifier(&nb);

    return 0;
}

static void __exit bbswitch_exit(void) {
    remove_proc_entry("bbswitch", acpi_root_dir);

    if (nb.notifier_call)
        unregister_pm_notifier(&nb);
}

module_init(bbswitch_init);
module_exit(bbswitch_exit);
