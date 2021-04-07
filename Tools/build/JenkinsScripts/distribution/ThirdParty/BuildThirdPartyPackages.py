"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import BuildThirdPartyArgs
import BuildThirdPartyUtils
import SDKPackager
import ThirdPartySDKAWS
import json
import os
import re
import sys
import urlparse

importDir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(importDir, "..")) #Required for AWS_PyTools
from AWS_PyTools import LyChecksum
from AWS_PyTools import LyCloudfrontOps

# Some files are in the 3rd party folder and not associated with any SDK/Versions.
fileIgnorelist = [
    "boost/Boost_Autoexp.dat",
    "boost/CryEngine Customizations.txt",
    "boost/CryREADME.txt",
    "boost/LICENSE_1_0.txt",
    "boost/lumberyard-1.61.0.patch",
    "Qwt/license.txt",
    "lz4/git checkout.txt"
    "3rdParty.txt"
]


class SDK(object):

    def __init__(self):
        self.fileList = []
        self.path = ""
        self.versionFolder = ""


def getListFromFile(file):
    openFile = open(file, 'r')
    filePaths = openFile.readlines()
    openFile.close()
    return filePaths


def addUntrackedSDKs(sdks):
    # Not all SDKs are represented in SetupAssistantConfig.json yet.

    untrackedSDKs = {
        "OpenEXR20": ["OpenEXR/2.0", "2.0"],
        "OpenEXR22": ["OpenEXR/2.2", "2.2"],
    }

    for sdkName, untrackedSDK in untrackedSDKs.iteritems():
        sdk = SDK()
        sdk.path = untrackedSDK[0]
        sdk.versionFolder = untrackedSDK[1]
        sdks[sdkName] = sdk


def getSDKsToPathsDict(thirdPartyVersionsFile):
    versionsList = getListFromFile(thirdPartyVersionsFile)
    sdks = {}
    for version in versionsList:
        match = re.match(r"(.*)(\.package.dir=)(.*)", version)
        if not match:
            BuildThirdPartyUtils.printError('SDK version file {0} has invalid formatting on line {1}'.format(
                    thirdPartyVersionsFile,
                    version))

        sdkName = match.group(1)
        # Store the SDK Path with forward slashes to make matching easier.
        sdkPath = match.group(3).replace('\\', '/')
        sdk = SDK()
        sdk.path = sdkPath

        versionMatch = re.search(r"([^/\\\\]*)$", sdkPath)
        if not versionMatch:
            BuildThirdPartyUtils.printError('SDK version file {0} has invalid formatting on line {1}'.format(
                    thirdPartyVersionsFile,
                    version))

        sdk.versionFolder = versionMatch.group(1)
        sdks[sdkName] = sdk

    addUntrackedSDKs(sdks)
    return sdks


def populateSDKFilePaths(sdks, sdkFileListFile):
    fileList = getListFromFile(sdkFileListFile)
    lastSDKName = ""
    for file in fileList:
        slashesFixed = file.replace('\\', '/')
        match = re.search("3rdParty/(.*)", slashesFixed)
        if not match:
            BuildThirdPartyUtils.printError("Could not find third party folder in file path {0}".format(file))

        localPath = match.group(1)
        sdkFound = False

        # Files are generally grouped by SDK in the file list,
        # caching the last SDK used can save a search through the loop.
        if lastSDKName:
             if localPath.startswith(sdks[lastSDKName].path):
                sdk.fileList.append(file)
                sdkFound = True
                continue

        for sdkName, sdk in sdks.iteritems():
            if localPath.startswith(sdk.path):
                sdk.fileList.append(file)
                sdkFound = True
                lastSDKName = sdkName
                break

        # Some files are loose and not trackable within the current system. For now we're going to ignore them,
        # and they will need to be included in the package manually.
        for ignore in fileIgnorelist:
            if localPath == ignore:
                sdkFound = True
                break

        if not sdkFound:
            BuildThirdPartyUtils.printError("File {0} is not associated with any known SDKs".format(
                    file,
                    sdkFileListFile))


def checkForSDKStagingErrors(bucket, baseBucketPath, sdkPath, sdkPlatform, filesetHash):
    tmpDirPath = SDKPackager.getTempDir(sdkPath, sdkPlatform)

    filelistFileName = SDKPackager.getFilelistFileName(sdkPlatform)
    filelistLocalPath = os.path.join(tmpDirPath, filelistFileName)
    filelistStagingPath = ThirdPartySDKAWS.getS3StagingPath(baseBucketPath, sdkPath) + filelistFileName

    if not os.path.exists(tmpDirPath):
        os.makedirs(tmpDirPath)
    bucket.download_file(filelistStagingPath, filelistLocalPath)

    filelistData = open(filelistLocalPath, 'r')
    filelistJsonData = json.load(filelistData)
    filelistData.close()

    assert(filelistData != ""), "Failed to load from " + filelistLocalPath

    filelistChecksum = filelistJsonData["filelist"]["checksum"]

    if filelistChecksum != filesetHash.hexdigest():
        # If the checksums don't match, then the SDK has been modified without changing the version. This is an error.
        return True
    return False


def checkForExistingSDK(ignoreExisting,
                        bucket,
                        baseBucketPath,
                        sdkName,
                        versionFolder,
                        sdkPath,
                        sdkPlatform,
                        filesetHash):
    returnCode = 0
    statusMessage = None
    # Check for existing manifest
    manifestExists, filelistExists = ThirdPartySDKAWS.getSDKStagingStatus(bucket,
                                                                          baseBucketPath,
                                                                          sdkPath,
                                                                          sdkPlatform)
    if ignoreExisting:
        return statusMessage, returnCode

    # If the manifest or filelist is missing, but one is available, then the SDK likely failed to upload.
    # In this case, just continue on and generate the SDK package.
    if manifestExists and filelistExists:
        # If the manifest and filelist both exist, this SDK.version.package has been uploaded already.
        stagingError = checkForSDKStagingErrors(bucket,
                                                baseBucketPath,
                                                sdkPath,
                                                sdkPlatform,
                                                filesetHash)

        if stagingError:
            statusMessage = "ERROR: The file list manifests do not match for SDK {0}, Version {1}, Platform {2}".format(sdkName,
                                                                                                                 versionFolder,
                                                                                                                 sdkPlatform)
            returnCode = 1
        else:
            statusMessage = "\tEverything is up to date for SDK {0}, Version {1}, Platform {2}".format(sdkName,
                                                                                                     versionFolder,
                                                                                                     sdkPlatform)
    if statusMessage:
        print statusMessage
    return statusMessage, returnCode


def getListFromJsonFile(jsonFile, root):
    if not jsonFile:
        return []
    if not os.path.isfile(jsonFile):
        print "{} is not a valid file, please check the filename specified.".format(jsonFile)
        exit(1)
    with open(jsonFile, 'r') as source:
        source_json = json.load(source)
        try:
            sdks_list = source_json[root]
            return sdks_list
        except KeyError:
            print "Unknown json root {}, please check the json root specified.".format(root)
            exit(1)


############################


def main():
    print "Building third party packages"
    args = BuildThirdPartyArgs.createArgs()

    print "Parsing file lists"
    ignoreExistingSDKList = getListFromJsonFile(args.ignoreExistingList, args.sdkPlatform)
    sdkBlacklist = getListFromJsonFile(args.sdkBlacklist, "Blacklist")
    sdks = getSDKsToPathsDict(args.thirdPartyVersions)
    populateSDKFilePaths(sdks, args.sdkFilelist)

    cloudfrontDist = LyCloudfrontOps.getCloudfrontDistribution(args.cloudfrontDomain, args.awsProfile)
    bucket = LyCloudfrontOps.getBucket(cloudfrontDist, args.awsProfile)
    baseBucketPath = LyCloudfrontOps.buildBucketPath(urlparse.urljoin(args.cloudfrontDomain, args.stagingFolderPath), cloudfrontDist)
    baseCloudfrontUrl = args.cloudfrontDomain # we assume that the domain ends in a trailing '/'
    if args.stagingFolderPath:
        baseCloudfrontUrl += args.stagingFolderPath # we assume that the folder path ends in a trailing '/'

    returnCode = 0
    sdksGenerated = []

    sdkCount = len(sdks)
    currentSDK = -1
    for sdkName, sdk in sdks.iteritems():
        currentSDK += 1
        print "{0}/{1} - Processing {2}".format(currentSDK, sdkCount, sdkName)

        # Don't process SDKs in the blacklist.
        if sdkName in sdkBlacklist:
            print "\tSkipping blacklist SDK {0}".format(sdkName)
            continue
        # Hash all files for the SDK
        filesetHash = LyChecksum.generateFilesetChecksum(sdk.fileList)

        if args.internalPackage:
            ignoreExisting = True
        else:
            ignoreExisting = sdkName in ignoreExistingSDKList

        statusMessage, sdkReturnCode = checkForExistingSDK(ignoreExisting,
                                                           bucket,
                                                           baseBucketPath,
                                                           sdkName,
                                                           sdk.versionFolder,
                                                           sdk.path,
                                                           args.sdkPlatform,
                                                           filesetHash)
        # The final return should be the highest reported return code.
        returnCode = max(returnCode, sdkReturnCode)
        if statusMessage:
            continue
        manifestPath, filelistPath, zipFiles = SDKPackager.generateSDKPackage(baseCloudfrontUrl,
                                                                              sdkName,
                                                                              sdk.versionFolder,
                                                                              sdk.path,
                                                                              args.sdkPlatform,
                                                                              filesetHash,
                                                                              sdk.fileList,
                                                                              args.archiveMaxSize)
        if not args.skipUpload:
            ThirdPartySDKAWS.uploadSDKToStaging(bucket,
                                                baseBucketPath,
                                                sdkName,
                                                sdk.versionFolder,
                                                sdk.path,
                                                args.sdkPlatform,
                                                manifestPath,
                                                filelistPath,
                                                zipFiles)
        print "{0}/{1} - Completed Processing {2}".format(currentSDK+1, sdkCount, sdkName)
        sdksGenerated.append(sdkName)

    # If any SDKs were updated, and we were told to make a file to output
    # the list of updated SDKs to, then make said file and write out the list
    if len(sdksGenerated) > 0 and args.updatesFile:
        try:
            with open(args.updatesFile, 'w') as out:
                out.writelines('\n'.join(sdksGenerated))
        except:
            BuildThirdPartyUtils.printError("Failed to write list of updated SDKs to {0}. Backup zip files will not be created properly for this build.".format(args.updatesFile))

    return returnCode

if __name__ == "__main__":
    sys.exit(main())
