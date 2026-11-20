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


def verifyOutputFormats(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--om"])

    if ok:
        predicates = []
        # check all references of func()
        predicates.append(lambda: len(j["outputLayouts"]) == 12)

        predicates.append(lambda: len(j["outputLayouts"][0]["renderTargets"]) == 1)
        predicates.append(lambda: j["outputLayouts"][0]["entry"] == "MainPS_1_1")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][0]["format"]         == "R16G16B16A16_FLOAT")

        predicates.append(lambda: len(j["outputLayouts"][1]["renderTargets"]) == 1)
        predicates.append(lambda: j["outputLayouts"][1]["entry"] == "MainPS_1_2")
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][0]["semanticIndex"]  == 1)
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][0]["format"]         == "R16G16B16A16_FLOAT")

        predicates.append(lambda: len(j["outputLayouts"][2]["renderTargets"]) == 1)
        predicates.append(lambda: j["outputLayouts"][2]["entry"] == "MainPS_1_3")
        predicates.append(lambda: j["outputLayouts"][2]["renderTargets"][0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][2]["renderTargets"][0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][2]["renderTargets"][0]["format"]         == "R16G16B16A16_FLOAT")

        predicates.append(lambda: len(j["outputLayouts"][3]["renderTargets"]) == 1)
        predicates.append(lambda: j["outputLayouts"][3]["entry"] == "MainPS_1_4")
        predicates.append(lambda: j["outputLayouts"][3]["renderTargets"][0]["semanticName"]   == "SV_Depth")
        predicates.append(lambda: j["outputLayouts"][3]["renderTargets"][0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][3]["renderTargets"][0]["format"]         == "R32")

        predicates.append(lambda: len(j["outputLayouts"][4]["renderTargets"]) == 1)
        predicates.append(lambda: j["outputLayouts"][4]["entry"] == "MainPS_1_5")
        predicates.append(lambda: j["outputLayouts"][4]["renderTargets"][0]["semanticName"]   == "SV_Depth")
        predicates.append(lambda: j["outputLayouts"][4]["renderTargets"][0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][4]["renderTargets"][0]["format"]         == "R32")

        predicates.append(lambda: len(j["outputLayouts"][5]["renderTargets"]) == 1)
        predicates.append(lambda: j["outputLayouts"][5]["entry"] == "MainPS_1_8")
        predicates.append(lambda: j["outputLayouts"][5]["renderTargets"][0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][5]["renderTargets"][0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][5]["renderTargets"][0]["format"]         == "R16G16B16A16_FLOAT")

        predicates.append(lambda: len(j["outputLayouts"][6]["renderTargets"]) == 1)
        predicates.append(lambda: j["outputLayouts"][6]["entry"] == "MainPS_2_1")
        predicates.append(lambda: j["outputLayouts"][6]["renderTargets"][0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][6]["renderTargets"][0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][6]["renderTargets"][0]["format"]         == "R16G16B16A16_FLOAT")

        predicates.append(lambda: len(j["outputLayouts"][7]["renderTargets"]) == 8)
        predicates.append(lambda: j["outputLayouts"][7]["entry"] == "MainPS_2_2")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][0]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][1]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][1]["semanticIndex"]  == 1)
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][1]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][2]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][2]["semanticIndex"]  == 2)
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][2]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][3]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][3]["semanticIndex"]  == 3)
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][3]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][4]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][4]["semanticIndex"]  == 4)
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][4]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][5]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][5]["semanticIndex"]  == 5)
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][5]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][6]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][6]["semanticIndex"]  == 6)
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][6]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][7]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][7]["semanticIndex"]  == 7)
        predicates.append(lambda: j["outputLayouts"][7]["renderTargets"][7]["format"]         == "R16G16B16A16_FLOAT")

        predicates.append(lambda: len(j["outputLayouts"][8]["renderTargets"]) == 8)
        predicates.append(lambda: j["outputLayouts"][8]["entry"] == "MainPS_2_3")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][0]["semanticIndex"]  == 3)
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][0]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][1]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][1]["semanticIndex"]  == 4)
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][1]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][2]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][2]["semanticIndex"]  == 1)
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][2]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][3]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][3]["semanticIndex"]  == 6)
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][3]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][4]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][4]["semanticIndex"]  == 7)
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][4]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][5]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][5]["semanticIndex"]  == 5)
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][5]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][6]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][6]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][6]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][7]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][7]["semanticIndex"]  == 2)
        predicates.append(lambda: j["outputLayouts"][8]["renderTargets"][7]["format"]         == "R16G16B16A16_FLOAT")

        predicates.append(lambda: len(j["outputLayouts"][9]["renderTargets"]) == 8)
        predicates.append(lambda: j["outputLayouts"][9]["entry"] == "MainPS_2_3A")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][0]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][1]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][1]["semanticIndex"]  == 1)
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][1]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][2]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][2]["semanticIndex"]  == 2)
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][2]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][3]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][3]["semanticIndex"]  == 3)
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][3]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][4]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][4]["semanticIndex"]  == 4)
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][4]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][5]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][5]["semanticIndex"]  == 5)
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][5]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][6]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][6]["semanticIndex"]  == 6)
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][6]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][7]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][7]["semanticIndex"]  == 7)
        predicates.append(lambda: j["outputLayouts"][9]["renderTargets"][7]["format"]         == "R16G16B16A16_FLOAT")

        predicates.append(lambda: len(j["outputLayouts"][10]["renderTargets"]) == 2)
        predicates.append(lambda: j["outputLayouts"][10]["entry"] == "MainPS1")
        predicates.append(lambda: j["outputLayouts"][10]["renderTargets"][0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][10]["renderTargets"][0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][10]["renderTargets"][0]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][10]["renderTargets"][1]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][10]["renderTargets"][1]["semanticIndex"]  == 1)
        predicates.append(lambda: j["outputLayouts"][10]["renderTargets"][1]["format"]         == "R16G16B16A16_FLOAT")

        predicates.append(lambda: len(j["outputLayouts"][11]["renderTargets"]) == 11)
        predicates.append(lambda: j["outputLayouts"][11]["entry"] == "MainPS_All")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 0]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 1]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 1]["semanticIndex"]  == 1)
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 1]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 2]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 2]["semanticIndex"]  == 2)
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 2]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 3]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 3]["semanticIndex"]  == 3)
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 3]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 4]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 4]["semanticIndex"]  == 4)
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 4]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 5]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 5]["semanticIndex"]  == 5)
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 5]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 6]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 6]["semanticIndex"]  == 6)
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 6]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 7]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 7]["semanticIndex"]  == 7)
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 7]["format"]         == "R16G16B16A16_FLOAT")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 8]["semanticName"]   == "SV_Coverage")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 8]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 8]["format"]         == "R32")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 9]["semanticName"]   == "SV_StencilRef")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 9]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][ 9]["format"]         == "R32")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][10]["semanticName"]   == "SV_Depth")
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][10]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][11]["renderTargets"][10]["format"]         == "R32")


        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False


def verifyOutputFormatsAttr(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--om"])

    if ok:
        predicates = []
        # check all references of func()
        predicates.append(lambda: len(j["outputLayouts"]) == 2)

        predicates.append(lambda: len(j["outputLayouts"][0]["renderTargets"]) == 8)
        predicates.append(lambda: j["outputLayouts"][0]["entry"] == "MainPS_All")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][0]["format"]         == "R32")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][1]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][1]["semanticIndex"]  == 1)
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][1]["format"]         == "R32G32")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][2]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][2]["semanticIndex"]  == 2)
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][2]["format"]         == "R32A32")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][3]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][3]["semanticIndex"]  == 3)
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][3]["format"]         == "R16G16B16A16_UNORM")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][4]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][4]["semanticIndex"]  == 4)
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][4]["format"]         == "R16G16B16A16_SNORM")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][5]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][5]["semanticIndex"]  == 5)
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][5]["format"]         == "R16G16B16A16_UINT")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][6]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][6]["semanticIndex"]  == 6)
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][6]["format"]         == "R16G16B16A16_SINT")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][7]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][7]["semanticIndex"]  == 7)
        predicates.append(lambda: j["outputLayouts"][0]["renderTargets"][7]["format"]         == "R32G32B32A32")

        predicates.append(lambda: len(j["outputLayouts"][1]["renderTargets"]) == 4)
        predicates.append(lambda: j["outputLayouts"][1]["entry"] == "MainPS_Half")
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][0]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][0]["semanticIndex"]  == 0)
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][0]["format"]         == "R32")
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][1]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][1]["semanticIndex"]  == 1)
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][1]["format"]         == "R32G32")
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][2]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][2]["semanticIndex"]  == 2)
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][2]["format"]         == "R32A32")
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][3]["semanticName"]   == "SV_Target")
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][3]["semanticIndex"]  == 3)
        predicates.append(lambda: j["outputLayouts"][1]["renderTargets"][3]["format"]         == "R16G16B16A16_UNORM")


        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
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

    if verifyOutputFormats(os.path.join(workDir, "../Samples/PixelShaderOutput.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyOutputFormatsAttr(os.path.join(workDir, "../Samples/PixelShaderOutputAttributes.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")
