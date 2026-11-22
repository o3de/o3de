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

def testVariables(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        predicates = []
        # check that there is global variable `a`
        predicates.append(lambda: symbols["Symbol '/a'"]['kind'] == 'Variable')
        predicates.append(lambda: symbols["Symbol '/a'"]['type']['core']['name'] == '?int')
        # and its only reference is at line 41
        predicates.append(lambda: len(symbols["Symbol '/a'"]['references']) == 1)
        predicates.append(lambda: symbols["Symbol '/a'"]['references'][0]['line'] == 42)  # .. a + b;

        # now check global var `b`
        predicates.append(lambda: symbols["Symbol '/b'"]['kind'] == 'Variable')
        predicates.append(lambda: symbols["Symbol '/b'"]['type']['core']['name'] == '?int')
        predicates.append(lambda: set(symbols["Symbol '/b'"]['storage'].split()) == set(['static', 'const']))
        # it appears in 2 places:
        predicates.append(lambda: len(symbols["Symbol '/b'"]['references']) == 2)
        predicates.append(lambda: symbols["Symbol '/b'"]['references'][0]['line'] == 10)  # c = b;
        predicates.append(lambda: symbols["Symbol '/b'"]['references'][1]['line'] == 42)  # .. a + b;

        # check member variable `c`
        predicates.append(lambda: len(symbols["Symbol '/S/c'"]['references']) == 5)
        predicates.append(lambda: symbols["Symbol '/S/c'"]['references'][0]['line'] == 10) # c = b;
        predicates.append(lambda: symbols["Symbol '/S/c'"]['references'][1]['line'] == 11) # return c;
        predicates.append(lambda: symbols["Symbol '/S/c'"]['references'][2]['line'] == 30) # param.c;
        predicates.append(lambda: symbols["Symbol '/S/c'"]['references'][3]['line'] == 31) # param.c (first appearance)
        predicates.append(lambda: symbols["Symbol '/S/c'"]['references'][4]['line'] == 31) # param.c; (last appearance)

        # check deep nested `d`
        predicates.append(lambda: len(symbols["Symbol '/S/N/NN/NNN/d'"]['references']) == 2)
        predicates.append(lambda: symbols["Symbol '/S/N/NN/NNN/d'"]['references'][0]['line'] == 44) # the 's' in `if (s.n.nn.nnn.d)`
        predicates.append(lambda: symbols["Symbol '/S/N/NN/NNN/d'"]['references'][1]['line'] == 50) # on the right of assignment

        # check refs to global `s` of UDT
        predicates.append(lambda: symbols["Symbol '/s'"]['references'][0]['line'] == 36)  # ref to ::s as passed to func(s)
        predicates.append(lambda: symbols["Symbol '/s'"]['references'][1]['line'] == 44)  # ref to ::s in the s.n.nn... expression
        predicates.append(lambda: symbols["Symbol '/s'"]['references'][2]['line'] == 48)  # ref to ::s in assignment RHS

        # check ref to local var `c`
        predicates.append(lambda: symbols["Symbol '/main()/c'"]['references'][0]['line'] == 41)  # first assignment (LHS)

        # check refs to s1: (local var of type sampler)()
        predicates.append(lambda: symbols["Symbol '/main()/s1'"]['references'][0]['line'] == 53)
        predicates.append(lambda: symbols["Symbol '/main()/s1'"]['references'][0]['col'] == 33)

        # check refs to s2: (local var of type sampler)()
        predicates.append(lambda: symbols["Symbol '/main()/s2'"]['references'][0]['line'] == 57)
        predicates.append(lambda: symbols["Symbol '/main()/s2'"]['references'][0]['col'] == 42)

        # check refs to tex in the if scope
        predicates.append(lambda: symbols["Symbol '/main()/$bk1/tex'"]['references'][0]['line'] == 53)
        predicates.append(lambda: symbols["Symbol '/main()/$bk1/tex'"]['references'][0]['col'] == 21)

        # check refs to tex after the if scope
        predicates.append(lambda: symbols["Symbol '/main()/tex'"]['references'][0]['line'] == 57)
        predicates.append(lambda: symbols["Symbol '/main()/tex'"]['references'][0]['col'] == 31)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "variable references verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates)) + " variable references predicates verified."+ style.RESET_ALL)

    return ok

def testFunctions(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        predicates = []
        # check all references of func()
        predicates.append(lambda: symbols["Symbol '/func()'"]['kind'] == 'Function')
        predicates.append(lambda: symbols["Symbol '/func()'"]['references'][0]['line'] == 1)  # first declaration
        predicates.append(lambda: symbols["Symbol '/func()'"]['references'][1]['line'] == 5)  # call in psmain
        predicates.append(lambda: symbols["Symbol '/func()'"]['references'][2]['line'] == 14)  # re-declaration
        predicates.append(lambda: symbols["Symbol '/func()'"]['references'][3]['line'] == 25)  # in typeof expression
        predicates.append(lambda: symbols["Symbol '/func()'"]['references'][4]['line'] == 26)  # in switch condition expression
        predicates.append(lambda: symbols["Symbol '/func()'"]['references'][5]['line'] == 29)  # call in assignment
        predicates.append(lambda: symbols["Symbol '/func()'"]['references'][6]['line'] == 35)  # ref 7 in constructor

        # note that "nope" cases didn't appear as false positive references, otherwise they would have
        # disrupted the index.

        if not silent: print (fg.CYAN+ style.BRIGHT+ "functions references verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " function references predicates verified."+ style.RESET_ALL)
    return ok

def testMethods(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        predicates = []
        # let's verify that there is a free function with name f at global scope
        predicates.append(lambda: symbols["Symbol '/f(?int)'"]['kind'] == 'Function')  # this is a symbol meant to lead the tool to possible confusion
        predicates.append(lambda: symbols["Symbol '/f(?int)'"]['line'] == 1)
        predicates.append(lambda: symbols["Symbol '/_(?int)'"]['kind'] == 'Function')  # just there test function call expressions with minimal visual pollution
        predicates.append(lambda: symbols["Symbol '/_(?int)'"]['line'] == 2)
        # setup ok, let's see the references to the Parent/f family
        predicates.append(lambda: symbols["Symbol '/Parent/f(?int)'"]['kind'] == 'Function')
        predicates.append(lambda: len(symbols["Symbol '/Parent/f(?int)'"]['references']) <= 1) # one ref to itself max. since you can't call it directly.
        predicates.append(lambda: symbols["Symbol '/Parent/f(?int)'"]["has overriding children"][0]["name"] == "/Child/f(?int)") # one child does override
        # let's check the child also back-refers to this parent:
        predicates.append(lambda: symbols["Symbol '/Child/f(?int)'"]["is hiding base symbol"] == "/Parent/f(?int)")
        # let's checkout the references of this child:
        appearanceLines = [16, 21, 23, 24, 33, 34, 37] # as per comments in the file ref, ref2, ref3...
        for ii, line in enumerate(appearanceLines):
            predicates.append(lambda ii=ii, line=line: symbols["Symbol '/Child/f(?int)'"]['references'][ii]['line'] == line)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "methods references verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " methods references predicates verified."+ style.RESET_ALL)
    return ok

def testStructs(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        predicates = []
        appearanceLines = [7, 12, 13, 15, 20, 29, 36, 40] # as per comments in the file ref, ref2, ref3...
        for ii, line in enumerate(appearanceLines):
            predicates.append(lambda ii=ii, line=line: symbols["Symbol '/S'"]['references'][ii]['line'] == line)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "structs references verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " structs references predicates verified."+ style.RESET_ALL)

    return ok

def testSRGs(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        predicates = []
        # let's check MySRG
        appearances = [(24,5), (29,5), (29,24), (30,12), (31,14), (33,15), (35,17), (39,48), (41,19), (41,41)] # line:col
        for ii, lineCol in enumerate(appearances):
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/MySRG'"]['references'][ii]['line'] == lineCol[0])
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/MySRG'"]['references'][ii]['col'] == lineCol[1])

        # let's check Inner
        appearances = [(17,22), (24,12), (29,12), (30,19), (31,23)]
        for ii, lineCol in enumerate(appearances):
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/MySRG/Inner'"]['references'][ii]['line'] == lineCol[0])
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/MySRG/Inner'"]['references'][ii]['col'] == lineCol[1])

        # let's check m_mat
        appearances = [(30,26), (30,43), (41,57)]
        for ii, lineCol in enumerate(appearances):
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/MySRG/Inner/m_mat'"]['references'][ii]['line'] == lineCol[0])
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/MySRG/Inner/m_mat'"]['references'][ii]['col'] == lineCol[1])

        # check one appearance of Deep
        predicates.append(lambda: symbols["Symbol '/MySRG/Inner/Deep'"]['references'][0]['line'] == 32)
        predicates.append(lambda: symbols["Symbol '/MySRG/Inner/Deep'"]['references'][0]['col'] == 21)

        # check m_h (as a leaf)
        predicates.append(lambda: symbols["Symbol '/MySRG/Inner/Deep/m_h'"]['references'][0]['line'] == 32)
        predicates.append(lambda: symbols["Symbol '/MySRG/Inner/Deep/m_h'"]['references'][0]['col'] == 29)
        predicates.append(lambda: symbols["Symbol '/MySRG/Inner/Deep/m_h'"]['references'][1]['line'] == 32)
        predicates.append(lambda: symbols["Symbol '/MySRG/Inner/Deep/m_h'"]['references'][1]['col'] == 55)

        # check m_deep (as an instance declared from a type grammar rule)
        predicates.append(lambda: symbols["Symbol '/MySRG/Inner/m_deep'"]['references'][0]['line'] == 32)
        predicates.append(lambda: symbols["Symbol '/MySRG/Inner/m_deep'"]['references'][0]['col'] == 48)

        # check m_stb as a special type of data (structured buffer)
        predicates.append(lambda: symbols["Symbol '/MySRG/m_stb'"]['references'][0]['line'] == 29)
        predicates.append(lambda: symbols["Symbol '/MySRG/m_stb'"]['references'][0]['col'] == 31)

        predicates.append(lambda: symbols["Symbol '/MySRG/m_stb'"]['references'][1]['line'] == 41)
        predicates.append(lambda: symbols["Symbol '/MySRG/m_stb'"]['references'][1]['col'] == 48)

        # check ref to SData as a local variable type
        predicates.append(lambda: symbols["Symbol '/SData'"]['references'][0]['line'] == 36)
        predicates.append(lambda: symbols["Symbol '/SData'"]['references'][0]['col'] == 5)

        # check sampler ref line 34
        predicates.append(lambda: symbols["Symbol '/MySRG/m_sampler'"]['references'][0]['line'] == 35)

        # ref to d, clr, s
        predicates.append(lambda: symbols["Symbol '/main()/d'"]['references'][0]['line'] == 37)
        predicates.append(lambda: symbols["Symbol '/SData/clr'"]['references'][0]['line'] == 37)
        predicates.append(lambda: symbols["Symbol '/main()/s'"]['references'][0]['line'] == 37)

        # ref to f at the end
        predicates.append(lambda: symbols["Symbol '/main()/f'"]['references'][0]['line'] == 41)
        predicates.append(lambda: symbols["Symbol '/main()/f'"]['references'][0]['col'] == 31)

        # worldmatrix
        predicates.append(lambda: symbols["Symbol '/MySRG/m_worldMatrix'"]['references'][0]['line'] == 39)
        predicates.append(lambda: symbols["Symbol '/MySRG/m_worldMatrix'"]['references'][0]['col'] == 55)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "SRG references verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " SRG references predicates verified."+ style.RESET_ALL)
    return ok

def testCBs(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:

        predicates = []
        # let's check that m_diffuseColor appears in the good places
        appearances = [(17,34), (26,48), (43,49), (45,38), (51,18)] # line:col
        for ii, lineCol in enumerate(appearances):
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/MySRGOne/InnerStruct/m_diffuseColor'"]['references'][ii]['line'] == lineCol[0])
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/MySRGOne/InnerStruct/m_diffuseColor'"]['references'][ii]['col'] == lineCol[1])

        # let's check the real materialConstants references
        appearances = [(17,16), (26,30), (42,29), (43,31), (44,49)] # line:col
        for ii, lineCol in enumerate(appearances):
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/MySRGOne/materialConstants'"]['references'][ii]['line'] == lineCol[0])
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/MySRGOne/materialConstants'"]['references'][ii]['col'] == lineCol[1])

        # let's check the global decoy main/materialConstants references
        appearances = [(41,5), (45,20), (48,17)] # line:col
        for ii, lineCol in enumerate(appearances):
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/materialConstants'"]['references'][ii]['line'] == lineCol[0])
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/materialConstants'"]['references'][ii]['col'] == lineCol[1])

        # let's check the local decoy main/materialConstants references
        appearances = [(47,19), (49,24)] # line:col
        for ii, lineCol in enumerate(appearances):
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/main(?float2)/MySRGOne/materialConstants'"]['references'][ii]['line'] == lineCol[0])
            predicates.append(lambda ii=ii, lineCol=lineCol: symbols["Symbol '/main(?float2)/MySRGOne/materialConstants'"]['references'][ii]['col'] == lineCol[1])

        if not silent: print (fg.CYAN+ style.BRIGHT+ "CB references verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " CB references verified."+ style.RESET_ALL)

    return ok

# test Member Access Expression
def testMAE(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        predicates = []
        predicates.append(lambda: symbols["Symbol '/A'"]['references'][0]['line'] == 8)
        predicates.append(lambda: symbols["Symbol '/A'"]['references'][0]['col'] == 5)
        predicates.append(lambda: symbols["Symbol '/A'"]['references'][1]['line'] == 9)
        predicates.append(lambda: symbols["Symbol '/A'"]['references'][1]['col'] == 7)

        if not silent: print (fg.CYAN+ style.BRIGHT+ "qualified-id verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " qualified-id verified."+ style.RESET_ALL)

    return ok

# test Deported Method definition test file
def testDeported(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        predicates = []
        # check *
        predicates.append(lambda: symbols["Symbol '/Random/Next()'"]['line'] == 20)
        predicates.append(lambda: symbols["Symbol '/Random/Next()'"]['def line'] == 34)
        predicates.append(lambda: symbols["Symbol '/Random/Next()'"]['references'][0]['line'] == 20)
        predicates.append(lambda: symbols["Symbol '/Random/Next()'"]['references'][1]['line'] == 29)
        # check **
        predicates.append(lambda: symbols["Symbol '/Random/cur'"]['line'] == 23)
        predicates.append(lambda: symbols["Symbol '/Random/cur'"]['references'][0]['line'] == 28)
        predicates.append(lambda: symbols["Symbol '/Random/cur'"]['references'][1]['line'] == 38)
        # check ***
        predicates.append(lambda: symbols["Symbol '/Random/Init()'"]['line'] == 26)
        predicates.append(lambda: symbols["Symbol '/Random/Init()'"]['def line'] == 26)
        predicates.append(lambda: symbols["Symbol '/Random/Init()'"]['references'][0]['line'] == 37)
        predicates.append(lambda: symbols["Symbol '/Random/Init()'"]['references'][1]['line'] == 45)

        if not silent: print (fg.CYAN+ style.BRIGHT+ " deported-methods verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " deported-methods verified."+ style.RESET_ALL)

    return ok


# test typealias is registered and doesn't break the reference chain of other user defined types
def testTypealias(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        predicates = []
        # check for the understanding of the access of the stuff variable through indirect structuredbuffer access
        predicates.append(lambda: symbols["Symbol '/PassVars/stuff'"]['references'][0]['line'] == 15)
        # check for registration of the alias itself
        predicates.append(lambda: symbols["Symbol '/StructBuf'"]['kind'] == 'TypeAlias')
        predicates.append(lambda: symbols["Symbol '/StructBuf'"]['canonical type']['generic']['name'] == '/PassVars')

        if not silent: print (fg.CYAN+ style.BRIGHT+ " alias predicates verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " alias predicates verified."+ style.RESET_ALL)

    return ok

def testFunctionOverloads(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        predicates = []

        predicates.append(lambda: symbols["Symbol '/MySRG/Metal'"]['kind'] == "Struct")

        predicates.append(lambda: symbols["Symbol '/MySRG/Barrel'"]['kind'] == "Struct")
        predicates.append(lambda: symbols["Symbol '/MySRG/Barrel'"]['kind'] == "Struct")

        predicates.append(lambda: symbols["Symbol '/MySRG/MakeMat(?float)'"]['kind'] == "Function")
        predicates.append(lambda: symbols["Symbol '/MySRG/MakeMat(?float)'"]['references'][0]['line'] == 53)
        predicates.append(lambda: symbols["Symbol '/MySRG/MakeMat(?float)'"]['references'][1]['line'] == 58)

        predicates.append(lambda: symbols["Symbol '/MySRG/MakeMat'"]['kind'] == 'OverloadSet')
        predicates.append(lambda: symbols["Symbol '/MySRG/MakeMat'"]['functions'][0] == '/MySRG/MakeMat(/MySRG/Metal)')
        predicates.append(lambda: symbols["Symbol '/MySRG/MakeMat'"]['functions'][1] == '/MySRG/MakeMat(?float)')
        predicates.append(lambda: not symbols["Symbol '/MySRG/MakeMat'"]['references'])

        predicates.append(lambda: symbols["Symbol '/MySRG/MakeMat(/MySRG/Metal)'"]['references'][0]['line'] == 55)

        # because of use of * multiply, azslc can't resolve the overload, so the set holds the reference
        predicates.append(lambda: symbols["Symbol '/MySRG/Luminosity'"]['references'][0]['line'] == 63)

        if not silent: print (fg.CYAN+ style.BRIGHT+ " overload predicates verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " overload predicates verified."+ style.RESET_ALL)

    return ok

def testUnnamedBlocks(thefile, compilerPath, silent):
    '''return number of successes'''
    symbols, ok = testfuncs.buildAndGetSymbols(thefile, compilerPath, silent)
    if ok:
        predicates = []

        predicates.append(lambda: symbols["Symbol '/S'"]['kind'] == "Class")

        predicates.append(lambda: symbols["Symbol '/S/i'"]['kind'] == "Variable")
        predicates.append(lambda: symbols["Symbol '/S/i'"]['references'][0]['line'] == 21)
        predicates.append(lambda: symbols["Symbol '/S/i'"]['references'][1]['line'] == 25)
        predicates.append(lambda: symbols["Symbol '/S/i'"]['references'][2]['line'] == 69)
        predicates.append(lambda: symbols["Symbol '/S/i'"]['references'][3]['line'] == 72)

        predicates.append(lambda: symbols["Symbol '/i'"]['kind'] == "Variable")
        predicates.append(lambda: symbols["Symbol '/i'"]['references'][0]['line'] == 16)
        predicates.append(lambda: symbols["Symbol '/i'"]['references'][1]['line'] == 18)
        predicates.append(lambda: symbols["Symbol '/i'"]['references'][2]['line'] == 29)
        predicates.append(lambda: symbols["Symbol '/i'"]['references'][3]['line'] == 70)

        predicates.append(lambda: symbols["Symbol '/f()/$bk0/i'"]['type']['core']['name'] == '?int')
        predicates.append(lambda: symbols["Symbol '/f()/$bk0/i'"]['line'] == 14)
        predicates.append(lambda: symbols["Symbol '/f()/$bk0/i'"]['references'][0]['line'] == 15)

        predicates.append(lambda: symbols["Symbol '/f()/i'"]['type']['core']['name'] == '?int')
        predicates.append(lambda: symbols["Symbol '/f()/i'"]['line'] == 23)
        predicates.append(lambda: symbols["Symbol '/f()/i'"]['references'][0]['line'] == 28)
        predicates.append(lambda: symbols["Symbol '/f()/i'"]['references'][1]['line'] == 58)

        predicates.append(lambda: symbols["Symbol '/f()/$sw1/$bk2/i'"]['line'] == 33)
        predicates.append(lambda: symbols["Symbol '/f()/$sw1/$bk2/i'"]['references'][0]['line'] == 34)

        predicates.append(lambda: symbols["Symbol '/f()/$sw1/i'"]['line'] == 37)
        predicates.append(lambda: symbols["Symbol '/f()/$sw1/i'"]['references'][0]['line'] == 40)

        predicates.append(lambda: symbols["Symbol '/f()/$for3/i'"]['line'] == 43)
        predicates.append(lambda: symbols["Symbol '/f()/$for3/i'"]['references'][0]['line'] == 43)
        predicates.append(lambda: symbols["Symbol '/f()/$for3/i'"]['references'][1]['line'] == 43)
        predicates.append(lambda: symbols["Symbol '/f()/$for3/i'"]['references'][2]['line'] == 45)
        predicates.append(lambda: symbols["Symbol '/f()/$for3/i'"]['references'][3]['line'] == 45)

        predicates.append(lambda: symbols["Symbol '/f()/$bk4/i'"]['line'] == 50)
        predicates.append(lambda: symbols["Symbol '/f()/$bk4/i'"]['references'][0]['line'] == 51)
        predicates.append(lambda: symbols["Symbol '/f()/$bk4/i'"]['references'][1]['line'] == 51)

        predicates.append(lambda: symbols["Symbol '/f()/$bk5/i'"]['line'] == 56)
        predicates.append(lambda: symbols["Symbol '/f()/$bk5/i'"]['references'][0]['line'] == 57)
        predicates.append(lambda: symbols["Symbol '/f()/$bk5/i'"]['references'][1]['line'] == 57)

        predicates.append(lambda: symbols["Symbol '/f()/$for6/i'"]['line'] == 60)
        predicates.append(lambda: symbols["Symbol '/f()/$for6/i'"]['references'][0]['line'] == 60)
        predicates.append(lambda: symbols["Symbol '/f()/$for6/i'"]['references'][1]['line'] == 60)

        predicates.append(lambda: symbols["Symbol '/f()/$for7/i'"]['line'] == 63)
        predicates.append(lambda: symbols["Symbol '/f()/$for7/i'"]['references'][0]['line'] == 63)
        predicates.append(lambda: symbols["Symbol '/f()/$for7/i'"]['references'][1]['line'] == 64)

        predicates.append(lambda: symbols["Symbol '/S/f()/j'"]['line'] == 75)

        if not silent: print (fg.CYAN+ style.BRIGHT+ " shadowed symbols predicates verification..."+ style.RESET_ALL)
        ok = testfuncs.verifyAllPredicates(predicates, symbols, silent)
        if ok and not silent:
            print (style.BRIGHT+ "OK! "+ str(len(predicates))+ " shadowed symbols predicates verified."+ style.RESET_ALL)

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

    if not silent: print ("testing for variables...")
    if testVariables(os.path.join(workDir, "seenat-variables.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

    if not silent: print ("testing for functions...")
    if testFunctions(os.path.join(workDir, "seenat-functions.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

    if not silent: print ("testing for methods...")
    if testMethods(os.path.join(workDir, "seenat-methods.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

    if not silent: print ("testing for structs...")
    if testStructs(os.path.join(workDir, "seenat-structs.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

    if not silent: print ("testing for SRGs...")
    if testSRGs(os.path.join(workDir, "seenat-srgs.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

    if not silent: print ("testing for constant buffers...")
    if testCBs(os.path.join(workDir, "seenat-cb.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

    if not silent: print ("testing for qualification in member access expression RHS...")
    if testMAE(os.path.join(workDir, "seenat-MAE-qualifiedRHS.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

    if not silent: print ("testing for deported methods...")
    if testDeported(os.path.join(workDir, "seenat-deported-methods.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

    if not silent: print ("testing for type alias...")
    if testTypealias(os.path.join(workDir, "seenat-typedef.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

    if not silent: print ("testing for function overloading...")
    if testFunctionOverloads(os.path.join(workDir, "seenat-function-overloads.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

    if not silent: print ("testing for unnamed blocks...")
    if testUnnamedBlocks(os.path.join(workDir, "seenat-unnamed-blocks.azsl"), compiler, silent): result += 1
    else: resultFailed += 1

if __name__ == "__main__":
    print ("please call from testapp.py")
