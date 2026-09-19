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

'''  the test looks like the example below, so we want to verify symbols and their references
int func(int i) {..}  // 1

int func(float f) {..}  // 2

float4 main() : SV_Target0
{
    g_func(1);  // ref to 1
    g_func(1.5);  // ref to 2
'''

def execTest(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    # check the specific stuff we want to verify with this test
    if ok:
        try:
            ok = symbols["Symbol '/g_func'"]['kind'] == 'OverloadSet'
            ok = ok and symbols["Symbol '/g_func(?int)'"]['kind'] == 'Function'
            ok = ok and symbols["Symbol '/g_func(?float)'"]['kind'] == 'Function'

            ok = ok and symbols["Symbol '/g_func(?int)'"]['return type']['core']['name'] == '?int'

            ok = ok and symbols["Symbol '/g_func(?int)'"]['parameters'][0]["name"] == 'i'
            ok = ok and symbols["Symbol '/g_func(?int)'"]['parameters'][0]["type"]["core"]["name"] == '?int'
            ok = ok and symbols["Symbol '/g_func(?float)'"]['parameters'][0]["name"] == 'f'
            ok = ok and symbols["Symbol '/g_func(?float)'"]['parameters'][0]["type"]["core"]["name"] == '?float'

            ok = ok and symbols["Symbol '/g_func(?int)/i'"]['kind'] == 'Variable'
            ok = ok and symbols["Symbol '/g_func(?int)/i'"]['type']['core']['name'] == '?int'
            ok = ok and symbols["Symbol '/g_func(?float)/f'"]['kind'] == 'Variable'
            ok = ok and symbols["Symbol '/g_func(?float)/f'"]['type']['core']['name'] == '?float'

            ok = ok and len(symbols["Symbol '/f'"]['references']) == 1  # one unsolved ref, because f()
            ok = ok and len(symbols["Symbol '/f(?int,?float)'"]['references']) == 2  # one decl site. one call site
            ok = ok and len(symbols["Symbol '/f(?int)'"]['references']) == 2  # one decl site. one call site

            ok = ok and symbols["Symbol '/f(?int,?float)'"]['references'][1]['line'] == 28
            ok = ok and symbols["Symbol '/f(?int)'"]['references'][1]['line'] == 27
            ok = ok and symbols["Symbol '/f'"]['references'][0]['line'] == 26

            if not ok:
                print (style.DIM + fg.YELLOW + "ERR: all expected symbol founds, but their semantic understanding seems off" + style.RESET_ALL)
            else:
                print (style.BRIGHT + "OK! all symbols semantics correctly understood" + style.RESET_ALL)
        except Exception as e:
            print (fg.RED + "Err: Parsed --dumpsym may lack some expected keys" + style.RESET_ALL, e)
    return 1 if ok else 0

result = 0  # to define for sub-tests
resultFailed = 0
def doTests(compiler, silent, azdxcpath):
    global result
    global resultFailed

    # Working directory should have been set to this script's directory by the calling parent
    # You can get it once doTests() is called, but not during initialization of the module,
    #  because at that time it will still be set to the working directory of the calling script
    workDir = os.getcwd()

    if execTest(os.path.join(workDir, "function-overloading.azsl"), compiler, silent): result += 1
    else: resultFailed += 1


if __name__ == "__main__":
    print ("please call from testapp.py")
