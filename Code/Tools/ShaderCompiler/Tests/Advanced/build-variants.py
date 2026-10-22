#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""
import sys
import os
from os.path import join, normpath, basename

result = 0  # to define for sub-tests
resultFailed = 0

def doTests(compiler, silent, azdxcpath):
    global result
    global resultFailed

    # Working directory should have been set to this script's directory by the calling parent
    # You can get it once doTests() is called, but not during initialization of the module,
    #  because at that time it will still be set to the working directory of the calling script
    workDir = os.getcwd()

    azslShaderFileList = ["../Samples/Variants.azsl", "../Samples/VariantsReorder.azsl"]

    # Here's one bonus success for you.
    # This script is deprecated - it's being moved to the Platform/ folders and will be deleted after merging
    result += 1


if __name__ == "__main__":
    print ("please call from testapp.py")
