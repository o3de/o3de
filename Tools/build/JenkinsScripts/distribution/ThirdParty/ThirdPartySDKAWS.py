"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os.path
import sys
import urlparse
import SDKPackager
import BuildThirdPartyUtils

importDir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(importDir, "..")) #Required for AWS_PyTools
from AWS_PyTools import LyCloudfrontOps

def getS3StagingPath(stagingFolderPath, sdkPath):
    # The staging path is used for the build machines to upload new builds of 3rd party packages.
    # The Setup Assistant supports a global override so Lumberyard team members can pull from the staging location.
    return urlparse.urljoin(stagingFolderPath, sdkPath.replace(' ', '_')) + '/'

def getProductionUrl(cloudfrontUrl, sdkPath, filePath):
    # The production path is the final path customers will download the SDK from.
    # These links get baked into the manifest, which the Setup Assistant executable uses to acquire these SDKs.
    # TODO : Generate an actual cloudfront production link.
    #return cloudfrontUrl + getS3StagingPath(stagingFolderPath, sdkPath.replace(' ', '_')) + os.path.basename(filePath)
    # we assume that the cloudfront url and the result of getS3stagingPath have a trailing '/'
    return getS3StagingPath(cloudfrontUrl, sdkPath) + os.path.basename(filePath)

def getSDKStagingStatus(bucket, baseBucketPath, sdkPath, sdkPlatform):

    pathToSDK = getS3StagingPath(baseBucketPath, sdkPath)
    pathToFilelist = pathToSDK + SDKPackager.getFilelistFileName(sdkPlatform)
    pathToManifest = pathToSDK + SDKPackager.getManifestFileName(sdkPlatform)
    manifestExists = False
    filelistExists = False

    objs = list(bucket.objects.filter(Prefix=pathToManifest))
    if len(objs) > 0 and objs[0].key == pathToManifest:
        manifestExists = True
    else:
        manifestExists = False

    objs = list(bucket.objects.filter(Prefix=pathToFilelist))
    if len(objs) > 0 and objs[0].key == pathToFilelist:
        filelistExists = True
    else:
        filelistExists = False

    return manifestExists, filelistExists

def uploadSDKToStaging(bucket, baseBucketPath, sdkName, sdkVersion, sdkPath, sdkPlatform, manifestPath, filelistPath, zipFiles):
    print "\tUploading SDK: {0} ({1},{2})".format(sdkName, sdkVersion, sdkPlatform)

    manifestStagingPath = getS3StagingPath(baseBucketPath, sdkPath) + os.path.basename(manifestPath)
    bucket.upload_file(manifestPath, manifestStagingPath)
    print "\tUploaded manifest"

    filelistStagingPath = getS3StagingPath(baseBucketPath, sdkPath) + os.path.basename(filelistPath)
    bucket.upload_file(filelistPath, filelistStagingPath)
    print "\tUploaded filelist"

    filesUploaded = 0
    totalFilesCount = len(zipFiles)
    for zipFile in zipFiles:
        fileLocalPath = zipFile.filePath
        fileStagingPath = getS3StagingPath(baseBucketPath, sdkPath) + os.path.basename(zipFile.filePath)
        bucket.upload_file(fileLocalPath, fileStagingPath)
        filesUploaded += 1
        BuildThirdPartyUtils.reportIterationStatus(filesUploaded, totalFilesCount, 5, "files uploaded")
