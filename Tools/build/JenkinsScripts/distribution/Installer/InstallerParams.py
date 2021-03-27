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
from SignTool import *


class InstallerParamError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


class InstallerParams(object):
    def __init__(self, args):
        self.fullPathTo2015Thru2019Redist = None
        self.signingType = None

        if args.privateKey is not None:
            if not args.password:
                import getpass
                args.password = getpass.getpass("Please provide the signing password: ")
            self.signingType = KeySigning(args.privateKey, args.password)
        elif args.certName is not None:
            self.signingType = NameSigning(args.certName)

        self.doSigning = self.signingType is not None

        self.wxsRoot = os.path.join(args.genRoot, "wxs")
        self.wixObjOutput = os.path.join(args.genRoot, "wixobj_module")
        self.heatPackageBase = "HeatPackageBase.wxs"
        self.packagesPath = os.path.join(args.genRoot, "unsignedInstaller")
        # Wix primarily differentiates between a file and directory by looking for a trailing slash.
        # os.path.join with an empty string is a way to guarantee a trailing slash is added to a path.
        self.bootstrapWixObjDir = os.path.join(os.path.join(args.genRoot, "wixobj_bootstrap"), '')
        self.intermediateInstallerPath = os.path.join(args.genRoot, "unsignedInstaller")

        self.installerPath = self.intermediateInstallerPath
        if self.doSigning:
            self.installerPath = os.path.join(args.genRoot, "installer")

        self.sourcePath = args.packageRoot
        if args.bootstrapOnly:
            self.sourcePath = self.installerPath

        self.fullPathToMetrics = args.metricsExe
        if self.fullPathToMetrics is None:
            self.fullPathToMetrics = find_file_in_package(self.sourcePath, "LyInstallerMetrics.exe", ["InternalSDKs"])
        if self.fullPathToMetrics is None:
            raise InstallerParamError('Path to LyInstallerMetrics.exe could not be found underneath the Tools/InternalSDKs folder in the package ')

        # Gather information on the Visual Studio 2015-2019 redistributable.
        # Note that this is a hardcoded path, and not a search. This is because there are multiple different redistributables
        # with the exact same name, the easiest way to identify which is which is based on the location in the package.
        # Also, this VS 2015-2019 redistributable appears multiple times in the package. For consistency, we want to make sure
        # the exact same one is used.
        self.name2015Thru2019Redist = "VC_redist.x64.exe"
        self.fullPathTo2015Thru2019Redist = os.path.join(args.packageRoot, "dev", "Tools", "Redistributables", "Visual Studio 2015-2019", self.name2015Thru2019Redist)
        # When rebuilding just the bootstrap for a package, the source package root is a pointer to where the installer was
        # generated previously. At this point, the redistributable is already in the package root.
        if args.bootstrapOnly:
            self.fullPathTo2015Thru2019Redist = find_file_in_package(self.sourcePath, [self.name2015Thru2019Redist])

        self.skipMsiAndCabCreation = args.signOnly or args.bootstrapOnly
        self.metricsPath, self.metricsExe = os.path.split(self.fullPathToMetrics)
        self.pathTo2015Thru2019Redist, self.redist2015Thru2019Exe = os.path.split(self.fullPathTo2015Thru2019Redist)
        self.tempBootstrapOutputDir = os.path.join(args.genRoot, "buildBootstrap")

        if self.doSigning and args.bootstrapOnly:
            self.msiFileNameList = get_file_names_in_directory(self.installerPath, ".msi")
            self.cabFileNameList = get_file_names_in_directory(self.installerPath, ".cab")

        # Default to LumberyardInstallerVERSION.exe, unless args.bootstrapName is set
        if args.bootstrapName:
            self.bootstrapName = args.bootstrapName
        else:
            self.bootstrapName = "LumberyardInstaller{}.exe".format(args.lyVersion)

        if self.doSigning:
            self.tempBootstrapName = "temp" + self.bootstrapName
            self.bootstrapOutputPath = os.path.join(self.tempBootstrapOutputDir, self.tempBootstrapName)
        else:
            self.bootstrapOutputPath = os.path.join(self.installerPath, self.bootstrapName)
