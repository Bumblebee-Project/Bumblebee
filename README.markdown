Bumblebee Daemon
=================

Bumblebee daemon is a rewrite of the original
[Bumblebee](https://github.com/Bumblebee-Project/Bumblebee)
service, providing an elegant and stable means of managing Optimus
hybrid graphics chipsets. A primary goal of this project is to not only
enable use of the discrete GPU for rendering, but also to enable
smart power management of the dGPU when it's not in use.

Build Requirements
-------------------

- pkg-config
- autotools (2.68+ recommended)
- glib-2.0 and development headers
- libx11 and development headers
- libbsd and development headers (if pidfile support is enabled, default yes)

Runtime dependencies
--------------------

If you want to use `optirun` for running applications with the discrete nVidia
card, you will also need:

- [virtualgl](http://virtualgl.org/)
- Driver for nvidia graphics card: [nouveau](http://nouveau.freedesktop.org/)
  or the proprietary nvidia driver. Don't install it directly from nvidia.com
  as it will break 3D capabilities on the Intel graphics card and therefore
  affect the display of frames from the nvidia card.

If you want to make use of Power Management, you will need:

- [bbswitch](https://github.com/Bumblebee-Project/bbswitch)
- If you're brave and want to try the `switcheroo` method, install at least the
  [optimus patch](http://lekensteyn.nl/files/nouveau-switcheroo-optimus.patch).
  Note that suspend is not yet supported by this method.

Building
---------

    autoreconf -fi
    ./configure
    make

Installing System-wide and Packaging
-------------------------------------

You can build the binaries and set the system wide paths at configure time

    autoreconf -fi
    ./configure --prefix=/usr --sysconfdir=/etc
    make

To set the default driver to `nvidia` and adjust the library and module paths
for it, use `./configure` like:

    ./configure CONF_DRIVER=nvidia CONF_DRIVER_MODULE_NVIDIA=nvidia-current \
      CONF_LDPATH_NVIDIA=/usr/lib/nvidia-current:/usr/lib32/nvidia-current \
      CONF_MODPATH_NVIDIA=/usr/lib/nvidia-current/xorg,/usr/lib/xorg/modules \
      --prefix=/usr --sysconfdir=/etc

For all available options, run:

    ./configure --help

After building the binaries and bash completion script, it can be installed
using `make`:

    sudo make install

For packagers you need to add DESTDIR=$pkgdir

    make install DESTDIR=$pkgdir

Example initscripts are available in the `scripts/` directory. Currently,
Upstart, SystemD and SysV initscripts are available

Usage
------

The first time you install Bumblebee, the `bumblebee` group has to be created.
Users who are allowed to use Bumblebee need to be added to the group:

    sudo groupadd bumblebee
    sudo usermod -a -G bumblebee $USER

To test Bumblebee before installing it system-wide, run:

    sudo bin/bumblebeed --daemon
    bin/optirun -- <application>

For more information, try `--help` on either of the two binaries.
