#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#!/bin/bash

echo "Running setup.sh script..."
echo

# setup standard groups and user account db for ssh
mkpasswd -c > /etc/passwd
mkgroup -c > /etc/group

# give special permissions to the builder user
editrights -a SeAssignPrimaryTokenPrivilege -u $1
editrights -a SeCreateTokenPrivilege -u $1
editrights -a SeTcbPrivilege -u $1
editrights -a SeServiceLogonRight -u $1

set -e

function add_if_missing {
    LINE="${1}"
    FILE="${2}"
    grep -Fx "${LINE}" "${FILE}" >/dev/null 2>&1 || echo "${LINE}" >> "${FILE}"
}

echo "  * Setting PS1 environment variable in .bashrc"
add_if_missing 'export PS1='"'"'\[\033[01;35m\]\u\[\033[34m\]@\[\033[36m\]\h\[\033[00m\]:\[\033[01;33m\]\w\[\033[31m\] \$\[\033[00m\] '"'" ~/.bashrc
echo "  * Making sure PATH gets sourced in .bashrc"
add_if_missing 'export PATH="/cygdrive/c/Windows/system32:${PATH}"' ~/.bashrc
echo "  * Making sure there is a pair of public/private keys"
[ ! -e ~/.ssh/id_rsa ] || [ ! -e ~/.ssh/id_rsa.pub ] && ssh-keygen -b 1024 -t rsa -N '' -f ~/.ssh/id_rsa
echo "  * Setting syntax highlighting in vi"
add_if_missing 'syntax on' ~/.vimrc
echo "  * Setting colour scheme in vi"
add_if_missing 'colorscheme murphy' ~/.vimrc
echo "  * Setting tab to four spaces in vi"
add_if_missing 'set tabstop=4' ~/.vimrc
[ ! -e /c ] && ln -s /cygdrive/c /c
if [ ! -e ~/bin/sudo ]; then
echo "  * Creating ""Sudo"" command"
cat > /bin/sudo << 'EOF'
#!/usr/bin/bash
cygstart --action=runas "$@"
EOF
chmod a+x /bin/sudo
fi
echo "  * Installing apt-cyg"
cd ~
wget rawgit.com/transcode-open/apt-cyg/master/apt-cyg
mv apt-cyg.* apt-get
install apt-get /bin
apt-get mirror http://mirrors.kernel.org/sourceware/cygwin/
apt-get update

echo " * Configuring the Cygwin git client to use the AWS codecommit helper"
git config --global credential.helper "!aws codecommit credential-helper $@"
git config --global credential.UseHttpPath true
git config --global filter.lfs.required true
git config --global filter.lfs.clean "git-lfs clean -- %f"
git config --global filter.lfs.smudge "git-lfs smudge -- %f"
git config --global filter.lfs.process "git-lfs filter-process"