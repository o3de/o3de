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
import inspect
import pprint
sys.path.append("..")
sys.path.append("../..")
from clr import *
import testfuncs
import subprocess
import os.path
from os.path import join, normpath, basename
import re

spirvcrosspath = ""

class buildResult:
    def __init__(self, c, d):
        self.canBuild = c     # Indicates if this environment can build the shader source
        self.didBuild = d     # Indicates if the shaders were successfully built

buildImpossible = buildResult(False, False)
buildFailed     = buildResult(True, False)
buildSucceeded  = buildResult(True, True)

'''Returns the path to the Windows 10 version of dxc, or None if it can't be found'''
def findDXC(silent):
    if not (platform.system() == "Windows" and platform.release() == "10"):
        if not silent: print (fg.YELLOW+ style.BRIGHT+ "Will only test dxc.exe on Windows 10."+ style.RESET_ALL)
        return None

    windowsKitsDir = os.path.expandvars("%ProgramFiles(x86)%/Windows Kits/10/bin")
    if not os.path.isdir(windowsKitsDir):
        if not silent: print (fg.YELLOW+ style.BRIGHT+ "Expected {}, but did not find it.".format(windowsKitsDir)+ style.RESET_ALL)
        return None

    dxcLocations = []
    for root, dir, files in os.walk(windowsKitsDir):
        if "dxc.exe" in files and root.find("x64") >= 0:
            dxcLocations.append(os.path.join(root, "dxc.exe"))
    dxcLocations.sort(reverse=True)

    if not dxcLocations:
        return None

    return dxcLocations[0]

'''Returns the path to the Spirv-Cross compiler, or None if it can't be found'''
def findSpirvCross(spirvCrossPath, silent):
    if not os.path.isdir(spirvCrossPath):
        if not silent: print (fg.YELLOW+ style.BRIGHT+ "Expected {}, but did not find it.".format(spirvCrossPath)+ style.RESET_ALL)
        return None

    spirvLocations = []
    for root, dir, files in os.walk(spirvCrossPath):
        if os.name == 'nt':
            if "spirv-cross.exe" in files and root.find("win_x64") >= 0:
                spirvLocations.append(os.path.join(root, "spirv-cross.exe"))
        elif os.name == 'posix':
            if "spirv-cross" in files and root.find("darwin_x64") >= 0:
                spirvLocations.append(os.path.join(root, "spirv-cross"))

    spirvLocations.sort(reverse=True)

    if not spirvLocations:
        return None

    return spirvLocations[0]

'''Caches the Spirv-Cross path and returns the path to the dxc compiler, or None if it can't be found'''
def findDXCAndSpirvCross(silent, azdxcpath, needSvc):
    azVersionDXC = False
    global spirvcrosspath
    if azdxcpath is not None:
        spirvcrosspath = azdxcpath + "/SPIRVCross/"
        spirvcrosspath = findSpirvCross(spirvcrosspath, silent)
        if needSvc and spirvcrosspath is None:
            if not silent: print (fg.YELLOW+ style.BRIGHT+ "SPIRV not found. *Assuming* we shouldn't test it on this platform."+ style.RESET_ALL)
            return None
        if platform.system() == "Windows":
            azdxcpath = azdxcpath + "/DirectXShaderCompiler/3.0.0-az/bin/win_x64/Release/dxc.exe"
        elif platform.system() == "Darwin":
            azdxcpath = azdxcpath + "/DirectXShaderCompiler/3.0.0-az/bin/darwin_x64/Release/bin/dxc"
        elif platform.system() == "Linux":
            azdxcpath = azdxcpath + "/DirectXShaderCompiler/3.0.0-az/bin/linux_x64/Release/bin/dxc"
        else:
            return None

    if azdxcpath and os.path.exists(azdxcpath):
        dxcPath = azdxcpath         # Use amazon version of dxc
        azVersionDXC = True
    else:
        dxcPath = findDXC(silent)   # Use windows version of dxc
        if needSvc:
            print (fg.YELLOW+ style.BRIGHT+ "Windows' DXC does not support SPIRV. skipping."+ style.RESET_ALL)
            return None

    if dxcPath == None:
        if not silent: print (fg.YELLOW+ style.BRIGHT+ "DXC not found. *Assuming* we shouldn't test it on this platform."+ style.RESET_ALL)
        return None
    return dxcPath

'''Returns buildResult'''
def buildDXC(thefile, compilerPath, silent, extraIncList, azslcArgs, dxcArgs, azdxcpath, OutFormat, needSpirvCross):
    dxcPath = findDXCAndSpirvCross(silent, azdxcpath, needSpirvCross)
    if dxcPath is None:
        return buildImpossible

    inFile, inFileExt = os.path.splitext(thefile)
    codeOut  = inFile + ".hlsl"
    sbinVS   = inFile + "VS." + OutFormat
    sbinPS   = inFile + "PS." + OutFormat

    if os.path.exists(codeOut): os.remove(codeOut)
    if os.path.exists(sbinVS):  os.remove(sbinVS)
    if os.path.exists(sbinPS):  os.remove(sbinPS)

    stdout, ok = testfuncs.buildAndGet(thefile, compilerPath, silent, azslcArgs)
    if not ok:
        if not silent: print (fg.RED+ style.BRIGHT+ "Failed to generate .hlsl file with AZSLc."+ style.RESET_ALL)
        return buildFailed

    # HLSL CodeGen - add all extra includes from the list, supposing they're local to the target source file
    f = open(codeOut, "wb+")
    for incFile in extraIncList:
        fInFullname = os.path.join(os.path.dirname(thefile), incFile)
        if not os.path.exists(fInFullname):
            if not silent: print (fg.RED+ style.BRIGHT+ "Include file {} doesn't exist!".format(fInFullname)+ style.RESET_ALL)
            return buildFailed
        fIn = open(fInFullname, "rb")
        f.write(fIn.read())
        fIn.close()
    f.write(stdout)
    f.close()

    dxcCompileArgs = [dxcPath, "-T", "vs_6_2", "-E", "MainVS"] + dxcArgs + ["-Fo", sbinVS, codeOut]
    process = subprocess.Popen(dxcCompileArgs, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if not silent: sys.stderr.write(err.decode('utf-8'))
    if not os.path.exists(sbinVS):
        if not silent: print (fg.RED + style.BRIGHT + "Failed to compile MainVS with dxc.exe(for "+OutFormat+")." + style.RESET_ALL)
        return buildFailed

    dxcCompileArgs = [dxcPath, "-T", "ps_6_2", "-E", "MainPS"] + dxcArgs + ["-Fo", sbinPS, codeOut]
    process = subprocess.Popen(dxcCompileArgs, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if not silent: sys.stderr.write(err.decode('utf-8'))
    if not os.path.exists(sbinPS):
        if not silent: print (fg.RED+ style.BRIGHT+ "Failed to compile MainPS with dxc.exe(for "+OutFormat+")."+ style.RESET_ALL)
        return buildFailed

    return buildSucceeded

'''Returns buildResult'''
def buildDXCCompute(thefile, compilerPath, silent, extraIncList, azslcArgs, dxcArgs, azdxcpath, OutFormat, needSpirvCross):
    dxcPath = findDXCAndSpirvCross(silent, azdxcpath, needSpirvCross)
    if dxcPath is None:
        return buildImpossible

    if not silent: print (fg.CYAN+ style.BRIGHT+ "Testing DXC compilation for compute shaders."+ style.RESET_ALL)

    inFile, inFileExt = os.path.splitext(thefile)
    codeOut  = inFile + ".hlsl"
    sbinCS   = inFile + "CS." + OutFormat

    if os.path.exists(codeOut): os.remove(codeOut)
    if os.path.exists(sbinCS):  os.remove(sbinCS)

    stdout, ok = testfuncs.buildAndGet(thefile, compilerPath, silent, azslcArgs)
    if not ok:
        if not silent: print (fg.RED+ style.BRIGHT+ "Failed to generate .hlsl file with AZSLc."+ style.RESET_ALL)
        return buildFailed

    # HLSL CodeGen - add all extra includes from the list, supposing they're local to the target sourc file
    f = open(codeOut, "wb+")
    for incFile in extraIncList:
        fInFullname = os.path.join(os.path.dirname(thefile), incFile)
        if not os.path.exists(fInFullname):
            if not silent: print (fg.RED+ style.BRIGHT+ "Include file {} doesn't exist!".format(fInFullname)+ style.RESET_ALL)
            return buildFailed
        fIn = io.open(fInFullname, "r", encoding="latin-1")
        f.write(fIn.read())
        fIn.close()
    f.write(stdout)
    f.close()

    dxcCompileArgs = [dxcPath, "-T", "cs_6_2", "-E", "MainCS"] + dxcArgs + ["-Fo", sbinCS, codeOut]
    process = subprocess.Popen(dxcCompileArgs, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if not silent: sys.stderr.write(err.decode('utf-8'))
    if not os.path.exists(sbinCS):
        if not silent: print (fg.RED+ style.BRIGHT+ "Failed to compile MainCS with dxc.exe."+ style.RESET_ALL)
        return buildFailed

    if not silent: print (fg.GREEN+style.BRIGHT+ "["+thefile+"."+ OutFormat + "]" + style.RESET_ALL)
    return buildSucceeded
