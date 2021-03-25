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
import ctypes
import filecmp
import os
import os.path
import sys
import tempfile

parser = argparse.ArgumentParser(description='Tests the Lumberyard Installer bootstrapper.')
parser.add_argument('-v', '--verbose', action='store_true', help='Enables logging messages (default False)')
parser.add_argument('-k', '--keep', action='store_true', help="Don't delete temp files")
parser.add_argument('--packageRoot', default=None, help="Source content to test against, required for use. (default None)")
parser.add_argument('--lyVersion', default="0.0.0.0", help="Version to use when generated artifacts (default '0.0.0.0')")
parser.add_argument('--target', default='%temp%\\installertest\\TestInstall', help='The location to install Lumberyard. Make sure to use backslashes in the path. (default "%%temp%%\\installertest\\TestInstall"')
parser.add_argument('--genRoot', default='%temp%/installertest/', help='Path for temp data (default "%%temp%%/installertest/"')
parser.add_argument('--hostURL', default="https://s3-us-west-2.amazonaws.com/lumberyard-streaming-install-test/releases/JoeInstallTest/", help='The URL for the installer to download its packages from (msi + cab files). (default https://s3-us-west-2.amazonaws.com/lumberyard-streaming-install-test/releases/JoeInstallTest/)')
parser.add_argument('--testName', default=None, help='The name of the test to run (case insensitive). Will cause an error if no test with that name exists. Will run all tests if not specified. (default None)')
bootstrapperName = "automatedTestBootstrapper.exe"

args = parser.parse_args()

# Installation requires administration privileges.
try:
    is_admin = os.getuid() == 0
except AttributeError:
    is_admin = ctypes.windll.shell32.IsUserAnAdmin() != 0

if not is_admin:
    sys.exit("Administrator privileges must be enabled to install Lumberyard")

# Search target for forward slash (/) and replace with back slashes (\)
# because the install command doesn't like forward slashes.
installTarget = args.target.replace('/', '\\')

pfxName = "LumberyardDev.pfx"
signingArgs = "--privateKey {} --password test".format(pfxName)


# Creates an installer with the given additional arguments. Will leave temp files
#   even if the keep argument for this script is specified, in order to correctly
#   perform some tests
def makeInstaller(additionalCommands=None):
    buildInstallerCommand = "BuildInstaller.py --packageRoot {} " \
                            "--lyVersion {} " \
                            "--genRoot {} " \
                            "--bootstrapName {} " \
                            "--hostURL {}".format(args.packageRoot,
                                                  args.lyVersion,
                                                  args.genRoot,
                                                  bootstrapperName,
                                                  args.hostURL)
    if args.verbose:
        buildInstallerCommand += " -v"

    if additionalCommands:
        buildInstallerCommand += " " + additionalCommands

    os.system(buildInstallerCommand)


# Will create an installer with the given additional arguments, run the installer,
#   and verify contents of the installed lumberyard with the source package.
#   Some tests require artifacts of previous installs to exist. If they do not,
#   a set of artifacts will be created.
def createAndTestInstaller(installerSubfolder,
                           additionalCommands,
                           requiresSigning=False,
                           requiresArtifacts=False):
    if requiresArtifacts:
        # Create a set of artifacts if none are available
        if not os.path.exists(os.path.join(args.genRoot, installerSubfolder)):
            if requiresSigning:
                makeInstaller(signingArgs)
            else:
                makeInstaller()

    makeInstaller(additionalCommands)
    bootstrapperPath = os.path.join(args.genRoot, installerSubfolder, bootstrapperName)
    installCommand = "{} /silent InstallFolder={}".format(bootstrapperPath, installTarget)
    if args.verbose:
        print("Running installation command:")
        print("\t{}".format(installCommand))
    os.system(installCommand)

    # Validate install was successful
    pathToInstalledPackage = os.path.join(installTarget, args.lyVersion)
    # filecmp.dircmp failes if you pass in a directory with "%temp%", so %temp% has to be replaced by
    # temp dir before calling it.
    scrubbedPackagedRoot = args.packageRoot.replace("%temp%", tempfile.gettempdir())
    pathToInstalledPackage = pathToInstalledPackage.replace("%temp%", tempfile.gettempdir())
    installVerifier = filecmp.dircmp(scrubbedPackagedRoot, pathToInstalledPackage)
    if args.verbose:
        print("Dif between install and source:")
        installVerifier.report_full_closure()
    if installVerifier.left_only or installVerifier.right_only:
        print("Error: Install failed, source and destination do not match")

    # Uninstall Lumberyard
    uninstallCommand = "start /WAIT {} /silent /uninstall".format(bootstrapperPath)
    if args.verbose:
        print("Running uninstallation command:")
        print("\t{}".format(uninstallCommand))
    os.system(uninstallCommand)

    # Validate uninstall was successful
    if os.path.isdir(args.target):
        print("Error: Uninstall failed.")


tests = {}


def makeTest(testName,
             installerSubfolder,
             additionalCommands,
             requiresSigning=False,
             requiresArtifacts=False):

    def _makeTestArgs(testName, *testArgs):
        tests[testName] = testArgs

    _makeTestArgs(testName, installerSubfolder, additionalCommands, requiresSigning, requiresArtifacts)


def listTests():
    print('Tests with the following names have been defined:')
    for testName in tests.keys():
        print(testName)


def runTest(testName):
    if args.verbose:
        print('Performing test "{}"'.format(testName))
    testArgs = tests[testName]
    if testArgs is not None and len(testArgs) > 0:
        createAndTestInstaller(*testArgs)


def runAllTests():
    for testName in tests.keys():
        runTest(testName)


# DEFINE TEST CASES

# Test success cases
# Unsigned
makeTest("unsigned installer", "unsignedInstaller", None)
makeTest("unsigned bootstrap only", "unsignedInstaller", "--bootstrapOnly", False, True)
# Signed
makeTest("signed installer", "installer", signingArgs, True)
makeTest("signed sign only", "installer", signingArgs + " --signOnly", True, True)
makeTest("signed bootstrap only", "installer", signingArgs + " --bootstrapOnly", True, True)

# END DEFINE TEST CASES


# RUN TEST(S)
if args.testName is not None:
    if args.testName.lower() in tests:
        runTest(args.testName.lower())
    else:
        print('Test with the name "{}" does not exist.'.format(args.testName))
        listTests()
else:
    runAllTests()
# END RUN TEST(S)


# CLEAN-UP
if not args.keep:
    import shutil
    if os.path.exists(args.genRoot):
        shutil.rmtree(args.genRoot)
# END CLEAN-UP
