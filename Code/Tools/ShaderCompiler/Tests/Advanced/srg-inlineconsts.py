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
sys.path.append("../..")
from clr import *
import testfuncs

def verifyOK(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--srg", "--root-const=52"])

    if ok:
        predicates = []

        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["count"]                    ==    1)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["index"]                    ==    0)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["space"]                    ==    0)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["usage"]                    ==    "Read")
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["sizeInBytes"]              ==    60)
        predicates.append(lambda: j["RootConstantBuffer"]["bufferForRootConstants"]["id"]                       ==    "Root_Constants")

        if not silent: print (fg.CYAN+ style.BRIGHT+ "inline const verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyZeroOK(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--srg", "--root-const=0"])

    return True if ok else False

def verifyZeroFails(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--srg", "--root-const=0"])

    return True if not ok else False
result = 0  # to define for sub-tests
resultFailed = 0
def doTests(compiler, silent, azdxcpath):
    global result
    global resultFailed

    # Working directory should have been set to this script's directory by the calling parent
    # You can get it once doTests() is called, but not during initialization of the module,
    #  because at that time it will still be set to the working directory of the calling script
    workDir = os.getcwd()

    if verifyOK(os.path.join(workDir, "srg-inlineconsts.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyZeroOK(os.path.join(workDir, "simple.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyZeroFails(os.path.join(workDir, "srg-inlineconsts.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")
