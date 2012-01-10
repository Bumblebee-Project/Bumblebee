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
- [bbswitch](https://github.com/Bumblebee-Project/bbswitch) (optional, for PM)

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
