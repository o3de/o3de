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


def verifyInputLayouts(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--ia"])
    if ok:
        predicates = []
        # check all references of func()
        predicates.append(lambda: len(j["inputLayouts"]) == 5)

        predicates.append(lambda: j["inputLayouts"][0]["entry"] == "MainCS1")
        predicates.append(lambda: len(j["inputLayouts"][0]["numthreads"]) == 3)
        predicates.append(lambda: j["inputLayouts"][0]["numthreads"][0] == 1)
        predicates.append(lambda: j["inputLayouts"][0]["numthreads"][1] == 1)
        predicates.append(lambda: j["inputLayouts"][0]["numthreads"][2] == 1)

        predicates.append(lambda: j["inputLayouts"][1]["entry"] == "MainCS2")
        predicates.append(lambda: len(j["inputLayouts"][1]["numthreads"]) == 3)
        predicates.append(lambda: j["inputLayouts"][1]["numthreads"][0] == 8)
        predicates.append(lambda: j["inputLayouts"][1]["numthreads"][1] == 1)
        predicates.append(lambda: j["inputLayouts"][1]["numthreads"][2] == 1)

        predicates.append(lambda: j["inputLayouts"][2]["entry"] == "MainCS3")
        predicates.append(lambda: len(j["inputLayouts"][2]["numthreads"]) == 3)
        predicates.append(lambda: j["inputLayouts"][2]["numthreads"][0] == 16)
        predicates.append(lambda: j["inputLayouts"][2]["numthreads"][1] == 4)
        predicates.append(lambda: j["inputLayouts"][2]["numthreads"][2] == 1)

        predicates.append(lambda: j["inputLayouts"][3]["entry"] == "MainCS4")
        predicates.append(lambda: len(j["inputLayouts"][3]["numthreads"]) == 3)
        predicates.append(lambda: j["inputLayouts"][3]["numthreads"][0] == 1)
        predicates.append(lambda: j["inputLayouts"][3]["numthreads"][1] == 1)
        predicates.append(lambda: j["inputLayouts"][3]["numthreads"][2] == 64)

        predicates.append(lambda: j["inputLayouts"][4]["entry"] == "MainVS1")
        predicates.append(lambda: "numthreads" not in j["inputLayouts"][4].keys())

        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return ok

result = 0  # to define for sub-tests
resultFailed = 0

def doTests(compiler, silent, azdxcpath):
    global result
    global resultFailed

    # Working directory should have been set to this script's directory by the calling parent
    # You can get it once doTests() is called, but not during initialization of the module,
    #  because at that time it will still be set to the working directory of the calling script
    workDir = os.getcwd()

    if verifyInputLayouts(os.path.join(workDir, "compute-entries.azsl"), compiler, silent): result += 1
    else: resultFailed += 1


if __name__ == "__main__":
    print ("please call from testapp.py")
