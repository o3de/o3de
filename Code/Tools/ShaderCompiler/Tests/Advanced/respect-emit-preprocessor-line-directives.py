#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import sys
import os
import re
sys.path.append("..")
from clr import *
import testfuncs

'''
This test suite validates that azslc respects the preprocessor
#line <number> <filepath>
directives when outputting errors.
The idea is that when reporting errors it should produce meaningful filenames
and line locations from the orignal file where the error is actually coming from.
'''

def validateFilesAppearInLineDirectives(hlslContent, fileList, silent):
    """
    @fileList List of files to search for in @hlslContent.
              It is treated as a stack and we expect the line matching
              to occur in the order as they appear in this list              
    """
    regexp = re.compile('#\s*line\s+\d+\s*"(.*)"$')
    hlslLines = hlslContent.splitlines()
    found0 = False
    for hlslLine in hlslLines:
        m = regexp.match(hlslLine)
        if not m:
            continue
        f0 = m.group(1).endswith(fileList[0])  # check top of stack
        f1 = len(fileList) > 1 and m.group(1).endswith(fileList[1]) # or second position to allow progression in the list
        if f0 or f1:
            if found0 and f1: del fileList[0]  # forget about a file only after its potential repetition is finished
            if len(fileList) == 0:
                break;
            found0 = f0
        else: print(fg.RED + f"problem: was expecting to find {fileList[0]} or {fileList[1]} (but got {m.group(1).rsplit('/',1)[-1]})" + fg.RESET)
    return len(fileList) <= 1

def testSampleFileCompilationEmitsPreprocessorLineDirectives(theFile, compilerPath, silent):
    if not silent:
        print (fg.CYAN+ style.BRIGHT+
               "testSampleFileCompilationEmitsPreprocessorLineDirectives: "+
               "Verifying sample file compiles and #line directives are emitted..."+
               style.RESET_ALL)
    stdout, ok = testfuncs.buildAndGet(theFile, compilerPath, silent, [])
    stdout = stdout.decode('utf-8')
    ok = validateFilesAppearInLineDirectives(stdout,
                                             ["srg_semantics.azsli", "level0.azsli", "level1.azsli", "level2.azsli", "main.azsl"],
                                             silent)
    return ok

def CreateTmpFileWithSyntaxError(theFile, goodSearchLine, badReplaceLine):
    """
    Takes a reference file and creates of temporary clone file with a known good line (@goodSearchLine)
    that gets replaced with a known bad line (@badReplaceLine)
    """
    dirName, fileName = os.path.split(theFile)
    tmpFilePath = os.path.join(dirName, "{}.tmp".format(fileName))
    if os.path.exists(tmpFilePath): os.remove(tmpFilePath)
    
    foundGoodSearchLine = False
    tmpFileContent = []
    with open(theFile) as fp:
        for cnt, line in enumerate(fp):
            line = line.rstrip('\r\n')
            if line == goodSearchLine:
                tmpFileContent.append("{}\n".format(badReplaceLine))
                foundGoodSearchLine = True
            else:
                tmpFileContent.append("{}\n".format(line))
    if not foundGoodSearchLine:
        print(fg.RED + f"fail: {goodSearchLine} not found in {fileName}" + fg.RESET)
        return None
    
    with open(tmpFilePath, 'w') as outFp:
        outFp.writelines(tmpFileContent)
    return tmpFilePath

def testErrorReportUsesPreprocessorLineDirectives(theFile, compilerPath, silent, goodSearchLine, badReplaceLine, searchFilename, errorType):
    """
    In this test an error will be injected at a specfic line and expect the stderr
    output produced by azslc to mention that the failure comes from one of the included files
    instead of the input file.
    """
    if not silent:
        print (fg.CYAN+ style.BRIGHT+
               "testSyntaxErrorReportUsesPreprocessorLineDirectives: "+
               "Verifying syntax error report..."+ style.RESET_ALL)
    filePathOfTmpFile = CreateTmpFileWithSyntaxError(theFile, goodSearchLine, badReplaceLine)
    if not filePathOfTmpFile:
        return False;
    if not silent:
        print (fg.CYAN+ style.BRIGHT+
               "testSyntaxErrorReportUsesPreprocessorLineDirectives: "+
               "Compiling and expecting errors..."+ style.RESET_ALL)
    stderr, failed = testfuncs.buildAndGetError(filePathOfTmpFile, compilerPath, silent, [])
    stderr = stderr.decode('utf-8')
    if not failed:
        print(fg.RED + "fail: expected non-buildable didn't report a build error." + fg.RESET)
        return False
    if not silent:
        print (fg.CYAN+ style.BRIGHT+
               "testSyntaxErrorReportUsesPreprocessorLineDirectives: "+
               "Good, good compiler error, now let's make sure the source file is mentioned..."+ style.RESET_ALL)
    if not searchFilename in stderr:
        print(fg.RED + f"fail: didn't find {searchFilename} in stderr" + fg.RESET)
        return False
    if not silent:
        print (fg.CYAN+ style.BRIGHT+
               "testSyntaxErrorReportUsesPreprocessorLineDirectives: "+
               "Good, The search file was mentioned, now let's check the type of error..."+ style.RESET_ALL)
    ok = errorType in stderr
    if not ok:
        print(fg.RED + f"fail: err #{errorType} not in stderr" + fg.RESET)
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
    
    if testSampleFileCompilationEmitsPreprocessorLineDirectives(os.path.join(workDir, "RespectEmitLine/main.azsl.mcpp"),
                                                                compiler, silent): result += 1
    else:
        print(fg.RED + "fail: testSampleFileCompilationEmitsPreprocessorLineDirectives" + fg.RESET)
        resultFailed += 1
    
    if not silent: print("\n")
    if testErrorReportUsesPreprocessorLineDirectives(os.path.join(workDir, "RespectEmitLine/main.azsl.mcpp"),
                                                           compiler, silent,
                                                           "ShaderResourceGroup SRG2 : Slot2", "ShaderResour ceGroup SRG2 : Slot2",
                                                           "level2.azsli",
                                                           "syntax error"): result += 1
    else:
        print(fg.RED + "fail: testErrorReportUsesPreprocessorLineDirectives" + fg.RESET)
        resultFailed += 1
    
    if not silent: print("\n")
    if testErrorReportUsesPreprocessorLineDirectives(os.path.join(workDir, "RespectEmitLine/main.azsl.mcpp"),
                                                           compiler, silent,
                                                           "ShaderResourceGroup SRG1 : Slot1", "ShaderResourceGroup SRG1 : SlotX",
                                                           "level1.azsli",
                                                           "Semantic error"): result += 1
    else: resultFailed += 1
    
    #if testSemanticErrorReportUsesPreprocessorLineDirectives(os.path.join(workDir, "RespectEmitLine/main.azsl.mcpp"),
    #                                                       compiler, silent): result += 1
    #else: resultFailed += 1


if __name__ == "__main__":
    print ("please call from testapp.py")
