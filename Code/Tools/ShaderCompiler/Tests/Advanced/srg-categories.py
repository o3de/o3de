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

def verifySrgCategories (thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--srg"])

    if ok:
        predicates = []

        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][0]["id"] == "scene")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForBufferViews"][1]["id"] == "buf")
        predicates.append(lambda: j["ShaderResourceGroups"][0]["inputsForImageViews"][0]["id"] == "tex")

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

    if verifySrgCategories(os.path.join(workDir, "srg-categories.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")
