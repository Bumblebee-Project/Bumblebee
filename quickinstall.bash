#!/bin/bash

autoreconf -fi
./configure CONF_DRIVER=nvidia CONF_DRIVER_MODULE_NVIDIA=nvidia???? \
  CONF_PRIMUS_LD_PATH=/usr/lib/x86_64-linux-gnu/primus:/usr/lib/i386-linux-gnu/primus \
  CONF_LDPATH_NVIDIA=/usr/lib/nvidia????:/usr/lib32/nvidia???? \
  CONF_MODPATH_NVIDIA=/usr/lib/nvidia????/xorg,/usr/lib/xorg/modules \

make clean
make
sudo make install
sudo cp scripts/systemd/bumblebeed.service /lib/systemd/system/.


