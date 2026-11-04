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

def verifyIASemantics (thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--ia"])

    if ok:
        predicates = []

        predicates.append(lambda: j["inputLayouts"][0]["streams"][0]["systemValue"]  == 0)
        predicates.append(lambda: j["inputLayouts"][0]["streams"][1]["systemValue"]  == 0)
        predicates.append(lambda: j["inputLayouts"][1]["streams"][0]["systemValue"]  == 1) #SV_POSITION
        predicates.append(lambda: j["inputLayouts"][1]["streams"][1]["systemValue"]  == 0)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler semantics verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyOMSemantics (thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--om"])

    if ok:
        predicates = []

        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][0]["systemValue"]  == 1) #SV_TARGET

        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler semantics verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

result = 0  # to define for sub-tests
resultFailed = 0
def doTests(compiler, silent, azdxcpath):
    global result
    global resultFailed

    # Working directory should have been set to this script's directory by the calling parent
    # You can get it once doTests() is called, but not during initialization of the module,
    #  because at that time it will still be set to the working directory of the calling script
    workDir = os.getcwd()

    if verifyIASemantics(os.path.join(workDir, "srg-semantics.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyOMSemantics(os.path.join(workDir, "srg-semantics.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")
