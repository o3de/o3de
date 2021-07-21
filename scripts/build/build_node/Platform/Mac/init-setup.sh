#!/bin/bash 
set -euo pipefail

# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

BUILD_USER=$(aws ssm get-parameters --names "shared.builderuser" --region $region --with-decryption | ConvertFrom-Json)
BUILD_PASS=$(aws ssm get-parameters --names "shared.builderpass" --region $region --with-decryption | ConvertFrom-Json)

echo "Setting up ${BUILD_USER} as autologin admin"
sudo sysadminctl -addUser "${BUILD_USER}" -fullName "${BUILD_USER}" -password "${BUILD_PASS}" -admin
sudo echo "${BUILD_USER} ALL=(ALL) NOPASSWD:ALL" | sudo tee /etc/sudoers.d/${BUILD_USER}
sudo /usr/bin/defaults write /Library/Preferences/com.apple.loginwindow autoLoginUser "${BUILD_USER}"

echo "Change ownership for brew to ${BUILD_USER}"
sudo chown -R "${BUILD_USER}":admin $(brew --prefix)/*

echo "Configure SSH"
mkdir /Users/"${BUILD_USER}"/.ssh
echo "PATH=/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin:/Library/Apple/usr/bin" > /Users/"${BUILD_USER}"/.ssh/environment
sudo sed -i -e 's/#PermitUserEnvironment no/PermitUserEnvironment yes/g' /etc/ssh/sshd_config
sudo launchctl stop com.openssh.sshd && sudo launchctl start com.openssh.sshd

echo "Expanding root volume to EBS configurated size"
PDISK=$(diskutil list physical external | head -n1 | cut -d" " -f1)
APFSCONT=$(diskutil list physical external | grep "Apple_APFS" | tr -s " " | cut -d" " -f8)
yes | sudo diskutil repairDisk ${PDISK}
sudo diskutil apfs resizeContainer ${APFSCONT} 0

echo "Removing hibernate and sleep image"
sudo pmset hibernatemode 0
sudo rm -f /var/vm/sleepimage

echo "Disable screensaver and sleep modes"
macUUID=$(ioreg -rd1 -c IOPlatformExpertDevice | grep -i "UUID" | cut -c27-62)

rm -rf /Users/"${BUILD_USER}"/Library/Preferences/com.apple.screensaver.${macUUID}.plist
rm -rf /Users/"${BUILD_USER}"/Library/Preferences/ByHost/com.apple.screensaver.${macUUID}.plist
rm -rf /Users/"${BUILD_USER}"/Library/Preferences/com.apple.screensaver.plist
rm -rf /Users/"${BUILD_USER}"/Library/Preferences/ByHost/com.apple.screensaver.plist

defaults write /Users/"${BUILD_USER}"/Library/Preferences/com.apple.screensaver.${macUUID}.plist idleTime -string 0
defaults write /Users/"${BUILD_USER}"/Library/Preferences/com.apple.screensaver.${macUUID}.plist CleanExit "YES"
defaults write /Users/"${BUILD_USER}"/Library/Preferences/ByHost/com.apple.screensaver.${macUUID}.plist idleTime -string 0
defaults write /Users/"${BUILD_USER}"/Library/Preferences/ByHost/com.apple.screensaver.${macUUID}.plist CleanExit "YES"
defaults write /Users/"${BUILD_USER}"/Library/Preferences/com.apple.screensaver.plist idleTime -string 0
defaults write /Users/"${BUILD_USER}"/Library/Preferences/com.apple.screensaver.plist CleanExit "YES"
defaults write /Users/"${BUILD_USER}"/Library/Preferences/ByHost/com.apple.screensaver.plist idleTime -string 0
defaults write /Users/"${BUILD_USER}"/Library/Preferences/ByHost/com.apple.screensaver.plist CleanExit "YES"

chown -R "${BUILD_USER}":staff /Users/"${BUILD_USER}"/Library/Preferences/ByHost/
chown -R "${BUILD_USER}":staff /Users/"${BUILD_USER}"/Library/Preferences/

killall cfprefsd

# Set values to 0, to prevent sleep at all
pmset -a displaysleep 0 sleep 0 disksleep 0

echo "Disable automatic updates"
sudo softwareupdate --schedule off
defaults write com.apple.SoftwareUpdate AutomaticDownload -int 0
defaults write com.apple.SoftwareUpdate CriticalUpdateInstall -int 0
defaults write com.apple.commerce AutoUpdate -bool false
defaults write com.apple.SoftwareUpdate AutomaticCheckEnabled -bool false

echo "Additional NTP servers adding into /etc/ntp.conf file"
cat > /etc/ntp.conf << EOF
server 0.pool.ntp.org
server 1.pool.ntp.org
server 2.pool.ntp.org
server 3.pool.ntp.org
server time.apple.com
server time.windows.com
EOF

# Set the timezone to UTC.
echo "Setting timezone to UTC"
ln -sf /usr/share/zoneinfo/UTC /etc/localtime