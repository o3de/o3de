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
import BuildInstallerUtils
from SignTool import *
import os
import tempfile
import ctypes


def createArgs():
    defaultTempLocation = os.path.join(tempfile.gettempdir(), "LYPackage")

    parser = argparse.ArgumentParser(description='Builds the WiX based Lumberyard Installer for Windows.')
    # PRIMARY ARGS - many if not all will be used in production
    # INPUT/OUTPUT
    parser.add_argument('--packageRoot', default=None, help="Path to the root of the package (default None)")
    parser.add_argument('--genRoot', default=defaultTempLocation, help="Path for temp data (default Python's temp directory + '/LYPackage')")
    parser.add_argument('--allowedEmptyFolders', default=None, help="Path to a JSON file containing a list of empty folders that are allowed to exist in --packageRoot")
    # The default is applied in Build
    parser.add_argument('--bootstrapName', default=None, help="Bootstrap name (default 'LumberyardInstaller{}.exe'.format(args.lyVersion))")
    parser.add_argument('--metricsExe', default=None, help="Override path to metrics executable, if not provided this script will search for LyInstallerMetrics.exe in the package root (default None)")
    # VERSION & GUID
    parser.add_argument('--lyVersion', default="0.2.2.1", help="Lumberyard Version (default '0.2.2.1')")
    parser.add_argument('--buildId', default=None, help="The build number of the package that the installer was made from. If not provided, there will be no version file created in the output. (default None)")
    # SIGNING
    # more information on the step by step details can be found at:
    #   https://wiki.labcollab.net/confluence/display/lmbr/Sign+Lumberyard+Binaries
    parser.add_argument('--privateKey', default=None, help="The signing private key to use to sign the output of this script. Will only attempt to sign if this switch or --certName is specified. Use only one of these two switches. (default None)")
    parser.add_argument('--password', default=None, help="The password for using the signing private key. Must include this if signing should occur. (default None)")
    parser.add_argument('--certName', default=None, help="The subject name of the signing certificate to use to sign with. Will only attempt to sign if this switch or --privateKey is specified. Use only one of these two switches. (default None)")
    parser.add_argument('--timestampServer', default="http://tsa.starfieldtech.com", help="The timestamp server to use for signing. (default http://tsa.starfieldtech.com)")
    # VERBOSE OUTPUT
    parser.add_argument('-v', '--verbose', action='store_true', help='Enables logging messages (default False)')
    # URL INFO
    parser.add_argument('--hostURL', default=None, help='The URL for the installer to download its packages from (msi + cab files). No URL implies the files will be on local disk already. (default None)')
    # RAPID ITERATION ARGS
    parser.add_argument('--bootstrapOnly', action='store_true', help="Only create a bootstrapper. Will assume packageRoot contains all necessary MSIs and CABs. Ignores cabCachePath if it was provided.")
    parser.add_argument('--signOnly', action="store_true", help="Will sign already existing CAB and MSI files. Will then generate a bootstrapper with those signed MSIs, and sign the bootstrapper.")
    parser.add_argument('--cabCachePath', default=None, help='Path to a cache of the cab files. Should only be used if no new packages are being added, and everything already has cabs built. (default None)')
    parser.add_argument('-k', '--keep', action='store_true', help="Don't delete temp files")
    parser.add_argument('--dirFilelist', default=None, help="A list of files (by directory) to include in the install content.")
    args = parser.parse_args()
    print("Installer arguments:")
    print(args)

    # don't allow ambiguity of which way to sign. Have to do this here as we need to only create one SignType to keep in the params object
    assert (args.privateKey is None or args.certName is None), "Both a private key and a certificate name was provided, introducing ambiguity. Please only specify one way to sign."

    return args


def validateArgs(args, params):
    assert (args.hostURL is not None), "No URL to provide to the bootstrapper was given. Please use --hostURL to specify where the bootstrapper will download the MSI and CAB files from."

    # find empty folders
    BuildInstallerUtils.check_for_empty_subfolders(args.packageRoot, args.allowedEmptyFolders)
    
    # BootstrapOnly validation
    if args.bootstrapOnly:
        assert (not args.cabCachePath), "Error: Ignoring cabCachePath since bootstrapOnly was specified. Please choose one to use."
        if args.packageRoot:
            print("Warning: Specifying packageRoot with bootstrapOnly. Base package not used in bootstrapOnly builds.")
        # make sure there are MSI files in the installer directory
        hasInstallerFiles = False
        for file in os.listdir(params.intermediateInstallerPath):
            hasInstallerFiles = file.endswith(".msi") or file.endswith(".cab")
            if hasInstallerFiles:
                return
        assert hasInstallerFiles, "The packageRoot given ({}) does not contain any MSI or CAB files. When using bootstrapOnly, packageRoot should point to the directory with MSIs and CAB files.".format(params.intermediateInstallerPath)

    # Make sure metrics and Visual Studio 2015-2019 redist exist somewhere
    assert (params.fullPathToMetrics is not None), "Metrics executable path was not provided and was not found in the package."
    assert (os.path.exists(params.fullPathToMetrics)), "Metrics executable expected at {}, but cannot be found.".format(params.fullPathToMetrics)
    assert (params.fullPathTo2015Thru2019Redist is not None), "Visual Studio 2015-2019 redist could not be found in the package."
    assert (os.path.exists(params.fullPathTo2015Thru2019Redist)), "Visual Studio 2015-2019 redist expected at {}, but cannot be found.".format(params.fullPathTo2015Thru2019Redist)

    # Signing parameter asserts
    if params.doSigning:
        # Installation requires administration privileges.
        try:
            is_admin = os.getuid() == 0
        except AttributeError:
            is_admin = ctypes.windll.shell32.IsUserAnAdmin() != 0
        assert is_admin, "Administrator privileges must be enabled to sign an installer."

        if args.privateKey is not None:
            # make sure that private key is valid and we were given a password to use.
            assert (os.path.exists(args.privateKey)), "No private key exists at the given path."
            assert (args.privateKey.endswith(".pfx")), "The file at {} is not a signing private key.".format(args.privateKey)
            assert (args.password is not None), "Must include the password needed for the signing private key to sign."
        # if --certName is specified, the verification of that name being valid must be done prior to this script being run, as it is platform dependent.

        # Make sure that all CABs and MSI files are signed if only the bootstrapper needs to be built.
        if args.bootstrapOnly:
            for filename in params.msiFileNameList:
                signed = signtoolVerifySign(os.path.join(params.installerPath, filename), args.verbose)
                assert signed, "Not all MSI and CAB files in {} are verified to have been signed. Please use --signOnly instead.".format(params.installerPath)
            for filename in params.cabFileNameList:
                signed = signtoolVerifySign(os.path.join(params.installerPath, filename), args.verbose)
                assert signed, "Not all MSI and CAB files in {} are verified to have been signed. Please use --signOnly False instead.".format(params.installerPath)
            signed = signtoolVerifySign(params.fullPathToMetrics, args.verbose)
            assert signed, "Metrics executable at {} is not verified to have been signed. Please either create a signed executable or use --signOnly instead.".format(params.fullPathToMetrics)

    if args.signOnly:
        assert params.doSigning, "Cannot specify --signOnly if not given a private key and password, or a certificate name for signing."
