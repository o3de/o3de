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

'''
Validates the functionality of the --strip-unused-srgs flag.
'''

def testUnusedSrgStripping(thefile, compilerPath, silent, aliveSrgs, deadSrgs):
    # First we compile as is, no srg stripping, and make sure all SRGs are present in the symbol output
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        if not silent: print (fg.CYAN+ style.BRIGHT+ "testUnusedSrgStripping: Verifying no srg was stripped..."+ style.RESET_ALL)
        predicates = []
        for srgName in aliveSrgs + deadSrgs:
            symbolExpr = "Symbol '{}'".format(srgName)
            predicates.append(lambda symbolExpr=symbolExpr: symbols[symbolExpr]['kind'] == 'ShaderResourceGroup')
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates)) + " testUnusedSrgStripping: Verified no srg was stripped."+ style.RESET_ALL)
    else:
        return False
    # Now we force unused srg removal and make sure that only @aliveSrgs are present and all @deadSrgs got removed.
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent, ["--strip-unused-srgs"])
    if ok:
        if not silent: print (fg.CYAN+ style.BRIGHT+ "testUnusedSrgStripping: Verifying srgs were stripped..."+ style.RESET_ALL)
        predicates = []
        for srgName in aliveSrgs:
            symbolExpr = "Symbol '{}'".format(srgName)
            predicates.append(lambda symbolExpr=symbolExpr: symbols[symbolExpr]['kind'] == 'ShaderResourceGroup')
        for srgName in deadSrgs:
            symbolExpr = "Symbol '{}'".format(srgName)
            predicates.append(lambda symbolExpr=symbolExpr: symbolExpr not in symbols)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates)) + " testUnusedSrgStripping: Verified srgs were stripped."+ style.RESET_ALL)
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

    if not silent: print ("testing for removal of two SRGs...")
    if testUnusedSrgStripping(os.path.join(workDir, "srg-strip-unused-SRG2-SRG3.azsl"), compiler, silent,
        aliveSrgs=["/SRG1"], deadSrgs=["/SRG2", "/SRG3"] ): result += 1
    else: resultFailed += 1
    
    if not silent: print ("testing for removal of one SRG...")
    if testUnusedSrgStripping(os.path.join(workDir, "srg-strip-unused-SRG3.azsl"), compiler, silent,
        aliveSrgs=["/SRG1", "/SRG2"], deadSrgs=["/SRG3"] ): result += 1
    else: resultFailed += 1
    
    if not silent: print ("testing for survival of all SRGs...")
    if testUnusedSrgStripping(os.path.join(workDir, "srg-strip-unused-none.azsl"), compiler, silent,
        aliveSrgs=["/SRG1", "/SRG2", "/SRG3"], deadSrgs=[] ): result += 1
    else: resultFailed += 1
    
    if not silent: print ("testing for removal of unused partial SRG...")
    if testUnusedSrgStripping(os.path.join(workDir, "srg-strip-unused-partial.azsl"), compiler, silent,
        aliveSrgs=["/MainSRG"], deadSrgs=["/PartialSRG1", "/CompleteSRG2"]  ): result += 1
    else: resultFailed += 1
    
    if not silent: print ("testing for survival of all SRGs because one of them has the fallback key...")
    if testUnusedSrgStripping(os.path.join(workDir, "srg-strip-unused-none-fallback.azsl"), compiler, silent,
        aliveSrgs=["/SRG1", "/SRG2", "/SRG3"], deadSrgs=[] ): result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")