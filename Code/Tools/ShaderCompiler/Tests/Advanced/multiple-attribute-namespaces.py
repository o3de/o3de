#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import sys
import os
sys.path.append("..")
from clr import *
import testfuncs
import testhelper

'''
Validates having multiple attribute namespaces in the same file.
'''


result = 0  # to define for sub-tests
resultFailed = 0

def doTests(compiler, silent, azdxcpath):
    global result
    global resultFailed

    # Working directory should have been set to this script's directory by the calling parent
    # You can get it once doTests() is called, but not during initialization of the module,
    #  because at that time it will still be set to the working directory of the calling script
    workDir = os.getcwd()

    if testhelper.verifyEmissionPattern("multiple-attribute-namespaces.azsl", "multiple-attribute-namespaces.txt", compiler, silent, ["--namespace=mt", "--namespace=vk"]):
        result += 1
    else:
        resultFailed += 1

    if testhelper.compileAndExpectError("multiple-attribute-namespaces.azsl", compiler, silent, ["--namespace=mt,vk"]):
        result += 1
    else:
        resultFailed += 1
        
    if testhelper.compileAndExpectError("multiple-attribute-namespaces.azsl", compiler, silent, ["--namespace=mt&vk"]):
        result += 1
    else:
        resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")