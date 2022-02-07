#!/bin/bash

# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# This script must be run as root
if [[ $EUID -ne 0 ]]
then
    echo "This script must be run as root (sudo)"
    exit 1
fi

# Install latest updates to AWS drivers and GCC (to build display driver)
#
echo Updating OS and tools
apt-get update -y
apt-get upgrade -y linux-aws
apt-get install -y gcc make linux-headers-$(uname -r)

# Install Desktop environment with tools. Nice is only compatible with Gnome
#
echo Installing desktop environment and tools
apt-get install ubuntu-desktop mesa-utils vulkan-tools awscli unzip -y

# Setup X desktop manager (Wayland needs to be turned off for GDM3)
#
if [ "`cat /etc/issue | grep 18.04`" != "" ] ; then
    apt-get install lightdm -y
else
    apt-get install gdm3 -y
    sed -i 's/#WaylandEnable=false/WaylandEnable=false/g' /etc/gdm3/custom.conf
    systemctl restart gdm3
fi

# Set desktop environment to start by default
#
systemctl get-default
systemctl set-default graphical.target
systemctl isolate graphical.target

# Prepare for the nVidia driver by disabling nouveau
#
cat << EOF | sudo tee --append /etc/modprobe.d/blacklist.conf
blacklist vga16fb
blacklist nouveau
blacklist rivafb
blacklist nvidiafb
blacklist rivatv
EOF

# Blocking nouveau from activating during grub startup
#
echo 'GRUB_CMDLINE_LINUX="rdblacklist=nouveau"' >> /etc/default/grub
update-grub

# Copy drivers to local, then install
#
aws s3 cp --recursive s3://nvidia-gaming/linux/latest/ /tmp
cd /tmp
unzip NVIDIA-Linux-x86_64* \
&& rm NVIDIA-Linux-x86_64* \
&& chmod +x Linux/NVIDIA-Linux-x86_64*.run \
&& Linux/NVIDIA-Linux-x86_64*.run --accept-license \
                              --no-questions \
                              --no-backup \
                              --ui=none \
                              --install-libglvnd \
&& nvidia-xconfig --preserve-busid --enable-all-gpus \
&& rm -rf /tmp/Linux

# Download and configure licenses (needed for VMs and multiuser)
#
cat << EOF | sudo tee -a /etc/nvidia/gridd.conf
vGamingMarketplace=2
EOF
curl -o /etc/nvidia/GridSwCert.txt "https://nvidia-gaming.s3.amazonaws.com/GridSwCert-Archive/GridSwCertLinux_2021_10_2.cert"

# Optimize settings if headless
#
if [ ! $DISPLAY ] ; then
    echo Headless instance found. Disabling HardDPMS
    if [ ! $(grep '"HardDPMS" "false"' /etc/X11/xorg.conf) ]; then
        sed -i '/BusID */ a\
            Option         "HardDPMS" "false"' /etc/X11/xorg.conf
    fi
fi

echo Install complete!
read -t 10 -p "Rebooting in 10 seconds. Press enter to reboot this instance now or CTRL+c to cancel"
reboot now

