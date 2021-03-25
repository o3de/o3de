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
from Heat import *
from Candle import *
from Light import *
from BuildInstallerWixUtils import *


def createLooseFilePackage(args,
                           params,
                           packageGroupInfoMap,
                           path,
                           wixDirId,
                           directory,
                           wixSafePathId):
    # DISCOVER LOOSE FILES IN PATH
    looseFiles = []
    for (dirpath, dirnames, filenames) in os.walk(path):
        looseFiles.extend(filenames)
        # Only the files in the root directory need to
        # be discovered this way, so break on first result.
        break

    # If there are no loose files, there is no package to create.
    if not looseFiles:
        return False

    # HEAT LOOSE FILES
    looseFileMap = heatFiles(looseFiles,
                             path,
                             params.wxsRoot,
                             wixDirId,
                             args.verbose,
                             wixSafePathId)

    # CANDLE LOOSE FILES
    rootInfo = createPackageInfo(directory, args.packageRoot, params.wxsRoot)

    # There is no general component group for loose files, so clear it out.
    rootInfo['componentGroupRefs'] = ''
    packageGroupInfoMap[rootInfo['name']] = rootInfo

    candleSubDirectory = directory
    if not candleSubDirectory:
        candleSubDirectory = wixSafePathId

    for looseFile in looseFileMap:
        candlePackageContent(os.path.join(params.wixObjOutput, candleSubDirectory),
                             looseFileMap[looseFile]['wxsPath'],
                             args.verbose)
        if rootInfo.get('componentGroupRefs', ''):
            rootInfo['componentGroupRefs'] += ';' + looseFileMap[looseFile]['componentGroupRefs']
        else:
            rootInfo['componentGroupRefs'] = looseFileMap[looseFile]['componentGroupRefs']

    return True


def createThirdPartyPackages(args, params):
    """
    Creates a .msi and .cab files for each folder in 3rdParty, and places them at
        intermediateInstallerPath to be used when creating the bootstrapper.
    @return - A dictionary of the package information that was generated for the
        3rd Party directories.
    """
    thirdPartyWixDirId = "THIRDPARTYDIR"
    thirdPartyPath = os.path.join(args.packageRoot, "3rdParty")
    # HEAT PACKAGE
    thirdPartyInfoMap = heatDirectories(thirdPartyPath, params.wxsRoot,
                                    thirdPartyWixDirId, args.verbose, "ThirdParty")
    # CANDLE PACKAGE CONTENTS
    candleAllPackagesContent(thirdPartyInfoMap, params.wixObjOutput, args.verbose)

    # CREATE PACKAGE INFORMATION FOR LOOSE FILES IN 3RD PARTY ROOT
    createLooseFilePackage(args,
                           params,
                           thirdPartyInfoMap,
                           thirdPartyPath,
                           thirdPartyWixDirId,
                           "3rdParty",
                           "ThirdParty")

    # CANDLE PACKAGES
    candlePackages(thirdPartyInfoMap,
                   params.wixObjOutput,
                   params.heatPackageBase,
                   args.verbose,
                   args.lyVersion,
                   args.buildId)

    # LIGHT PACKAGES
    lightPackages(thirdPartyInfoMap,
                  params.packagesPath,
                  params.wixObjOutput,
                  args.verbose,
                  args.cabCachePath)

    return thirdPartyInfoMap


def createRootPackage(args, params):
    rootInfoMap = {}
    if createLooseFilePackage(args,
                              params,
                              rootInfoMap,
                              os.path.join(args.packageRoot, ""),
                              "INSTALLDIR",
                              "",
                              "packageRoot"):
        # CANDLE PACKAGES
        candlePackages(rootInfoMap,
                       params.wixObjOutput,
                       params.heatPackageBase,
                       args.verbose,
                       args.lyVersion,
                       args.buildId)
        rootInfoMap['packageRoot']['sourcePath'] = os.path.abspath(args.packageRoot)
        # LIGHT PACKAGES
        lightPackages(rootInfoMap,
                      params.packagesPath,
                      params.wixObjOutput,
                      args.verbose,
                      args.cabCachePath)

    return rootInfoMap


def createDevPackage(args, params):
    """
    Creates a .msi and .cab files for the dev folder, and places them at
        intermediateInstallerPath to be used when creating the bootstrapper.
    @return - A dictionary of the package information that was generated for the
        dev directory.
    """
    devWixDirId = "DEVDIR"
    devPath = os.path.join(args.packageRoot, "dev")

    # HEAT PACKAGE
    devInfoMap = heatDirectories(devPath, params.wxsRoot, devWixDirId, args.verbose, "dev", args.dirFilelist)

    # CANDLE PACKAGE CONTENTS
    candleAllPackagesContent(devInfoMap, params.wixObjOutput, args.verbose)

    # CREATE PACKAGE INFORMATION FOR LOOSE FILES IN DEV ROOT
    createLooseFilePackage(args,
                           params,
                           devInfoMap,
                           devPath,
                           devWixDirId,
                           "dev",
                           "dev")

    devInfo = devInfoMap["dev"]
    devInfo['componentRefs'] = 'DesktopShortcuts;StartMenuShortcuts;LevelListRegistryKeys'

    # CANDLE PACKAGE
    candlePackages(devInfoMap,
                   params.wixObjOutput,
                   params.heatPackageBase,
                   args.verbose,
                   args.lyVersion,
                   args.buildId)

    # Build the dev-specific wxs components (for shortcuts)
    candlePackage('{}/{}/'.format(params.wixObjOutput, devInfo['name']),
                  'HeatDevPackageBase.wxs',
                  devInfo,
                  args.verbose,
                  args.lyVersion,
                  args.buildId)

    # LIGHT PACKAGE
    lightPackages(devInfoMap,
                  params.packagesPath,
                  params.wixObjOutput,
                  args.verbose,
                  args.cabCachePath)

    return devInfoMap


def createRootFolderPackage(args, params, folderName):
    """
    Creates a .msi and .cab files for a folder in the package root, like docs, and places them at
        intermediateInstallerPath to be used when creating the bootstrapper.
    @return - A dictionary of the package information that was generated for the
        docs directory.
    """
    folderInfo = createPackageInfo(folderName, args.packageRoot, params.wxsRoot)
    folderInfoMap = {}
    folderInfoMap[folderInfo['name']] = folderInfo

    # HEAT PACKAGE
    heatDirectory(folderInfo['wxsName'], folderInfo['sourcePath'], folderInfo['wxsPath'],
        folderInfo['componentGroupRefs'], "INSTALLDIR", args.verbose)

    # CANDLE PACKAGE CONTENTS
    candleAllPackagesContent(folderInfoMap, params.wixObjOutput, args.verbose)

    # CANDLE PACKAGE
    candlePackages(folderInfoMap,
                   params.wixObjOutput,
                   params.heatPackageBase,
                   args.verbose,
                   args.lyVersion,
                   args.buildId)

    # LIGHT PACKAGE
    lightPackages(folderInfoMap,
                  params.packagesPath,
                  params.wixObjOutput,
                  args.verbose,
                  args.cabCachePath)

    return folderInfoMap
