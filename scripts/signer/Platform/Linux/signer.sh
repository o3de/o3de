#!/bin/bash

#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set +x

export GPG_TTY=$(tty) # Required to pass valid tty during ssh sessions
file=$1

# dpkg-sig depends on a valid and trusted GPG private key. This also assumes a private key password has already been cached via gpg-agent
# If you do need to pass a password, a gpg argument can be added to the command:
#   dpkg-sig -k $fingerprint -g "--pinentry-mode loopback --passphrase $pass" --sign builder

fingerprint=$(gpg --list-keys --with-colons  | awk -F: '/fpr:/ {print $10}' | tail -n1) #Get the last certificate in the list, which is the signing cert
if [ -z $fingerprint ]; then
    echo "No valid certs found. Exiting with 1"
    exit 1
fi
echo "Signing with $fingerprint"
dpkg-sig -k $fingerprint --sign builder $file
dpkg-sig --verify $file && echo "Signing $file complete!"
