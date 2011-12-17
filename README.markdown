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

autotools (2.68+ recommended)
libx11 and development headers
[bbswitch](https://github.com/Lekensteyn/acpi-stuff/tree/master/bbswitch) (optional)

Building
---------

    autoreconf -fi
    ./configure
    make

Usage
------

    sudo ./bumblebeed -d
    ./optirun <application>
