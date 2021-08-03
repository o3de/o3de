"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os.path
from os import path
import shutil
import json


def find_or_copy_file(destFilePath, sourceFilePath):
    if path.exists(destFilePath):
        return
    if not path.exists(sourceFilePath):
        raise ValueError('find_or_copy_file: source file [', sourceFilePath, '] doesn\'t exist')
        return
    
    dstDir = path.dirname(destFilePath)
    if not path.isdir(dstDir):
        os.makedirs(dstDir)
    shutil.copyfile(sourceFilePath, destFilePath)

def load_json_file(filePath):
    file_stream = open(filePath, "r")
    return json.load(file_stream)
