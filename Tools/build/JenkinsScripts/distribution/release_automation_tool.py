"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from Installer import InstallerAutomation
from ThirdParty import thirdparty_bucket_fetch
import os

def main():
    # make sure that current working directory is the directory that this
    #   script lives in
    abspath = os.path.abspath(__file__)
    dname = os.path.dirname(abspath)
    os.chdir(dname)
    
    # parse InstallerAutomation args from execution of this script
    os.chdir("Installer")
    installerArgs = InstallerAutomation.createArgs()
    InstallerAutomation.validateArgs(installerArgs)
    os.chdir("..")

    # If we succeed InstallerAutomation validation (we would have asserted otherwise),
    #   then parse thirdparty_bucket_fetch args.
    os.chdir("ThirdParty")
    ladPackageArgs = thirdparty_bucket_fetch.parse_args()
    ladPackageParams = thirdparty_bucket_fetch.PromoterParams(ladPackageArgs)
    os.chdir("..")

    # if we succeeded that one too, then it is safe to run the scripts themselves
    os.chdir("Installer")
    InstallerAutomation.run(installerArgs)

    os.chdir("../ThirdParty")
    thirdparty_bucket_fetch.run(ladPackageArgs, ladPackageParams)

    os.chdir("..")

if __name__ == "__main__":
    main()
