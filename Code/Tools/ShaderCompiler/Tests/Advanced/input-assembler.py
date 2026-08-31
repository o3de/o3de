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
        predicates.append(lambda: len(j["inputLayouts"]) == 3)

        predicates.append(lambda: len(j["inputLayouts"][0]["streams"]) == 3)
        predicates.append(lambda: j["inputLayouts"][0]["streams"][0]["name"] == "m_position")
        predicates.append(lambda: j["inputLayouts"][0]["streams"][0]["semanticName"] == "POSITION")
        predicates.append(lambda: j["inputLayouts"][0]["streams"][0]["systemValue"] == False)
        predicates.append(lambda: j["inputLayouts"][0]["streams"][1]["name"] == "m_color")
        predicates.append(lambda: j["inputLayouts"][0]["streams"][1]["dimensions"][0] == 4)
        predicates.append(lambda: j["inputLayouts"][0]["streams"][1]["semanticName"] == "COLOR")
        predicates.append(lambda: j["inputLayouts"][0]["streams"][1]["systemValue"] == False)
        predicates.append(lambda: j["inputLayouts"][0]["streams"][2]["name"] == "vtxIndex")
        predicates.append(lambda: j["inputLayouts"][0]["streams"][2]["semanticName"] == "SV_VertexID")
        predicates.append(lambda: j["inputLayouts"][0]["streams"][2]["systemValue"] == True)

        predicates.append(lambda: len(j["inputLayouts"][1]["streams"]) == 10)
        predicates.append(lambda: j["inputLayouts"][1]["streams"][0]["name"] == "m_position")
        predicates.append(lambda: j["inputLayouts"][1]["streams"][0]["semanticName"] == "POSITION")
        predicates.append(lambda: j["inputLayouts"][1]["streams"][0]["systemValue"] == False)
        predicates.append(lambda: j["inputLayouts"][1]["streams"][9]["name"] == "instId")
        predicates.append(lambda: j["inputLayouts"][1]["streams"][9]["semanticName"] == "SV_InstanceID")
        predicates.append(lambda: j["inputLayouts"][1]["streams"][9]["systemValue"] == True)
        predicates.append(lambda: j["inputLayouts"][1]["streams"][8]["name"] == "vtxIndex")
        predicates.append(lambda: j["inputLayouts"][1]["streams"][8]["semanticName"] == "SV_VertexID")
        predicates.append(lambda: j["inputLayouts"][1]["streams"][8]["systemValue"] == True)

        predicates.append(lambda: len(j["inputLayouts"][2]["streams"]) == 2)
        predicates.append(lambda: j["inputLayouts"][2]["streams"][0]["name"] == "position")
        predicates.append(lambda: j["inputLayouts"][2]["streams"][0]["semanticName"] == "POSITION")
        predicates.append(lambda: j["inputLayouts"][2]["streams"][0]["systemValue"] == False)
        predicates.append(lambda: j["inputLayouts"][2]["streams"][1]["name"] == "color")
        predicates.append(lambda: j["inputLayouts"][2]["streams"][1]["semanticName"] == "COLOR")
        predicates.append(lambda: j["inputLayouts"][2]["streams"][1]["systemValue"] == False)

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

    if verifyInputLayouts(os.path.join(workDir, "input-assembler.azsl"), compiler, silent): result += 1
    else: resultFailed += 1


if __name__ == "__main__":
    print ("please call from testapp.py")
