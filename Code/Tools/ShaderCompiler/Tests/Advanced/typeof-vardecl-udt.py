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

def test(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        try:
            ok = symbols["Symbol '/g_s'"]['kind'] == 'Variable'
            ok = ok and symbols["Symbol '/g_s'"]['type']['core']['name'] == '/S'

            if not silent:
                if not ok:
                    print (fg.RED+ "ERR: g_s type could not be validated"+ style.RESET_ALL)
                else:
                    print (style.BRIGHT+ "OK! "+ style.RESET_ALL)
        except Exception as e:
            print (fg.RED+ "Err: Parsed --dumpsym may lack some expected keys"+ style.RESET_ALL, e)
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

    if test(os.path.join(workDir, "../Semantic/combined-vardecl-udt.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")
