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
import boto3
try:
    import urllib.parse as urllib_compat
except:
    import urlparse as urllib_compat

def getCloudfrontDistPath(uploadURL):
    pathFromDomainName = urllib_compat.urlparse(uploadURL)[2]
    # need to remove the first slash, otherwise it will create a nameless directory on S3
    return pathFromDomainName[1:]


def getCloudfrontDistribution(cloudfrontUrl, awsCredentialProfileName):
    session = boto3.session.Session(profile_name=awsCredentialProfileName)
    cloudfront = session.client('cloudfront')
    distributionList = cloudfront.list_distributions()
    targetDistId = None
    for distribution in distributionList["DistributionList"]["Items"]:
        if distribution["DomainName"] == urllib_compat.urlparse(cloudfrontUrl)[1]:
            targetDistId = distribution["Id"]
            pass
    assert (targetDistId is not None), "No distribution with the domain name {} found.".format(cloudfrontUrl)
    targetDist = cloudfront.get_distribution(Id=targetDistId)
    return targetDist


def getBucket(cloudfrontDistribution, awsCredentialProfileName):
    bucketName = getBucketName(cloudfrontDistribution)
    session = boto3.session.Session(profile_name=awsCredentialProfileName)
    s3 = session.resource('s3')
    return s3.Bucket(bucketName)


def getBucketName(cloudfrontDistribution):
    s3Info = cloudfrontDistribution["Distribution"]["DistributionConfig"]["Origins"]["Items"][0]
    bucketDomainName = s3Info["DomainName"]
    return bucketDomainName.split('.')[0] # first part of the domain name is the bucket name


def buildBucketPath(cloudfrontUrl, cloudfrontDistribution):
    s3Info = cloudfrontDistribution["Distribution"]["DistributionConfig"]["Origins"]["Items"][0]
    originPath = s3Info["OriginPath"]
    bucketPath = None
    if originPath:
        # Start originPath after the first character (presumed to be '/') to avoid nameless directory in S3.
        bucketPath = str.format("{0}/{1}", originPath[1:], getCloudfrontDistPath(cloudfrontUrl))
    else:
        bucketPath = getCloudfrontDistPath(cloudfrontUrl)
    return bucketPath


def uploadFileToCloudfrontURL(absFilePath, cloudfrontBaseUrl, awsCredentialProfileName, overwrite):
    cloudfrontDist = getCloudfrontDistribution(cloudfrontBaseUrl, awsCredentialProfileName)
    s3Bucket = getBucket(cloudfrontDist, awsCredentialProfileName)
    s3BucketPath = buildBucketPath(cloudfrontBaseUrl, cloudfrontDist)
    targetBucketPath = urllib_compat.urljoin(s3BucketPath, os.path.basename(absFilePath))

    # Check if file already exists in the S3 bucket.
    file_exists = len(list(s3Bucket.objects.filter(Prefix=targetBucketPath))) > 0
    if not file_exists or overwrite:
        s3Bucket.upload_file(absFilePath, targetBucketPath)

    return targetBucketPath
