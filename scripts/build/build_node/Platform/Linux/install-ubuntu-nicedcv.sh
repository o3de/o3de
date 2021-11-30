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

# Download Nice DCV
#
echo Downloading Nice-DCV
mkdir /tmp/nice-dcv
cd /tmp/nice-dcv
curl -sSL https://d1uj6qtbmh3dt5.cloudfront.net/NICE-GPG-KEY | gpg --import -

if [ "`cat /etc/issue | grep 18.04`" != "" ] ; then
    dcv_server=`curl --silent --output - https://download.nice-dcv.com/ | \
grep href | egrep "$dcv_version" | egrep "ubuntu1804" | grep Server | \
sed -e 's/.*http/http/' -e 's/tgz.*/tgz/' | head -1`
else
    dcv_server=`curl --silent --output - https://download.nice-dcv.com/ | \
grep href | egrep "$dcv_version" | egrep "ubuntu2004" | grep Server | \
sed -e 's/.*http/http/' -e 's/tgz.*/tgz/' | head -1`
fi

# Install Nice DCV
#
echo Installing DCV from $dcv_server
wget $dcv_server \
&& tar zxvf nice-dcv-*ubun*.tgz
cd nice-dcv-*64
apt install -y ./nice-*amd64.ubuntu*.deb \
&& usermod -aG video dcv \
&& rm -rf /tmp/nice-dcv

# Setup multiuser environment for DCV
#
systemctl isolate multi-user.target
sleep 1
dcvgladmin enable
systemctl isolate graphical.target

# Check if DCV localuser for X has been setup correctly
#
if [ $(DISPLAY=:0 XAUTHORITY=$(ps aux | grep "X.*\-auth" | grep -v grep | sed -n 's/.*-auth \([^ ]\+\).*/\1/p') xhost | grep "SI:localuser:dcv$") ]; then
    echo DCV localuser validated
else
    echo [ERROR] DCV localuser not found. Output should be: SI:localuser:dcv. Exiting with 1
    exit 1
fi

# Configure DCV for auto start sessions and performance
#
sed -i "s/#create-session = true/create-session = true/g" /etc/dcv/dcv.conf
sed -i "s/#owner=\"session-owner\"/owner=\"$LOGNAME\"/g" /etc/dcv/dcv.conf
sed -i "s/#target-fps=30/target-fps=0/g" /etc/dcv/dcv.conf

# Enable and start the DCV server
#
systemctl enable dcvserver
systemctl start dcvserver

# Output the DCV installation diagnostics
#
dcvgldiag

# Start a DCV session to login
#
IP_ADDR=$(curl http://checkip.amazonaws.com)
echo Starting DCV desktop session for $LOGNAME. Session can be accessed at https://$IP_ADDR:8443
dcv create-session --type=console --owner $LOGNAME desktop
