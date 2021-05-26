"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
