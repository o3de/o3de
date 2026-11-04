#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import sys
import os
from clr import *
sys.path.append("..")
import testfuncs

def execTest(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    # check the specific stuff we want to verify with this test
    if ok:
        try:
            ok = symbols["Symbol '/A/A'"]['kind'] == 'Class'
            
            ok2 = symbols["Symbol '/A/A'"]["references"] == None   # no references for A::A
            
            ok3 = len(symbols["Symbol '/A'"]["references"]) == 1

            ok4 = symbols["Symbol '/A'"]["references"][0]["line"] == 11  # /A has one ref in the baselist of B line 11

            if not silent:
                if not ok:
                    print (fg.RED+ "Couldn't verify symbol /A/A is a Class"+ style.RESET_ALL)

                if not ok2:
                    print (fg.RED+ "Couldn't verify symbol /A/A is ot referenced"+ style.RESET_ALL)

                if not ok3:
                    print (fg.RED+ "Couldn't verify /A has 1 reference"+ style.RESET_ALL)

                if not ok4:
                    print (fg.RED+ "Couldn't verify /A's seenat is at line 11"+ style.RESET_ALL)

            ok = ok and ok2 and ok3 and ok4

        except Exception as e:
            print (fg.RED+ "Err: dumpsym didn't match expectations"+ style.RESET_ALL, e)
            ok = False
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

    if execTest(os.path.join(workDir, "inheritance-lookup-seenats.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")
