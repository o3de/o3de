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

import argparse
import os
import re
import shutil
import time
import zipfile
from urllib.parse import urlparse
from urllib.request import urlopen

import BuildInstallerUtils
import PackageExeSigning
import SignTool
import boto3


def getCloudfrontDistDomain(uploadURL):
    return urlparse(uploadURL)[1]


def getCloudfrontDistPath(uploadURL):
    pathFromDomainName = urlparse(uploadURL)[2]
    return pathFromDomainName[1:]   # need to remove the first slash, otherwise it will create a nameless directory on S3


def testSigningCredentials(args):
    # there are no credentials to test when certName was specified.
    if args.certName is not None:
        return True

    result = SignTool.signtoolTestCredentials(args.signingType,
                                              args.timestampServer,
                                              False)
    return result


defaultFilesToSign = ["dev/Tools/LmbrSetup/Win/SetupAssistant.exe",
                      "dev/Tools/LmbrSetup/Win/SetupAssistantBatch.exe",
                      "dev/Bin64vc141/ProjectConfigurator.exe",
                      "dev/Bin64vc141/lmbr.exe",
                      "dev/Bin64vc141/Lyzard.exe",
                      "dev/Bin64vc141/Editor.exe",
                      "dev/Bin64vc142/ProjectConfigurator.exe",
                      "dev/Bin64vc142/lmbr.exe",
                      "dev/Bin64vc142/Lyzard.exe",
                      "dev/Bin64vc142/Editor.exe"]

defaultFilesToSignHelpText = 'Additional files to sign, if signing. (default {})'.format(', '.join(defaultFilesToSign))


def createArgs():
    parser = argparse.ArgumentParser(description='Builds the WiX based Lumberyard Installer for Windows.')
    parser.add_argument('--packagePath', required=True, help="Path to package, can be url or local.")
    parser.add_argument('--workingDir', default="%TEMP%/installerAuto", help="Working directory (default '%%TEMP%%/installerAuto')")
    parser.add_argument('--allowedEmptyFolders', default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "allowed_empty_folders.json"), help="The JSON file containing the whitelist of empty folders that we expect in the source package.")
    parser.add_argument('--targetURL', required=True, help="Target URL to download the installer from.")
    parser.add_argument('--awsProfile', default=None, help='The aws cli profile to use to read from from s3 and cloudfront, and upload to s3. (Default None)') # if on a build machine, it will use the IAM role, if local, it will use [default] in aws credentials file.
    parser.add_argument('--lyVersion', default=None, help='Specifies the version used to identify the version of LY installed by this installer. Use of this field will ignore the default behavior of reading the value for this field from the default_settings.json file in the package. (DO NOT USE unless you know what you are doing.)')
    parser.add_argument('--suppressVersionInPath', action='store_true', help="Suppresses modification to the target paths with a version (default False)")
    parser.add_argument('-bi', '--addBuildIdToPath', action='store_true', help="Add the build version to the target paths prepending to the version of Lumberyard, i.e. buildId/version/installer. (default False)")
    parser.add_argument('--privateKey', default=None, help="The signing private key to use to sign the output of this script. Will only attempt to sign if this switch or --certName is specified. Use only one of these two switches. (default None)")
    parser.add_argument('--certName', default=None, help="The subject name of the signing certificate to use to sign with. Will only attempt to sign if this switch or --privateKey is specified. Use only one of these two switches. (default None)")
    parser.add_argument('-v', '--verbose', action='store_true', help='Enables logging messages (default False)')
    parser.add_argument('-k', '--keep', action='store_true', help='Keeps temp files (default False)')
    parser.add_argument('--timestampServer', default="http://tsa.starfieldtech.com", help="The timestamp server to use for signing. (default http://tsa.starfieldtech.com)")
    parser.add_argument('--filesToSign', nargs='+', default=defaultFilesToSign, help=defaultFilesToSignHelpText)
    args, unknown = parser.parse_known_args()
    print("Installer automation arguments:")
    print(args)
    return args


def validateArgs(args):
    args.signingPassword = None
    assert (os.path.exists(args.allowedEmptyFolders)), 'The whitelist file specified at {} does not exist.'.format(args.allowedEmptyFolders)
    # don't allow ambiguity of which way to sign. Have to do this here as we need to only create one SignType to keep in the params object
    assert (args.privateKey is None or args.certName is None), "Both a private key and a certificate name was provided, introducing ambiguity. Please only specify one way to sign."

    args.signingType = None
    if args.privateKey is not None:
        # get password for signing
        import getpass
        args.signingPassword = getpass.getpass("Please provide the signing password: ")
        args.signingType = SignTool.KeySigning(args.privateKey, args.signingPassword)
    elif args.certName is not None:
        args.signingType = SignTool.NameSigning(args.certName)
    args.doSigning = args.signingType is not None

    if args.doSigning is True:
        # Signing requires administration privileges.
        import ctypes
        try:
            is_admin = os.getuid() == 0
        except AttributeError:
            is_admin = ctypes.windll.shell32.IsUserAnAdmin() != 0
        assert is_admin, "Administrator privileges must be enabled to sign an installer."
        assert (testSigningCredentials(args)), "Signing password is incorrect. Failed to sign and verify test file."

    if args.awsProfile:
        if args.awsProfile is "":   # ANT jobs might pass an empty string to represent None, since ant doesn't have a concept of None or null
            args.awsProfile = None
        assert (boto3.Session(profile_name=args.awsProfile) is not None), "The AWS CLI profile name specified does not exist on this machine. Please specify an existing AWS CLI profile."

    if args.lyVersion:
        # check to make sure the value of lyVersion matches the format #.#.#.#
        r = re.compile(r"\d+\.\d+\.\d+\.\d+")
        assert (r.match(args.lyVersion) is not None), "The value of lyVersion given is not in the form of '<Product>.<Major>.<Minor>.<Patch>'. Please input a version with the correct format."


def run(args):
    expandedWorkingDir = os.path.expandvars(args.workingDir)
    expandedPackagePath = os.path.expandvars(args.packagePath)

    unpackedLocation = os.path.join(expandedWorkingDir, 'unpacked')
    fileName = BuildInstallerUtils.get_package_name(expandedPackagePath)
    downloadFileOnDisk = os.path.join(expandedWorkingDir, fileName)

    # Make sure temp directories exist
    BuildInstallerUtils.verbose_print(args.verbose, "Cleaning temp working directories")
    if not os.path.exists(expandedWorkingDir):
        os.makedirs(expandedWorkingDir)

    if os.path.exists(unpackedLocation):
        shutil.rmtree(unpackedLocation)

    os.makedirs(unpackedLocation)

    if os.path.isfile(downloadFileOnDisk):
        os.remove(downloadFileOnDisk)

    isDownloadFileTemp = False
    # 1. Download zip from S3 if it is an URL
    if BuildInstallerUtils.is_url(expandedPackagePath):
        isDownloadFileTemp = True
        BuildInstallerUtils.verbose_print(args.verbose, "Downloading package {}".format(expandedPackagePath))
        package = urlopen(expandedPackagePath)
        with open(downloadFileOnDisk, 'wb') as output:
            output.write(package.read())
    elif os.path.isfile(expandedPackagePath):
        BuildInstallerUtils.verbose_print(args.verbose, "using on disk package at {}".format(expandedPackagePath))
        downloadFileOnDisk = expandedPackagePath
    else:
        raise Exception('Could not find package "{}" at path {}'.format(fileName, expandedPackagePath))

    # 2. Unzip zip file.
    BuildInstallerUtils.verbose_print(args.verbose, "Unpacking package to {}".format(unpackedLocation))
    z = zipfile.ZipFile(downloadFileOnDisk, "r")
    z.extractall(unpackedLocation)
    # Preserver file's original timestamp
    for f in z.infolist():
        name, date_time = f.filename, f.date_time
        name = os.path.join(unpackedLocation, name)
        date_time = time.mktime(date_time + (0, 0, -1))
        os.utime(name, (date_time, date_time))
    z.close()

    #  Sign exes in Lumberyard
    if args.privateKey is not None or args.certName is not None:
        PackageExeSigning.SignLumberyardExes(unpackedLocation,
                                             args.signingType,
                                             args.timestampServer,
                                             args.verbose,
                                             args.filesToSign)

    # 3. Discover Lumberyard version.
    buildId = os.path.splitext(fileName)[0]
    packageVersion = BuildInstallerUtils.get_ly_version_from_package(args, unpackedLocation)
    version = packageVersion
    if args.lyVersion:
        version = args.lyVersion
        BuildInstallerUtils.verbose_print(args.verbose, "Package version is {}, but forcing version to value given for --lyVersion of {}".format(packageVersion, args.lyVersion))

    BuildInstallerUtils.verbose_print(args.verbose, "Building installer for Lumberyard v{}".format(version))

    # 4. Build installer.
    pathToBuild = os.path.join(expandedWorkingDir, version)

    targetUrl = BuildInstallerUtils.generate_target_url(args.targetURL, version, buildId, args.suppressVersionInPath, args.addBuildIdToPath)

    pathToDirFilelist = os.path.dirname(os.path.realpath(__file__)) + os.sep + 'dir_filelist.json'

    # take the name of the package without the file extension to use as the buildId
    buildCommand = "python BuildInstaller.py --packageRoot {} " \
                   "--lyVersion {} " \
                   "--genRoot {} " \
                   "--hostURL {} " \
                   "--allowedEmptyFolders {} " \
                   "--buildId {} " \
                   "--dirFilelist {}".format(unpackedLocation, version, pathToBuild, targetUrl,
                                             args.allowedEmptyFolders, buildId, pathToDirFilelist)

    if args.verbose:
        buildCommand += " -v"
    if args.keep:
        buildCommand += " --keep"
    if args.privateKey is not None:
        buildCommand += " --privateKey {} --password {}".format(args.privateKey, args.signingPassword)
    elif args.certName is not None:
        buildCommand += ' --certName "{}"'.format(args.certName)
    if args.doSigning:
        buildCommand += " --timestampServer {}".format(args.timestampServer)

    BuildInstallerUtils.verbose_print(args.verbose, "Creating build of installer with command:")
    BuildInstallerUtils.verbose_print(args.verbose, buildCommand)
    build_result = os.system(buildCommand)
    assert(build_result == 0), "Running BuildInstaller.py failed with result {}".format(build_result)
    BuildInstallerUtils.verbose_print(args.verbose, "Installer creation completed, build is available at {}".format(pathToBuild))

    # 5. Upload to the proper S3 bucket
    # Get the Cloudfront Distribution ID from the URL we expect to download from (targetUrl)
    BuildInstallerUtils.verbose_print(args.verbose, "Beginning upload of installer to S3")
    session = boto3.Session(profile_name=args.awsProfile)
    client = session.client('cloudfront')
    targetDomain = getCloudfrontDistDomain(targetUrl)
    distributionList = client.list_distributions()
    targetDistId = None
    for distribution in distributionList["DistributionList"]["Items"]:
        if distribution["DomainName"] == targetDomain:
            targetDistId = distribution["Id"]
            pass
    assert (targetDistId is not None), "No distribution with the domain name {} found.".format(targetDomain)

    # Get the s3 bucket info from the Distribution ID, and figure out where we are putting files in the bucket
    targetDist = client.get_distribution(Id=targetDistId)
    s3Info = targetDist["Distribution"]["DistributionConfig"]["Origins"]["Items"][0]
    bucketDomainName = s3Info["DomainName"]
    bucketName = bucketDomainName.split('.')[0] # first part of the domain name is the bucket name
    BuildInstallerUtils.verbose_print(args.verbose, "S3 bucket associated with targetUrl: {}".format(bucketName))
    originPath = s3Info["OriginPath"]
    bucketPath = None
    if originPath:
        # Start originPath after the first character (presumed to be '/') to avoid nameless directory in S3.
        bucketPath = '{}/{}'.format(originPath[1:], getCloudfrontDistPath(targetUrl))
    else:
        bucketPath = getCloudfrontDistPath(targetUrl)
    BuildInstallerUtils.verbose_print(args.verbose, "Uploading completed installer to S3 location: {}/{}".format(bucketName, bucketPath))

    # Upload each file to the S3 bucket
    s3 = session.resource('s3')
    s3Bucket = s3.Bucket(bucketName)
    installerOutputDir = None
    if args.signingType is not None:
        installerOutputDir = os.path.join(pathToBuild, "installer")
    else:
        installerOutputDir = os.path.join(pathToBuild, "unsignedInstaller")
    for file in os.listdir(installerOutputDir):
        fullFilePath = os.path.join(installerOutputDir, file)
        targetBucketPath = '{}/{}'.format(bucketPath, os.path.basename(file))
        s3Bucket.upload_file(fullFilePath, targetBucketPath)

    if not args.keep:
        if os.path.isfile(downloadFileOnDisk) and isDownloadFileTemp:
            os.remove(downloadFileOnDisk)


def main():
    args = createArgs()
    validateArgs(args)
    run(args)


if __name__ == "__main__":
    main()
