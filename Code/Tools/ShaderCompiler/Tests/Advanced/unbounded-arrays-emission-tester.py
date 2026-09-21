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
import testfuncs

result = 0  # to define for sub-tests
resultFailed = 0
def doTests(compiler, silent, azdxcpath):
    """
    This test validates:
        1. unbounded-arrays-unique-idx-should-pass.azsl with --unique-idx used to fail in v1.7.19, should pass now.
        2. unbounded-arrays-unique-idx-should-pass-2srgs.azsl with --unique-idx should pass too, because each unbounded array is in a unique register space.
    """
    global result
    global resultFailed

    # Working directory should have been set to this script's directory by the calling parent
    # You can get it once doTests() is called, but not during initialization of the module,
    #  because at that time it will still be set to the working directory of the calling script
    workDir = os.getcwd().replace('\\', '/')

    # expect success when using --unique-idx
    sampleFilePath = os.path.abspath(os.path.join(workDir, "../Semantic/unbounded-arrays-unique-idx-should-pass.azsl"))
    if testhelper.verifyEmissionPatterns(sampleFilePath, compiler, silent, ["--unique-idx","--namespace=dx"]) : result += 1
    else: resultFailed += 1
    
    # expect success when using --unique-idx
    sampleFilePath = os.path.abspath(os.path.join(workDir, "../Semantic/unbounded-arrays-unique-idx-should-pass-2srgs.azsl"))
    if testhelper.verifyEmissionPatterns(sampleFilePath, compiler, silent, ["--unique-idx","--namespace=dx"]) : result += 1
    else: resultFailed += 1

    testhelper.printFailedTestList(silent)
    
if __name__ == "__main__":
    print ("please call from testapp.py")
