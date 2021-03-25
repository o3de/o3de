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

# CANDLE COMMANDLINE TEMPLATES
candleCommandBase = "candle.exe -nologo -o {outputDirectory} {verbose} {preprocessorParams} {wxsFile}"
candlePackagePreprocessor = " -dProductGUID={productGUID} -dProductUpgradeGUID={upgradeCodeGUID}" \
                            " -dLumberyardVersion={lumberyardVersion}" \
                            " -dCabPrefix={cabPrefix}" \
                            " -dComponentGroupRefs={packageComponentGroup}" \
                            ' -dComponentRefs={componentRefs} -dPackageName="{packageName}"'
candleDownloadPreprocessor = " -dROOTURL={downloadURL}"
candleBootstrapPreprocessor = "-dLYInstallerPath={installerPath} -dLYInstallerNameList={installerList}" \
                              " -dLumberyardVersion={lumberyardVersion}" \
                              " -dUseStreaming={useStreaming}" \
                              ' -dMetricsSourcePath="{metricsSourcePath}"' \
                              ' -dMetricsExeName="{metricsExe}"' \
                              ' -dPathTo2015Thru2019Redist="{pathTo2015Thru2019Redist}"' \
                              ' -dFilename2015Thru2019Redist="{redist2015Thru2019Exe}"' \
                              " -dUpgradeCode={upgradeGUID} -ext WixBalExtension -ext WixUtilExtension"
usedCabPrefixList = set()


def candlePackageContent(outputDir, wxsPath, verboseMode):
    verboseCmd = getVerboseCommand(verboseMode)
    candleCommand = candleCommandBase.format(outputDirectory=os.path.join(outputDir, ''),
                                             verbose=verboseCmd, preprocessorParams="", wxsFile=wxsPath)

    verbose_print(verboseMode, '\n{}\n'.format(candleCommand))
    return os.system(candleCommand)


def candleAllPackagesContent(packageInfoMap, wixobjDir, verboseMode):
    for packageInfo in packageInfoMap.values():
        outputDir = os.path.join(wixobjDir, packageInfo['wxsName'])
        success = candlePackageContent(outputDir, packageInfo['wxsPath'], verboseMode)
        assert (success == 0), 'Failed to generate wixobj file for {} content.'.format(packageInfo['name'])


# Cab prefixes have to be less than 8 characters, including the sequential numbers for multiple cabs.
# To give room for multiple cabs for an MSI, we're dropping down to 5 characters.
# When we strip some paths in 3rd party to 5 characters, they collide, so we do a little extra
# Logic to help with collisions.
def generateCabPrefix(packageName):
    # We want a cab prefix length of five to give room for 100+ cabs for an MSI
    cabPrefixLength = 5

    uniquenessIndex = -1
    uniquenessValue = 0
    cabName = packageName[:cabPrefixLength]

    # If there is a cab name collision when two packages truncate to the same five digit
    # value, then we want to fiddle with the cab prefix to get something unique.
    # This simple logic starts at the last character in the prefix and replaces it with a numeral
    # it keeps trying that, and moving earlier in the string as it does so.
    while cabName in usedCabPrefixList:
        cabName = cabName[:cabPrefixLength+uniquenessIndex] + str(uniquenessValue) + cabName[cabPrefixLength+uniquenessIndex+1:]
        uniquenessValue += 1
        if uniquenessValue > 9:
            uniquenessValue = 0
            uniquenessIndex -= 1
            if uniquenessIndex <= -cabPrefixLength:
                raise Exception("Could not generate unique cab name for {}".format(packageName))

    usedCabPrefixList.add(cabName)
    return cabName


def candlePackage(outputDir, wxsTemplatePath, packageInfo, verboseMode, lyVersion, buildId):
    verboseCmd = getVerboseCommand(verboseMode)

    cabPrefix = generateCabPrefix(packageInfo['name'])
    productGUID = create_id(packageInfo['name'], 'PRODUCT', lyVersion, buildId)
    productUpgradeGUID = create_id(packageInfo['name'], 'PRODUCTUPDATE', lyVersion, buildId)
    componentRefs = packageInfo.get('componentRefs', '')

    candlePreprocessor = candlePackagePreprocessor.format(productGUID=productGUID,
                                                          upgradeCodeGUID=productUpgradeGUID,
                                                          lumberyardVersion=lyVersion,
                                                          componentRefs=componentRefs,
                                                          cabPrefix=cabPrefix,
                                                          packageComponentGroup=packageInfo['componentGroupRefs'],
                                                          packageName=packageInfo['sourceName'])

    candleCommand = candleCommandBase.format(outputDirectory=outputDir,
                                             verbose=verboseCmd,
                                             preprocessorParams=candlePreprocessor,
                                             wxsFile=wxsTemplatePath)

    verbose_print(verboseMode, '\n{}\n'.format(candleCommand))
    return os.system(candleCommand)


def candlePackages(packageInfoMap, wixobjDir, wxsTemplatePath, verboseMode, lyVersion, buildId):
    for packageInfo in packageInfoMap.values():
        outputDir = os.path.join(wixobjDir, packageInfo['wxsName'], 'HeatPackage{}.wixobj'.format(packageInfo['wxsName']))
        success = candlePackage(outputDir, wxsTemplatePath, packageInfo, verboseMode, lyVersion, buildId)
        assert (success == 0), 'Failed to generate wixobj file for {}.'.format(packageInfo['name'])


def candleBootstrap(outputDir,
                    installersPath,
                    installerNameList,
                    wxsFileName,
                    downloadURL,
                    verboseMode,
                    lyVersion,
                    metricsSourcePath,
                    metricsExe,
                    pathTo2015Thru2019Redist,
                    redist2015Thru2019Exe,
                    upgradeCodeGUID):
    verboseCmd = getVerboseCommand(verboseMode)
    useStreaming = downloadURL is not None
    useStreamingText = boolToWixBool(useStreaming)
    candlePreprocessor = candleBootstrapPreprocessor.format(installerPath=os.path.join(installersPath, ''),
                                                            installerList=installerNameList,
                                                            lumberyardVersion=lyVersion,
                                                            useStreaming=useStreamingText,
                                                            upgradeGUID=upgradeCodeGUID,
                                                            metricsSourcePath=metricsSourcePath,
                                                            metricsExe=metricsExe,
                                                            pathTo2015Thru2019Redist=pathTo2015Thru2019Redist,
                                                            redist2015Thru2019Exe=redist2015Thru2019Exe)
    if useStreaming:
        # The WXS file appends the trailing slash before the package name: "$(var.ROOTURL)/{2}"
        # We need to strip the trailing slash here if it was passed in with one.
        downloadURL = downloadURL.rstrip('/')
        candlePreprocessor += candleDownloadPreprocessor.format(downloadURL=downloadURL)

    candleCommand = candleCommandBase.format(outputDirectory=outputDir, verbose=verboseCmd,
                                             preprocessorParams=candlePreprocessor, wxsFile=wxsFileName)

    verbose_print(verboseMode, '\n{}\n'.format(candleCommand))
    return os.system(candleCommand)
