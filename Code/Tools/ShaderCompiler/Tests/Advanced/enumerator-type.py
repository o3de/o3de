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

def CheckSymbol(table, sym, field, value):
    if not table[sym]['type']['core'][field] == value:
        print (fg.RED+ sym + " must be of type "+ value + style.RESET_ALL)
        return False
    return True

def execTest(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if not ok:
        print (fg.RED+ "Couldn't get meaningful symbols table"+ style.RESET_ALL)
        return 0

    if not symbols["Symbol '/Monday'"]['type']['core']['name'] == '/Weekday':
        ok = False
        print (fg.RED+ "Symbol /Monday must be of type '/Weekday'"+ style.RESET_ALL)
    if not symbols["Symbol '/Monday'"]['type']['core']['underlying_scalar'] == '<NA>':
        ok = False
        print (fg.RED+ "Symbol /Monday must have underlying_scalar of type '<NA>'"+ style.RESET_ALL)

    ok = ok and CheckSymbol(symbols, "Symbol '/Monday'", 'name', '/Weekday')
    ok = ok and CheckSymbol(symbols, "Symbol '/Monday'", 'underlying_scalar', '<NA>')
    ok = ok and CheckSymbol(symbols, "Symbol '/Tuesday'", 'name', '/Weekday')
    ok = ok and CheckSymbol(symbols, "Symbol '/Tuesday'", 'underlying_scalar', '<NA>')
    ok = ok and CheckSymbol(symbols, "Symbol '/Wednesday'", 'name', '/Weekday')
    ok = ok and CheckSymbol(symbols, "Symbol '/Wednesday'", 'underlying_scalar', '<NA>')
    ok = ok and CheckSymbol(symbols, "Symbol '/Thursday'", 'name', '/Weekday')
    ok = ok and CheckSymbol(symbols, "Symbol '/Thursday'", 'underlying_scalar', '<NA>')
    ok = ok and CheckSymbol(symbols, "Symbol '/Friday'", 'name', '/Weekday')
    ok = ok and CheckSymbol(symbols, "Symbol '/Friday'", 'underlying_scalar', '<NA>')
    ok = ok and CheckSymbol(symbols, "Symbol '/Saturday'", 'name', '/Weekday')
    ok = ok and CheckSymbol(symbols, "Symbol '/Saturday'", 'underlying_scalar', '<NA>')
    ok = ok and CheckSymbol(symbols, "Symbol '/Sunday'", 'name', '/Weekday')
    ok = ok and CheckSymbol(symbols, "Symbol '/Sunday'", 'underlying_scalar', '<NA>')

    ok = ok and CheckSymbol(symbols, "Symbol '/FavouriteDay'", 'name', '/Weekday')
    ok = ok and CheckSymbol(symbols, "Symbol '/FavouriteDay'", 'underlying_scalar', '<NA>')

    ok = ok and CheckSymbol(symbols, "Symbol '/Season/Spring'", 'name', '/Season')
    ok = ok and CheckSymbol(symbols, "Symbol '/Season/Spring'", 'underlying_scalar', '<NA>')
    ok = ok and CheckSymbol(symbols, "Symbol '/Season/Summer'", 'name', '/Season')
    ok = ok and CheckSymbol(symbols, "Symbol '/Season/Summer'", 'underlying_scalar', '<NA>')
    ok = ok and CheckSymbol(symbols, "Symbol '/Season/Autumn'", 'name', '/Season')
    ok = ok and CheckSymbol(symbols, "Symbol '/Season/Autumn'", 'underlying_scalar', '<NA>')
    ok = ok and CheckSymbol(symbols, "Symbol '/Season/Winter'", 'name', '/Season')
    ok = ok and CheckSymbol(symbols, "Symbol '/Season/Winter'", 'underlying_scalar', '<NA>')

    ok = ok and CheckSymbol(symbols, "Symbol '/IsComing'", 'name', '/Season')
    ok = ok and CheckSymbol(symbols, "Symbol '/IsComing'", 'underlying_scalar', '<NA>')


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

    if execTest(os.path.join(workDir, "../Samples/Enumeration.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")