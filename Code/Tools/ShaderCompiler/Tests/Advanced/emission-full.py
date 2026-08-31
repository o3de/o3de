#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import sys
import os
import re
import testfuncs

result = 0  # to define for sub-tests
resultFailed = 0 # to define for sub-tests

# delete artefacts from previous compilations
def cleanArtefacts(workDir):
    if os.path.exists("simple-surface.hlsl"): os.remove("simple-surface.hlsl")
    if os.path.exists("simple-surface.ia.json"): os.remove("simple-surface.ia.json")
    if os.path.exists("simple-surface.om.json"): os.remove("simple-surface.om.json")
    if os.path.exists("simple-surface.srg.json"): os.remove("simple-surface.srg.json")
    if os.path.exists("simple-surface.options.json"): os.remove("simple-surface.options.json")
    if os.path.exists("simple-surface.bindingdep.json"): os.remove("simple-surface.bindingdep.json")

def doTests(compiler, silent, az3rdParty):
    global result
    global resultFailed

    # Working directory should have been set to this script's directory by the calling parent
    # You can get it once doTests() is called, but not during initialization of the module,
    #  because at that time it will still be set to the working directory of the calling script
    workDir = os.getcwd()

    cleanArtefacts(workDir)

    if testfuncs.buildAndGet("simple-surface.azsl", compiler, silent, ["-o", "simple-surface.hlsl", "--full"]) : result += 1
    else: resultFailed += 1

    # check existence of HLSL output file
    if os.stat("simple-surface.hlsl").st_size != 0 : result += 1
    else: resultFailed += 1

    # check existence of JSON output files

    if os.stat("simple-surface.ia.json").st_size != 0 : result += 1
    else: resultFailed += 1

    if os.stat("simple-surface.om.json").st_size != 0 : result += 1
    else: resultFailed += 1

    if os.stat("simple-surface.srg.json").st_size != 0 : result += 1
    else: resultFailed += 1

    if os.stat("simple-surface.options.json").st_size != 0 : result += 1
    else: resultFailed += 1

    if os.stat("simple-surface.bindingdep.json").st_size != 0 : result += 1
    else: resultFailed += 1

    cleanArtefacts(workDir)

if __name__ == "__main__":
    print ("please call from testapp.py")
