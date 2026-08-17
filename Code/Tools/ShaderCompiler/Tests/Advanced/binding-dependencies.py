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

def verifyBindingDependencies(thefile, compilerPath, silent):
    j, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--bindingdep"])

    if ok:
        predicates = []

        predicates.append(lambda: "MainPS" in j["Srg1"]["Srg1_SRGConstantBuffer"]["dependentFunctions"])
        predicates.append(lambda: "MainVS" in j["Srg1"]["Srg1_SRGConstantBuffer"]["dependentFunctions"])
        predicates.append(lambda: "m_meshDisplacement" in j["Srg1"]["Srg1_SRGConstantBuffer"]["participantConstants"])

        predicates.append(lambda: "MainPS" in j["Srg1"]["m_environmentMap"]["dependentFunctions"])

        predicates.append(lambda: "MainPS" in j["Srg1"]["m_extendedMaterials"]["dependentFunctions"])
        predicates.append(lambda: "MainVS" in j["Srg1"]["m_extendedMaterials"]["dependentFunctions"])

        predicates.append(lambda: "MainPS" in j["Srg1"]["m_materialConstants"]["dependentFunctions"])

        predicates.append(lambda: "MainPS" in j["Srg1"]["m_sampler1"]["dependentFunctions"])

        predicates.append(lambda: len(j["Srg1"]["m_sampler2"]["dependentFunctions"]) == 0)

        predicates.append(lambda: "MainVS" in j["Srg2"]["Srg2_SRGConstantBuffer"]["dependentFunctions"])
        predicates.append(lambda: "m_inverseTranspose" in j["Srg2"]["Srg2_SRGConstantBuffer"]["participantConstants"])
        predicates.append(lambda: "m_world" in j["Srg2"]["Srg2_SRGConstantBuffer"]["participantConstants"])

        predicates.append(lambda: "MainPS" in j["Srg2"]["m_IBLsampler"]["dependentFunctions"])

        predicates.append(lambda: "MainPS" in j["Srg2"]["m_diffuseIBL"]["dependentFunctions"])

        predicates.append(lambda: "over" not in j["Srg1"]["m_materialConstants"]["dependentFunctions"])

        if not silent: print (fg.CYAN+ style.BRIGHT+ "binding dependency analysis verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, j)
    return True if ok else False

def verifyBindingDependencies_2(thefile, compilerPath, silent):
    symbols, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--bindingdep"])

    if ok:
        predicates = []
        # this test is mostly to verify that the analysis doesn't crash altogether.
        # this is a regression test, since we had a build where that shader crashed the --bindingdep build.
        predicates.append(lambda: "StandardPbr_ForwardPassPS" in symbols["MaterialSrg"]["MaterialSrg_SRGConstantBuffer"]["dependentFunctions"])

        if not silent: print (fg.CYAN+ style.BRIGHT+ "complex input program binding dep verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " verified."+ style.RESET_ALL)

    return 1 if ok else 0

def verifyBindingDependencies_3(thefile, compilerPath, silent):
    symbols, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--bindingdep"])

    if ok:
        predicates = []
        # this is a regression test to make sure we can analyze fully the dependencies when we are in the
        # presence of variant options. because options have a fallback in one of the SRG.
        # and tracking the use of options to the fallback is its own code path
        # every increase of the degree of cyclomatic complexity mandates its own test.
        predicates.append(lambda: "MainPS" in symbols["TrianglePerInstanceSRG"]["TrianglePerInstanceSRG_SRGConstantBuffer"]["dependentFunctions"])
        predicates.append(lambda: "MainVS" in symbols["TrianglePerInstanceSRG"]["TrianglePerInstanceSRG_SRGConstantBuffer"]["dependentFunctions"])
        predicates.append(lambda: "m_objectMatrix" in symbols["TrianglePerInstanceSRG"]["TrianglePerInstanceSRG_SRGConstantBuffer"]["participantConstants"])
        predicates.append(lambda: "m_SHADER_VARIANT_KEY_NAME_" in symbols["TrianglePerInstanceSRG"]["TrianglePerInstanceSRG_SRGConstantBuffer"]["participantConstants"])

        if not silent: print (fg.CYAN+ style.BRIGHT+ "Program with variant fallback binding dep verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " verified."+ style.RESET_ALL)

    return 1 if ok else 0

def verifyBindingDependencies_4(thefile, compilerPath, silent):
    symbols, ok = testfuncs.buildAndGetJson(thefile, compilerPath, silent, ["--bindingdep"])

    if ok:
        predicates = []
        # this is a regression test to make sure the compiler doesn't crash on variables
        # internal to functions but declared after an unnamed scope.
        predicates.append(lambda: "MainCS" in symbols["PassSrg"]["PassSrg_SRGConstantBuffer"]["dependentFunctions"])
        predicates.append(lambda: "MainCS" in symbols["PassSrg"]["m_lutTexture"]["dependentFunctions"])

        if not silent: print (fg.CYAN+ style.BRIGHT+ "Program with unnamed scopes binding dep verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " verified."+ style.RESET_ALL)

    return 1 if ok else 0


result = 0  # to define for sub-tests
resultFailed = 0 # to define for sub-tests
def doTests(compiler, silent, az3rdParty):
    global result
    global resultFailed

    # Working directory should have been set to this script's directory by the calling parent
    # You can get it once doTests() is called, but not during initialization of the module,
    #  because at that time it will still be set to the working directory of the calling script
    workDir = os.getcwd()

    if verifyBindingDependencies(os.path.join(workDir, "entry-dependencies.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if result and verifyBindingDependencies_2(os.path.join(workDir, "../Semantic/standardpbr_forwardpass.azsl"), compiler, silent)  : result += 1
    else: resultFailed += 1

    if result and verifyBindingDependencies_3(os.path.join(workDir, "../Semantic/Triangle.azsl"), compiler, silent) : result += 1
    else: resultFailed += 1

    if result and verifyBindingDependencies_4(os.path.join(workDir, "BakeAcesOutputTransformLutCS.azslin"), compiler, silent) : result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")
