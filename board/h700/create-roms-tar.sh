#!/bin/bash

cd board/h700/ROMS
tar -Jcvf ../rootfs/root/roms.tar.xz .
cd ../../..