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

def verifyOptionsEmission(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--options"])

    if ok:
        predicates = []
        # check all references of func()

        predicates.append(lambda: j["ShaderOptions"][0]["defaultValue"]  ==   "FakeT::One")
        predicates.append(lambda: j["ShaderOptions"][0]["name"]          ==   "Fake1")
        predicates.append(lambda: j["ShaderOptions"][1]["defaultValue"]  ==   "FakeT::Two")
        predicates.append(lambda: j["ShaderOptions"][1]["name"]          ==   "Fake2")
        predicates.append(lambda: j["ShaderOptions"][2]["defaultValue"]  ==   "FakeT::Three")
        predicates.append(lambda: j["ShaderOptions"][2]["name"]          ==   "Fake3")
        predicates.append(lambda: j["ShaderOptions"][3]["defaultValue"]  ==   "FakeT::Four")
        predicates.append(lambda: j["ShaderOptions"][3]["name"]          ==   "Fake4")
        predicates.append(lambda: j["ShaderOptions"][4]["defaultValue"]  ==   "FakeT::Five")
        predicates.append(lambda: j["ShaderOptions"][4]["name"]          ==   "Fake5")

        # Currently the order of the options matches their order of declaration:
        for i in range(0, 15):
            predicates.append(lambda: j["ShaderOptions"][i]["order"]       ==   i)

        # Some repetitive emissions
        for i in range(0, 11):
            predicates.append(lambda: j["ShaderOptions"][i]["type"]          ==   "FakeT")
            predicates.append(lambda: j["ShaderOptions"][i]["keySize"]       ==   3)
        for i in range(5, 11):
            predicates.append(lambda: j["ShaderOptions"][i]["defaultValue"]  ==   "FakeT::One")

        # Testing the key bit-packing and 32-bit alignment
        predicates.append(lambda: j["ShaderOptions"][0]["keyOffset"]     ==   0)
        predicates.append(lambda: j["ShaderOptions"][1]["keyOffset"]     ==   3)
        predicates.append(lambda: j["ShaderOptions"][2]["keyOffset"]     ==   6)
        predicates.append(lambda: j["ShaderOptions"][3]["keyOffset"]     ==   9)
        predicates.append(lambda: j["ShaderOptions"][4]["keyOffset"]     ==   12)
        predicates.append(lambda: j["ShaderOptions"][5]["keyOffset"]     ==   15)
        predicates.append(lambda: j["ShaderOptions"][6]["keyOffset"]     ==   18)
        predicates.append(lambda: j["ShaderOptions"][7]["keyOffset"]     ==   21)
        predicates.append(lambda: j["ShaderOptions"][8]["keyOffset"]     ==   24)
        predicates.append(lambda: j["ShaderOptions"][9]["keyOffset"]     ==   27)
        predicates.append(lambda: j["ShaderOptions"][10]["keyOffset"]    ==   32)  # Notice the 32-bit alignment here!
        predicates.append(lambda: j["ShaderOptions"][11]["keyOffset"]    ==   35)

        predicates.append(lambda: j["ShaderOptions"][12]["defaultValue"] ==   "QualityT::High")
        predicates.append(lambda: j["ShaderOptions"][12]["name"]         ==   "Quality")
        predicates.append(lambda: j["ShaderOptions"][12]["type"]         ==   "QualityT")
        predicates.append(lambda: j["ShaderOptions"][12]["keyOffset"]    ==   38)
        predicates.append(lambda: j["ShaderOptions"][12]["keySize"]      ==   2)
        predicates.append(lambda: j["ShaderOptions"][13]["defaultValue"] ==   "Red")
        predicates.append(lambda: j["ShaderOptions"][13]["name"]         ==   "Color")
        predicates.append(lambda: j["ShaderOptions"][13]["type"]         ==   "ColorT")
        predicates.append(lambda: j["ShaderOptions"][13]["keyOffset"]    ==   40)
        predicates.append(lambda: j["ShaderOptions"][13]["keySize"]      ==   2)
        predicates.append(lambda: j["ShaderOptions"][14]["defaultValue"] ==   "true")
        predicates.append(lambda: j["ShaderOptions"][14]["name"]         ==   "UseGI")
        predicates.append(lambda: j["ShaderOptions"][14]["type"]         ==   "?bool")
        predicates.append(lambda: j["ShaderOptions"][14]["keyOffset"]    ==   42)
        predicates.append(lambda: j["ShaderOptions"][14]["keySize"]      ==   1)
        predicates.append(lambda: j["ShaderOptions"][15]["defaultValue"] ==   "42")
        predicates.append(lambda: j["ShaderOptions"][15]["name"]         ==   "IntOption")
        predicates.append(lambda: j["ShaderOptions"][15]["type"]         ==   "?int")
        predicates.append(lambda: j["ShaderOptions"][15]["keyOffset"]    ==   43)
        predicates.append(lambda: j["ShaderOptions"][15]["keySize"]      ==   6)
        predicates.append(lambda: j["ShaderOptions"][15]["range"]        ==   True)
        predicates.append(lambda: j["ShaderOptions"][15]["values"][0]    ==   "1")
        predicates.append(lambda: j["ShaderOptions"][15]["values"][1]    ==   "64")

        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyOptionsEmissionIntegerRanges(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--options"])

    if ok:
        predicates = []
        # check all references of func()

        for i in range(0, 2):
            predicates.append(lambda: j["ShaderOptions"][i]["type"]         ==   "?int")
            predicates.append(lambda: j["ShaderOptions"][i]["range"]        ==   True)

        predicates.append(lambda: j["ShaderOptions"][0]["defaultValue"] ==   "42")
        predicates.append(lambda: j["ShaderOptions"][0]["name"]         ==   "IntOptionA")
        predicates.append(lambda: j["ShaderOptions"][0]["keyOffset"]    ==   0)
        predicates.append(lambda: j["ShaderOptions"][0]["keySize"]      ==   6)
        predicates.append(lambda: j["ShaderOptions"][0]["values"][0]    ==   "1")
        predicates.append(lambda: j["ShaderOptions"][0]["values"][1]    ==   "64")

        predicates.append(lambda: j["ShaderOptions"][1]["defaultValue"] ==   "7")
        predicates.append(lambda: j["ShaderOptions"][1]["name"]         ==   "IntOptionB")
        predicates.append(lambda: j["ShaderOptions"][1]["keyOffset"]    ==   6)
        predicates.append(lambda: j["ShaderOptions"][1]["keySize"]      ==   2)
        predicates.append(lambda: j["ShaderOptions"][1]["values"][0]    ==   "5")
        predicates.append(lambda: j["ShaderOptions"][1]["values"][1]    ==   "8")

        predicates.append(lambda: j["ShaderOptions"][2]["defaultValue"] ==   "9")
        predicates.append(lambda: j["ShaderOptions"][2]["name"]         ==   "IntOptionC")
        predicates.append(lambda: j["ShaderOptions"][2]["keyOffset"]    ==   8)
        predicates.append(lambda: j["ShaderOptions"][2]["keySize"]      ==   3)
        predicates.append(lambda: j["ShaderOptions"][2]["values"][0]    ==   "5")
        predicates.append(lambda: j["ShaderOptions"][2]["values"][1]    ==   "9")


        if not silent: print (fg.CYAN+ style.BRIGHT+ "input assembler layouts verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyFail(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--options"])
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

    if verifyOptionsEmission(os.path.join(workDir, "../Emission/Variants.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyOptionsEmissionIntegerRanges(os.path.join(workDir, "../Samples/VariantsRanges.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    # These tests will fail
    if verifyFail(os.path.join(workDir, "../Samples/AsError/int-range-bad-max.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyFail(os.path.join(workDir, "../Samples/AsError/int-range-bad-min.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyFail(os.path.join(workDir, "../Samples/AsError/int-range-no-args.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if verifyFail(os.path.join(workDir, "../Samples/AsError/int-range-swapped.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")
