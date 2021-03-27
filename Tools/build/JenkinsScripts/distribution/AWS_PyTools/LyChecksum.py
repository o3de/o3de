"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import hashlib
import re


def generateFilesetChecksum(filePaths):
    filesetHash = hashlib.sha512()
    for filePath in filePaths:
        updateHashWithFileChecksum(filePath, filesetHash)
    return filesetHash


def getChecksumForSingleFile(filePath, openMode='rb'):
    filesetHash = hashlib.sha512()
    updateHashWithFileChecksum(filePath, filesetHash, openMode)
    return filesetHash


def getMD5ChecksumForSingleFile(filePath, openMode='rb'):
    filesetHash = hashlib.md5()
    updateHashWithFileChecksum(filePath, filesetHash, openMode)
    return filesetHash


def updateHashWithFileChecksum(filePath, filesetHash, openMode='rb'):
    BLOCKSIZE = 65536
    with open(filePath.strip('\n'), openMode) as file:
        buf = file.read(BLOCKSIZE)
        while len(buf) > 0:
            filesetHash.update(buf)
            buf = file.read(BLOCKSIZE)


def is_valid_hash_sha1(checksum):
    # sha1 hashes are 40 hex characters long.
    if len(checksum) is not 40:
        return False
    sha1_re = re.compile("(^[0-9A-Fa-f]{40}$)")
    result = sha1_re.match(checksum)
    if not result:
        return False
    return True


def is_valid_hash_sha512(checksum):
    # sha512 hashes are 128 hex characters long.
    if len(checksum) is not 128:
        return False
    sha512_re = re.compile("(^[0-9A-Fa-f]{128}$)")
    result = sha512_re.match(checksum)
    if not result:
        return False
    return True
