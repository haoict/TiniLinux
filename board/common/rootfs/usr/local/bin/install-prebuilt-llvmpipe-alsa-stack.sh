#!/bin/bash
if [ -f /usr/local/lib/libLLVM.so ]; then
    echo "Prebuilt LLVM and ALSA stack already installed. Skip downloading"
else
    git clone --depth 1 https://github.com/haoict/prebuilt-buildroot-libs-aarch64.git
    cd prebuilt-buildroot-libs-aarch64
    cp -r llvmpipe-stack/usr/* /usr/
    cp -r alsa-stack/usr/* /usr/
    ln -sf /usr/local/lib/gbm/ /usr/lib/
    cd ..
    rm -rf prebuilt-buildroot-libs-aarch64
fi

# for pi3b_development
if [ -f /boot/config.txt ]; then
    echo "Updating pi3b_development modprobe blacklist and config.txt"
    sed -i 's/^blacklist snd$//g' /etc/modprobe.d/my-blacklist.conf
    sed -i 's/^blacklist snd_bcm2835$//g' /etc/modprobe.d/my-blacklist.conf
    sed -i 's/^blacklist snd_usb_audio$//g' /etc/modprobe.d/my-blacklist.conf
    sed -i 's/^dtparam=audio=off$//g' /boot/config.txt
    # check if dtparam=audio=on already exists, if not add it
    if ! grep -q "dtparam=audio=on" /boot/config.txt; then
        sed -i 's/^arm_64bit=1$/arm_64bit=1\ndtparam=audio=on/g' /boot/config.txt
    fi
fi