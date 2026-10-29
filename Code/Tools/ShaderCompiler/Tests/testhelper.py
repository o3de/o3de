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
import platform
from clr import *
import testfuncs
import subprocess
import os.path
from os.path import join, normpath, basename
import re
import glob

# for reporting in detailed mode
failList = []

# remember the last regexp patch position to avoid repeating scan from zero
lastEnd = 0
def reset():
    global lastEnd
    lastEnd = 0

def found(needle, haystack, negative):
    '''search a regex in a haystack'''
    global lastEnd  # ugly state retention to avoid headaches with the lambdas calling this.
    words = re.split("\s", needle)
    words = filter(None, words)
    words = [re.escape(w) for w in words] # need to interpret each token as a string needle, not a regex pattern.
    pttrn = "\s+".join(words) # free number of spaces between each needle.
    p = re.compile(pttrn)
    consumed = haystack[lastEnd:]  # we search on the rest only. not the whole string at each time.
    #print("consumed is", fg.YELLOW, f"{consumed[:80]}...{consumed[-20:]}", fg.RESET)
    matchObject = p.search(consumed)
    if matchObject:
        lastEnd += matchObject.end()
        #print(f"found {fg.MAGENTA} {matchObject} {fg.RESET} last end updated to {lastEnd}")
        #print(f"verification: consumed[span]={fg.GREEN}{consumed[matchObject.start():matchObject.end()]}{fg.RESET}\n")
        return not negative
    #else:
        #print(f"searched pattern was {fg.RED}{pttrn}{fg.RESET}\n")
    return negative

# parse the argument mentioned in the shader source file Ex : Cmdargs: --namespace=vk   or Cmdargs: ['--unique-idx', '--root-sig', '--root-const', '0']
def parse_strlist(sl):
    clean = re.sub("[\[\],\s]","",sl)
    splitted = re.split("[\'\"]",clean)
    values_only = [s for s in splitted if s != '']
    return values_only


def verifyEmissionPattern(azslFile, patternsFileName, compilerPath, silent, argList):
    """
    Compiles @azslFile and validates all the predicates in @patternsFileName.
    """
    if not os.path.exists(patternsFileName):
        print ("Pattern file not found: " + patternsFileName)
        return False
    arg = []
    with io.open(patternsFileName, "r", encoding="utf-8") as f:
        for line in f:
            if line.find("Cmdargs") >= 0:
                line = line[line.rfind(':')+1:]
                arg = parse_strlist(line)
                if arg and not silent: print ("Adding extra command line arguments: " + str(arg))
                # now arg has additional arguments required for the test

    shaderCode, ok = testfuncs.buildAndGet(azslFile, compilerPath, silent, argList + arg)
    if ok:
        if not silent: print (style.BRIGHT + fg.CYAN + "Now to check emission patterns for " + patternsFileName + style.RESET_ALL)
        # normalizes the shader code by inserting spaces around identifiers stuck to other things.
        # eg : 'func()' will become 'func ( )'
        allidents = re.split("([a-zA-Z_]+[a-zA-Z_0-9]*)|(\.)|(,)|(::)|(;)|(\()|(\))|(<)|(>)|( )", shaderCode.decode('utf-8'))
        allidents = filter(lambda s: s is not None and s!="" and s!=" ", allidents)  # remove empties from ['', 'code', '', 'piece']...
        shaderCode = " ".join(allidents)
        predicates = []
        # load the pattern file:
        with io.open(patternsFileName, encoding="utf-8") as f:
            i = 0
            for line in f:
                negation = line.startswith('^')
                if negation:
                    line=line[1:]
                if line.startswith("\""):
                    line = line.strip().strip('"') #remove quotes at start&end
                    if not silent: print (fg.CYAN + "Verify (" + str(i) + ")" + (" not" if negation else "") + " : " + line + style.RESET_ALL)
                    predicates.append(lambda line=line, negation=negation: found(line, shaderCode, negation))
                    i = i+1
        reset()
        ok = testfuncs.verifyAllPredicates(predicates, shaderCode, silent)
    return ok

# read the output of the compiler and tries to match some regex on it
# to verify that the output looks like what we expect.
def verifyEmissionPatterns(thefile, compilerPath, silent, argList):
    global failList
    localFailList = []
    result = 0
    base = os.path.basename(thefile)
    filePrefixName = os.path.dirname(thefile) + '/' + os.path.splitext(base)[0]
    # loop over the .txt file for the same .azsl file by matching pattern filename.txt or filename-[0-9].txt
    # extract the Cmdargs (command arguments) specified in the .txt file to compile the .azsl shader with those arguments
    for file in glob.glob('%s*[0-9].txt' % filePrefixName) or glob.glob('%s.txt' % filePrefixName):
        patterFileName = file
        if not verifyEmissionPattern(thefile, patterFileName, compilerPath, silent, argList):
            localFailList.append(patterFileName)
        else:
            result = result + 1
    if len(localFailList) > 0:
        failList.extend(localFailList)
        return 0
    else:
        return result

def printFailedTestList(silent):
    global failList
    if not silent and len(failList) > 0:
        print(style.BRIGHT + fg.RED + "failed files: " + fg.WHITE + str(failList) + style.RESET_ALL)

    failList = [] # since the module is imported for other platforms too, reset the list

def compileAndExpectError(thefile, compilerPath, silent, argList):
    """
    This function will compile @thefile, with the given @argList.
    Returns 1 (success) if the compilation failed AND the error code of the runtime exception
    produced by AZSLc matches the "#EC <code number>" expression found inside the comments of @thefile.
    Otherwise returns 0 (failure).
    """
    global failList
    result = 0

    options = [thefile]
    options.extend(argList)
    out, err, code = testfuncs.launchCompiler(compilerPath, options, silent)
    if code == 0:
        if not silent:
            print (fg.RED + style.BRIGHT + f"FAIL. Expected {thefile} to report compilation errors" + style.RESET_ALL)
        failList.append(thefile)
        return 0 # Failure
    # Read the expected error code from the source file.
    f = io.open(thefile, 'r', encoding="utf-8")
    azslCode = f.read()
    f.close()
    expectedErrorCode = testfuncs.findTokenToInt(azslCode, r"#EC\s\d*")
    outputEC = testfuncs.findTokenToInt(err.decode('utf-8'), r"error\s#\d*:")
    if outputEC == expectedErrorCode:
        return 1 # Success
    if not silent:
        print (fg.RED + style.BRIGHT + f"FAIL. Expected error code {expectedErrorCode} from {thefile}, instead got error code {outputEC}" + style.RESET_ALL)
    failList.append(thefile)
    return 0 # Success
