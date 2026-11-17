#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import sys
import os
import io
from argparse import ArgumentParser
from os.path import join, normpath, basename
import inspect
clrpath = os.path.join(os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe()))), "clr.py")
print(f"clr path is {clrpath}")
import importlib.util
spec = importlib.util.spec_from_file_location("clr", clrpath)
clrmodule = importlib.util.module_from_spec(spec)
sys.modules["clr"] = clrmodule
spec.loader.exec_module(clrmodule)
globals().update({v: vars(clrmodule)[v] for v in ["fg", "bg", "style"]})
import re
import testfuncs
import importlib
from timeit import default_timer as timer
from datetime import timedelta

class testResult:
    def __init__(self, p, t, f, m):
        self.numPass = p
        self.numTodo = t
        self.numFail = f
        self.numEC = m   #EC = error code

    def add(self, other):
        self.numPass += other.numPass
        self.numTodo += other.numTodo
        self.numFail += other.numFail
        self.numEC += other.numEC

'''Returns a testResult. Prints a color-coded status before returning. Total number of tests is (numPass + numTodo + numFail).'''
def GetStatusVerbose(numPass, numFail, inputsource, suffixMessage, extraColor, extraMessage):
    if numFail == 0:
        print(fg.GREEN + style.BRIGHT+ "\\\\[ OK ]// : {}".format(inputsource)+ \
            suffixMessage+ extraColor+ extraMessage+ style.RESET_ALL)
        return testResult(numPass, 0, numFail, 0)

    if inputsource.find("wip-") >= 0:
        print (fg.YELLOW+ style.BRIGHT+ "\\\\[ TODO ]// : {}".format(inputsource)+\
            suffixMessage+ extraColor+ extraMessage+ style.RESET_ALL)
        return testResult(numPass, numFail, 0, 0)

    if extraMessage.find("Missing ErrorCode") >= 0:
        print (fg.YELLOW+ style.BRIGHT+ "\\\\[ EC ]// : {}".format(inputsource)+\
            suffixMessage+ extraColor+ extraMessage+ style.RESET_ALL)
        return testResult(numPass, 0, 0, numFail)

    print (fg.RED+ style.BRIGHT+ "\\\\[ FAILED ]// : {}".format(inputsource)+\
        suffixMessage+ extraColor+ extraMessage+ style.RESET_ALL)
    return testResult(numPass, 0, numFail, 0)

'''Returns a testResult. Total number of tests is (numPass + numTodo + numFail)'''
def testAdvancedScript(advancedFolder, inputsource, whatToTest, compilerPath, silent, az3rdParty):
    pass

'''Returns a testResult. Total number of tests is (numPass + numTodo + numFail)'''
def testFile(advancedFolder, inputsource, expectPass, whatToTest, compilerPath, silent, az3rdParty):
    '''returned integer is the number of ok-tests passed. You can use it to increment your global counter'''
    if inputsource.endswith(".py"):
        # Import the new module:
        # - make sure it's discoverable
        # - load the module
        # - change the current working directory
        sys.path.append(advancedFolder)
        moduleName = os.path.splitext(os.path.basename(inputsource))[0]
        testScriptModule = importlib.import_module(moduleName)
        oldCwd = os.getcwd()
        os.chdir(advancedFolder)

        # Run the test
        testScriptModule.doTests(compilerPath, silent, az3rdParty)

        # Unload the module:
        # - change the current working directory back
        # - make sure it's *not* discoverable
        # - delete the module
        os.chdir(oldCwd)
        sys.path.remove(advancedFolder)
        runResult = GetStatusVerbose(testScriptModule.result, testScriptModule.resultFailed, inputsource, "", fg.WHITE, "")
        del sys.modules[moduleName]
        return runResult

    options = [inputsource]
    if whatToTest == "[syntax]":
        options.append("--syntax")
    if whatToTest == "[semantic]":
        options.append("--semantic")
    out, err, code = testfuncs.launchCompiler(compilerPath, options, silent)
    (okpreds, numpreds) = testfuncs.executePredicateChecks(out)
    outputEC = testfuncs.findTokenToInt(err.decode('utf-8'), r"error\s#\d*:")
    fine = code == 0 and okpreds
    if expectPass == fine:
        if not whatToTest == "[syntax]" and not expectPass:
            # further verify in the case of Semantic+error check. do a second --syntax pass to verify it passes.
            # because semantic tests should not have syntax errors.
            if whatToTest == "[semantic]": options.pop()
            options.append("--syntax")
            out, err, syntaxCode = testfuncs.launchCompiler(compilerPath, options, silent)
            if syntaxCode != 0:
                return GetStatusVerbose(0, 1, inputsource, "", fg.RED,"Invalid syntax in a 'semantic error' check. These tests must have valid syntax.")
        predsOkMsg = " (" + str(numpreds) + " predicates ok)" if numpreds else ""

        # check if error code matches from the azlsc and azsl file
        f = io.open(inputsource, 'r', encoding="latin-1")
        azslCode = f.read()
        f.close()
        errorCode = testfuncs.findTokenToInt(azslCode, r"#EC\s\d*")
        if errorCode == -2:
            return GetStatusVerbose(0, 1, inputsource, "", fg.RED, ' Error code does not contain an integer')
        if errorCode != -1 and outputEC != errorCode:
            return GetStatusVerbose(0, 1, inputsource, "", fg.RED, ' Error code returned form the compiler(\"{0}\") does not match the one expected in the azsl file(\"{1}\")'.format(outputEC, errorCode))

        # errorcodes is only supported for Semantic related errors.
        # Log the files that don't have a specific errorcode assigned to their exceptions
        if outputEC == 1 and not whatToTest == "[syntax]":
            return GetStatusVerbose(1, 1, inputsource, "", fg.YELLOW, ' Missing ErrorCode')

        return GetStatusVerbose(1, 0, inputsource, predsOkMsg, fg.WHITE, "")

    else:
        expectedCode = 0 if expectPass else 1
        gotExpected = expectedCode == code
        codeMessage = fg.WHITE + " got expected code " + str(code)
        errorColor = (fg.WHITE if okpreds else fg.RED)
        errorMessage = " predicates: " + ("None" if numpreds==0 else str(okpreds))
        if not gotExpected:
            codeMessage = " expected code " + str(expectedCode) + " (but got " + str(code) + ")"
        return GetStatusVerbose(0, 1, inputsource, codeMessage, errorColor, errorMessage)

'''Returns a testResult. Total number of tests is (numPass + numTodo + numFail + numEC)'''
def runTests(testsRoot, paths, compiler, verboseLevel, az3rdParty):
    compiler = os.path.abspath(compiler) # TODO Does this work on Jenkins? Or do we need another solution?
    numTotal = testResult(0, 0, 0, 0)
    argsAreOnlyFiles = all([os.path.isfile(join(testsRoot, p)) for p in paths])
    if argsAreOnlyFiles:
        for f in paths:
            f = join(testsRoot, f)
            whatToTest = "[everything]"
            if "syntax" in f.lower(): whatToTest = "[syntax]"
            if "semantic" in f.lower(): whatToTest = "[semantic]"
            if "samples" in f.lower(): whatToTest = "[samples]"
            if verboseLevel > 0: print (fg.MAGENTA+ style.BRIGHT+ "== individual check "+ f+ " =="+ style.RESET_ALL)
            numTotal.add( testFile("Advanced", f, "error" not in f.lower(), whatToTest, compiler, True if verboseLevel < 2 else False, az3rdParty) )
    else:
        for dir in paths:
            joinDir = join(testsRoot, dir)
            for root, dirs, files in os.walk(joinDir):
                for f in files:
                    expectPass = "error" not in root.lower()
                    whatToTest = "[everything]"
                    if "syntax" in root.lower(): whatToTest = "[syntax]"
                    if "semantic" in root.lower(): whatToTest = "[semantic]"
                    if "samples" in root.lower(): whatToTest = "[samples]"
                    advancedTest = "advanced" in root.lower()  # these tests have their own python routines
                    if advancedTest:
                        if f.endswith(".py"):
                            if verboseLevel > 0: print (fg.MAGENTA+ style.BRIGHT+ "== advanced script "+ join(root,f)+ style.RESET_ALL)
                            numTotal.add( testFile(joinDir, join(root, f), expectPass, whatToTest, compiler, True if verboseLevel < 2 else False, az3rdParty) )
                    elif f.endswith(".azsl"):
                        whatmust = "must [pass]" if expectPass else "must [fail]"
                        if verboseLevel > 0: print (fg.MAGENTA + style.BRIGHT + "== start to build " + join(root,f) + " == "+ whatmust + whatToTest + style.RESET_ALL)
                        numTotal.add( testFile(joinDir, join(root, f), expectPass, whatToTest, compiler, True if verboseLevel < 2 else False, az3rdParty) )
    return (numTotal, argsAreOnlyFiles)

'''Returns a testResult. Total number of tests is (numPass + numTodo + numFail + numEC)'''
def runAll(testsRoot, paths, compiler, verboseLevel, az3rdParty):
    numAllTests, argsWereOnlyFiles = runTests(testsRoot, paths, compiler, verboseLevel, az3rdParty)

    if argsWereOnlyFiles:
        return numAllTests
    # do PAL only for non-specific test runs.
    platformsDir = join(testsRoot, "../Platform/")
    if os.path.exists(platformsDir):
        subDirs = [join(platformsDir, dir) for dir in os.listdir(platformsDir) if os.path.isdir(join(platformsDir, dir))]
        for d in subDirs:
            print (fg.WHITE + style.BRIGHT+ "Per platform testing (" + d + ")" + style.RESET_ALL)
            platformTests = join(d, "tests")
            if os.path.isdir(platformTests):
                numAllTests.add( runTests(platformTests, paths, compiler, verboseLevel, az3rdParty)[0] )
            else: print (fg.GREEN + style.BRIGHT + "               ... no extra tests found." + style.RESET_ALL)
    return numAllTests

if __name__ == "__main__":
    os.system('') # activate VT100 mode for windows console

    try:
        import yaml
    except ImportError as err:
        print ( fg.YELLOW + style.BRIGHT + "It seems your python environment lacks pyyaml. Run first through project-root's \"test.and.py\" (or pip install it)" + style.RESET_ALL )
        if input("Continue (may result in false failures)? y/n:").lower() != "y":
            exit(0)

    parser = ArgumentParser()
    parser.add_argument(
        '--path', dest='path',
        type=str, nargs='*',
        help='the directories with test files (takes . if not provided)',
    )
    parser.add_argument(
        '--compiler', dest='compiler',
        type=str,
        help='the path to the compiler exe',
    )
    parser.add_argument(
        '--silent', dest='silent',
        action='store_true', default=False,
        help="not show compiler's stdout",
    )
    parser.add_argument(
        '--az3rdParty', dest='az3rdParty',
        type=str,
        help="The path to amazon DXC)",
    )
    args = parser.parse_args()

    paths = args.path
    if paths is None: paths = ["."]
    # if all paths are files (i.e. do not contain even one directory)
    verboseLevel = 1 if args.silent else 2

    # chrono
    startTime = timer()
    # go
    numAllTests = runAll(".", paths, args.compiler, verboseLevel, args.az3rdParty)

    numTotal = numAllTests.numPass + numAllTests.numTodo + numAllTests.numFail + numAllTests.numEC
    # measure
    endTime = timer()
    # print stats
    print (fg.WHITE + style.BRIGHT + "FINISHED. Total = {}".format(numTotal) + style.RESET_ALL)
    print (fg.GREEN + style.BRIGHT + "PASS = {}".format(numAllTests.numPass) + fg.WHITE+ " /{}".format(numTotal) + style.RESET_ALL)
    print (fg.YELLOW + style.BRIGHT + "TODO = {}".format(numAllTests.numTodo) + fg.WHITE+ " /{}".format(numTotal) + style.RESET_ALL)
    print (fg.YELLOW + style.BRIGHT + "Missing EC = {}".format(numAllTests.numEC) + fg.WHITE+ " /{}".format(numTotal) + style.RESET_ALL)
    print (fg.RED + style.BRIGHT + "FAIL = {}".format(numAllTests.numFail) + fg.WHITE+ " /{}".format(numTotal) + style.RESET_ALL)
    td = timedelta(seconds=(endTime - startTime))
    print (fg.CYAN + style.BRIGHT + "Time taken: " + fg.WHITE + str(td) + style.RESET_ALL)
