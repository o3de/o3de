"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import zipfile
import os.path
import re
import json
import sys
import ThirdPartySDKAWS
import BuildThirdPartyUtils
import tempfile

importDir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(importDir, "..")) #Required for AWS_PyTools
from AWS_PyTools import LyChecksum

class SDKZipFile(object):
    def __init__(self):
        self.file = None
        self.filePath = None
        self.contents = []
        self.compressedSize = 0
        self.uncompressedSize = 0
        self.compressedHash = 0


def getFilelistVersion():
    return "1.0.0"


def getFilelistFileName(sdkPlatform):
    return "filelist." + getFilelistVersion() + "." + sdkPlatform + ".json"


def getManifestVersion():
    return "1.0.0"


def getManifestFileName(sdkPlatform):
    return "manifest." + getManifestVersion() + "." + sdkPlatform + ".json"


def toArchivePath(filePath):
    # Archives are generated relative to 3rdParty folder, so strip everything before that in the path.
    # zipfile is very particular in how paths to files within it are formatted.
    slashesFixed = filePath.replace('\\', '/')
    match = re.search("3rdParty/(.*)", slashesFixed)
    if not match:
        BuildThirdPartyUtils.printError("Could not find third party folder in file path {0}".format(filePath))
    archivePath = match.group(1).strip("/").strip("\n")
    return archivePath


def prepFilesystemForFile(filePath):
    directory = os.path.dirname(filePath)
    if not os.path.exists(directory):
        os.makedirs(directory)
    if os.path.isfile(filePath):
        os.remove(filePath)

def createJSONString(dataToFormat):
    return json.dumps(dataToFormat, sort_keys=True, indent=4, separators=(',', ': '))


def generateSDKPackage(baseCloudfrontUrl,
                       sdkName,
                       sdkVersion,
                       sdkPath,
                       sdkPlatform,
                       filesetHash,
                       filePaths,
                       archiveMaxSize):
    zipFiles = zipPackage(sdkName, sdkVersion, sdkPath, sdkPlatform, filePaths, archiveMaxSize)
    filelistPath = buildFilelistJSON(sdkPath, sdkPlatform, filesetHash, filePaths)
    manifestPath = buildManifestJSON(baseCloudfrontUrl,
                                     sdkName,
                                     sdkPath,
                                     sdkPlatform,
                                     zipFiles,
                                     filelistPath)
    return manifestPath, filelistPath, zipFiles


def getTempDir(sdkPath, sdkPlatform):
    tempDir = os.path.join(tempfile.tempdir, "LY", "3rdPartySDKs", sdkPath, sdkPlatform)
    return os.path.expandvars(tempDir)


def zipPackage(sdkName, sdkVersion, sdkPath, sdkPlatform, filePaths, archiveMaxSize):
    try:
        import zlib
        compression = zipfile.ZIP_DEFLATED
    except:
        compression = zipfile.ZIP_STORED

    tempDir = getTempDir(sdkPath, sdkPlatform)
    zipFiles = []
    currentZipFile = None

    fileIndex = 0
    fileCount = len(filePaths)

    for filePath in filePaths:

        filePath = filePath.strip("\n")
        archivePath = toArchivePath(filePath)
        if currentZipFile is None or currentZipFile.file is None or currentZipFile.compressedSize > archiveMaxSize:
            if currentZipFile and currentZipFile.file:
                currentZipFile.file.close()
            currentZipFile = SDKZipFile()
            zipFiles.append(currentZipFile)
            currentZipFile.filePath = os.path.join(tempDir, sdkName + "." + str(sdkPlatform) + "." + str(len(zipFiles)) + ".zip")
            prepFilesystemForFile(currentZipFile.filePath)
            currentZipFile.file = zipfile.ZipFile(currentZipFile.filePath, mode='w')
        currentZipFile.file.write(filePath, compress_type=compression, arcname=archivePath)
        fileInfo = currentZipFile.file.getinfo(archivePath)
        currentZipFile.compressedSize += fileInfo.compress_size
        currentZipFile.uncompressedSize += fileInfo.file_size
        currentZipFile.contents.append(filePath)

        fileIndex += 1
        BuildThirdPartyUtils.reportIterationStatus(fileIndex, fileCount, 25, "files zipped")

    if currentZipFile and currentZipFile.file:
        currentZipFile.file.close()

    for zipFile in zipFiles:
        zipFile.compressedHash = LyChecksum.getChecksumForSingleFile(zipFile.filePath)

    return zipFiles


def buildFilelistJSON(sdkPath, sdkPlatform, filesetHash, filePaths):
    jsonFormatFiles = []
    for filePath in filePaths:
        scrubbedFile = toArchivePath(filePath)
        jsonFormatFiles.append(scrubbedFile)

    filelistInfo = {
        "filelist": {
            "filelistVersion": getFilelistVersion(),
            "checksum": filesetHash.hexdigest(),
            "files": jsonFormatFiles,
        }
    }
    filelistJSON = createJSONString(filelistInfo)
    filelistName = getFilelistFileName(sdkPlatform)
    filelistPath = getTempDir(sdkPath, sdkPlatform)
    filelistFullPath = os.path.join(filelistPath, filelistName)

    prepFilesystemForFile(filelistFullPath)

    outputFile = open(filelistFullPath, 'w')
    outputFile.write(filelistJSON)
    outputFile.close()
    return filelistFullPath


def buildManifestJSON(baseCloudfrontUrl, sdkName, sdkPath, sdkPlatform, zipFiles, filelistPath):
    uncompressedSize = 0
    packageArchives = []
    for zipFile in zipFiles:
        uncompressedSize += zipFile.uncompressedSize
        archiveUrl = ThirdPartySDKAWS.getProductionUrl(baseCloudfrontUrl, sdkPath, zipFile.filePath)
        packageArchive = {
            "archiveUrl": archiveUrl,
            "archiveSize": str(zipFile.compressedSize),
            "archiveChecksum": zipFile.compressedHash.hexdigest()
        }
        packageArchives.append(packageArchive)

    filelistChecksum = LyChecksum.getChecksumForSingleFile(filelistPath)

    filelistUrl = ThirdPartySDKAWS.getProductionUrl(baseCloudfrontUrl, sdkPath, filelistPath)
    packageInfo = {
        "package":
        {
            "manifestVersion": getManifestVersion(),
            "identifier": sdkName,
            "platform": sdkPlatform,
            "uncompressedSize": str(uncompressedSize),
            "filelistUrl": filelistUrl,
            "filelistChecksum": filelistChecksum.hexdigest(),
            "archives": packageArchives

        }
    }
    manifestJSON = createJSONString(packageInfo)
    manifestName = getManifestFileName(sdkPlatform)
    manifestPath = getTempDir(sdkPath, sdkPlatform)
    manifestFullPath = os.path.join(manifestPath, manifestName)

    prepFilesystemForFile(manifestFullPath)

    outputFile = open(manifestFullPath, 'w')
    outputFile.write(manifestJSON)
    outputFile.close()
    return manifestFullPath
