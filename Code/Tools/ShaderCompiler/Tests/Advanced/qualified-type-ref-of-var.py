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
            ok = symbols["Symbol '/func()/d'"]['type']['core']['name'] == '/C/D'

            ok2 = symbols["Symbol '/func()/d2'"]['type']['core']['name'] == '/C/D'

            ok3 = symbols["Symbol '/func()/d3'"]['type']['core']['name'] == '/func()/S'

            mh34Type = symbols["Symbol '/func()/mh34'"]['type']['core']['name']

            rwbfType = symbols["Symbol '/func()/rwbf'"]['type']

            sbInlType = symbols["Symbol '/func()/sbInl'"]['type']

            ok4 = True # TODO (wip)

            ok5 = rwbfType['core']['name'] == "?RWBuffer" and rwbfType['generic']['name'] == "?float4x4"

            ok6 = sbInlType['core']['name'] == "?StructuredBuffer" and sbInlType['generic']['name'] == "/func()/Inl" and sbInlType['generic']['tclass'] == "Struct"

            if not silent:
                if not ok:
                    print (fg.RED+ "Couldn't verify symbol /C/D is type of /func()/d"+ style.RESET_ALL)
                else:
                    print (style.BRIGHT+ "/C/D is type of /func()/d. great !"+ style.RESET_ALL)
                if not ok2:
                    print (fg.RED+ "Couldn't verify symbol /C/D is type of /func()/d2"+ style.RESET_ALL)
                else:
                    print (style.BRIGHT+ "/C/D is type of /func()/d2. great !"+ style.RESET_ALL)
                if not ok3:
                    print (fg.RED+ "Couldn't verify symbol /func()/d3 is of type /func()/S"+ style.RESET_ALL)
                else:
                    print (style.BRIGHT+ "/func()/d3 is of type /func()/S. great !"+ style.RESET_ALL)

                print (style.BRIGHT+ fg.YELLOW+ ("WIP: need to verify {0} is canonicalized to half3x4 ?").format(mh34Type)+ style.RESET_ALL)

                if not ok5:
                    print (fg.RED+ "Couldn't verify symbol /func()/rwbf is of type ?RWBuffer<?float4x4>"+ style.RESET_ALL)
                else:
                    print (style.BRIGHT+ "/func()/rwbf is of type ?RWBuffer<?float4x4>. great !"+ style.RESET_ALL)

                if not ok6:
                    print (fg.RED+ "Couldn't verify symbol /func()/sbInl is of type ?StructuredBuffer< struct /func()/Inl >"+ style.RESET_ALL)
                else:
                    print (style.BRIGHT+ "/func()/sbInl is of type ?StructuredBuffer< struct /fubc/Inl >. great !"+ style.RESET_ALL)


            ok = ok and ok2 and ok3 and ok4 and ok5 and ok6

        except Exception as e:
            print (fg.RED+ "Err: Parsed --dumpsym dictionary didn't record /func()/d symbol ?"+ style.RESET_ALL, e)
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

    if execTest(os.path.join(workDir, "qualified-type-ref-of-var.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")
