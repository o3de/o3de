#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import os
from BuildInstallerWixUtils import *

# LIGHT COMMANDLINE TEMPLATES
lightCommandBase = 'light.exe -nologo -o {outputPath} -ext WixUIExtension -ext WiXUtilExtension -b "{packageSource}" {verbose} {wixobjFile}'
lightCommandBootstrap = 'light.exe -nologo -o {outputPath} -ext WixBalExtension -ext WiXUtilExtension {verbose} {wixobjFile}'
lightCommandCabCache = " -cc {cabCachePath} -reusecab"


def lightPackage(outputPath, sourceDirectory, wixobjFiles, verbose, cabCachePath):
    verboseCmd = getVerboseCommand(verbose)
    lightCommand = lightCommandBase.format(outputPath=outputPath,
                                           verbose=verboseCmd,
                                           packageSource=sourceDirectory,
                                           wixobjFile=wixobjFiles)

    if cabCachePath is not None:
        lightCommand += lightCommandCabCache.format(cabCachePath=cabCachePath)

    verbose_print(verbose, '\n{}\n'.format(lightCommand))
    return os.system(lightCommand)


def lightPackages(packageInfoMap, packagesPath, wixobjFiles, verboseMode, cabCachePath):
    numPackagesBuilt = 0

    for packageInfo in packageInfoMap.values():
        outputPath = os.path.join(packagesPath, '{}.msi'.format(packageInfo['wxsName']))
        packageWixObjPath = os.path.join(os.path.join(wixobjFiles, packageInfo['wxsName']), "*.wixobj")

        success = lightPackage(outputPath, packageInfo['sourcePath'], packageWixObjPath, verboseMode, cabCachePath)
        assert (success == 0), 'Failed to generate msi and cab files for {}.'.format(packageInfo['name'])

        numPackagesBuilt += 1
        verbose_print(verboseMode, '\nPackages Built: {}\n\n'.format(numPackagesBuilt))


def lightBootstrap(outputPath, wixobjFiles, verbose, cabCachePath):
    verboseCmd = getVerboseCommand(verbose)
    lightCommand = lightCommandBootstrap.format(outputPath=outputPath, verbose=verboseCmd, wixobjFile=wixobjFiles)

    if cabCachePath is not None:
        lightCommand += lightCommandCabCache.format(cabCachePath=cabCachePath)

    verbose_print(verbose, '\n{}\n'.format(lightCommand))
    return os.system(lightCommand)
