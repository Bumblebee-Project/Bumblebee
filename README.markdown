Bumblebee Daemon
=================

Bumblebee daemon is a rewrite of the original
[Bumblebee](https://github.com/Bumblebee-Project/Bumblebee-old)
service, providing an elegant and stable means of managing Optimus
hybrid graphics chipsets. A primary goal of this project is to not only
enable use of the discrete GPU for rendering, but also to enable
smart power management of the dGPU when it's not in use.

Build Requirements
-------------------

Source tarballs can be downloaded from
https://github.com/Bumblebee-Project/Bumblebee/downloads

The following packages are dependencies for the build process:

- pkg-config
- glib-2.0 and development headers
- libx11 and development headers
- libbsd and development headers (if pidfile support is enabled, default yes)
- help2man (optional, it is needed for building manual pages)

If you are building from git, you will also need:

- autotools (2.68+ recommended)

Runtime dependencies
--------------------

If you want to use `optirun` for running applications with the discrete nVidia
card, you will also need:

- At least one back-end for `optirun`:
  - [virtualgl](http://virtualgl.org/)
  - [primus](https://github.com/amonakov/primus) (beta)
- Driver for nvidia graphics card: [nouveau](http://nouveau.freedesktop.org/)
  or the proprietary nvidia driver. Don't install it directly from nvidia.com
  as it will break 3D capabilities on the Intel graphics card and therefore
  affect the display of frames from the nvidia card.

If you want to make use of Power Management, you will need:

- [bbswitch](https://github.com/Bumblebee-Project/bbswitch)
- If you're brave and want to try the `switcheroo` method, install at least the
  [optimus patch](http://lekensteyn.nl/files/nouveau-switcheroo-optimus.patch)
  (merged in Linux 3.3). Note that suspend is not yet supported by this
  method.

Building
---------

If you are building from git, you first need to run `autoreconf -fi` to generate
the `configure` script.

Next, run the configure script to check for dependencies and populate the
`Makefile`:

    ./configure

To set the default driver to `nvidia` and adjust the library and module paths
for it, use `./configure` like:

    ./configure CONF_DRIVER=nvidia CONF_DRIVER_MODULE_NVIDIA=nvidia-current \
      CONF_LDPATH_NVIDIA=/usr/lib/nvidia-current:/usr/lib32/nvidia-current \
      CONF_MODPATH_NVIDIA=/usr/lib/nvidia-current/xorg,/usr/lib/xorg/modules

For all available options, run:

    ./configure --help

After configuring, you can build the binaries with:

    make

Installing System-wide and Packaging
-------------------------------------

You can build the binaries and set the system wide paths at configure time

    ./configure --prefix=/usr --sysconfdir=/etc
    make

After building the binaries and bash completion script, it can be installed
together with an udev rule (unless `--without-udev-rules` was passed) using
`make`:

    sudo make install

For packagers you need to add DESTDIR=$pkgdir

    make install DESTDIR=$pkgdir

Example initscripts are available in the `scripts/` directory. Currently,
Upstart, systemd and Sys V initscripts are available.

Usage
------

The first time you install Bumblebee, the `bumblebee` group has to be created.
Users who are allowed to use Bumblebee need to be added to the group:

    sudo groupadd bumblebee
    sudo gpasswd -a $USER bumblebee

To run Bumblebee after installing it system-wide, run:

    sudo bumblebeed --daemon
    optirun -- <application>

For more information, try `--help` on either of the two binaries.

Interesting Links
------
- Facebook: http://www.bumblebee-project.org/facebook
- Twitter: http://www.bumblebee-project.org/twitter
- Google Plus: http://www.bumblebee-project.org/g+
- Ubuntu wiki: https://wiki.ubuntu.com/Bumblebee
- Arch wiki: https://wiki.archlinux.org/index.php/Bumblebee
- Mandriva wiki: http://wiki.mandriva.com/en/Bumblebee
- Debian wiki: http://wiki.debian.org/Bumblebee
