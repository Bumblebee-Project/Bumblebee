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
- libbsd and development headers

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

Usage
------

    sudo bin/bumblebeed --daemon
    bin/optirun -- <application>
    
For more information, try --help on either of the two binaries.

Installing System-wide and Packaging
-------------------------------------

You can build the binaries and set the system wide paths at configure time

    autoreconf -fi
    ./configure --prefix=/usr --sysconfdir=/etc
    make

After building the binaries they can be installed using make:

    sudo make install

For packagers you need to add DESTDIR=$pkgdir

    make install DESTDIR=$pkgdir
