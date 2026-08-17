#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""
import sys
import os

import platform
sys.path.append("..")
from clr import *
import os.path
from os.path import join, normpath, basename
import testhelper

result = 0  # to define for sub-tests
resultFailed = 0
def doTests(compiler, silent, azdxcpath):
    """
    The purpose of this test is to validate proper functionality of the compiler option --no-ms.
    --no-ms is an option that triggers automatic conversion of MultiSampling related Texture variables, function calls
    and system semantics, to their non-MultiSampling version. Example Texture2DMS to Texture2D, Texture2DMSArray to Texture2DArray, etc.
    """
    global result
    global resultFailed

    # Working directory should have been set to this script's directory by the calling parent
    # You can get it once doTests() is called, but not during initialization of the module,
    #  because at that time it will still be set to the working directory of the calling script
    workDir = os.getcwd().replace('\\', '/')

    # First let's make sure that WITHOUT --no-ms, the shader compiles fine and all the MultiSampling related variables and function call
    # remain unchanged.
    azslFile = os.path.abspath(os.path.join(workDir, "./texture2DMS-to-texture2D.azsl"))
    
    failList = []
    patternFile = os.path.abspath(os.path.join(workDir, "./texture2DMS-to-texture2D.txt"))
    if testhelper.verifyEmissionPattern(azslFile, patternFile, compiler, silent, []) : result += 1
    else:
        failList.append(patternFile)
        resultFailed += 1

    patternFile = os.path.abspath(os.path.join(workDir, "./texture2DMS-to-texture2D-noms.txt"))
    if testhelper.verifyEmissionPattern(azslFile, patternFile, compiler, silent, ["--no-ms"]) : result += 1
    else:
        failList.append(patternFile)
        resultFailed += 1

    if not silent and len(failList) > 0:
        print(style.BRIGHT + fg.RED + "failed files: " + fg.WHITE + str(failList) + style.RESET_ALL)
    
if __name__ == "__main__":
    print ("please call from testapp.py")
