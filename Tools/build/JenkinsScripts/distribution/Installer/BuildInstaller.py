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

import glob
import shutil
import InstallerParams
from Insignia import *
from InstallerArgs import *
from InstallerPackaging import *
from Light import *
from SignTool import *


# Per Installer:
#     Heat the includes to build the fragment
#     candle + light to build the installer / merge module / whatever
#     Collect the results
#     If Signing:
#         Copy installer and cabs to a clean folder
#         Sign the CAB files
#         insignia the installer to update its cab file references
#         Sign the installer
# For the Bootstrapper:
#     Generate S3 links or whatever data we need to shove into the final installer.
#     Generate a WXS file if we need to based on the above data
#     candle + light generated WXS + pre-built WXS to build the installer (from
#       the unsigned files or, if signing, the signed files of the installers)
#     If Signing:
#         Copy bootstrapper and cabs to a clean folder
#         Detach the burn engine from the bootstrapper, and sign it
#         Reattach the burn engine to the bootstrapper, and sign the bootstrapper
# After completion, clean up temp files if necessary.


class OperationMode:
    StepCounting, BuildInstaller = range(2)

    # It's good practice to define an init for a class in Python, but this
    # class only exists to serve as an enum for the operation mode.
    # Defining the __init__ with just a pass is a way to stub out this function so
    # Python IDEs don't get upset that there is a class with no init.
    def __init__(self):
        pass


args = createArgs()
try:
    params = InstallerParams.InstallerParams(args)
    validateArgs(args, params)
except InstallerParams.InstallerParamError as error:
    raise Exception('Installer Params failed to be created with error:\n{}'.format(error))

# Tracking this like this to simplify calls to performStep.
# Otherwise, it has to look like this:
#     def performStep(mode, stepsTaken, maxSteps, message, operation, *operationArgs):
#         return stepsTaken+1, result
# and calls to performStep have to also capture the stepsTaken.
stepsTaken = 0
maxSteps = 0
operationMode = OperationMode.StepCounting


# The goal of authoring this function is to simplify calls in, which reduces friction when using it.
# Call this when you want to only call a function during full operational mode, and not during step counting.
# This will use the function's name as the message for the printout.
def performStep(operation, *operationArgs):
    return performStepWithMessage(operation.__name__, operation, *operationArgs)


# Call when you want a message that does not match the function's name.
def performStepWithMessage(message, operation, *operationArgs):
    global stepsTaken
    result = None
    if operationMode == OperationMode.BuildInstaller:
        printProgress(message, stepsTaken, maxSteps)
        result = operation(*operationArgs)
    stepsTaken += 1
    return result


# Call when you want to handle branching yourself. Can be used to trigger a branch for the operation mode.
def describeStep(message):
    global stepsTaken
    if operationMode == OperationMode.BuildInstaller:
        printProgress(message, stepsTaken, maxSteps)
    stepsTaken += 1


def buildInstaller():
    global stepsTaken
    stepsTaken = 0
    # CREATE PACKAGES
    if not params.skipMsiAndCabCreation:
        performStep(createThirdPartyPackages, args, params)
        performStep(createDevPackage, args, params)
        performStep(createRootPackage, args, params)
    else:
        describeStep("Skipping Create Packages (MSIs and Cabs) step, re-using existing packages.")

    if not params.doSigning or not args.bootstrapOnly:
        params.msiFileNameList = performStepWithMessage("Gathering MSIs",
                                                        get_file_names_in_directory,
                                                        params.intermediateInstallerPath,
                                                        ".msi")
        params.cabFileNameList = performStepWithMessage("Gathering CABs",
                                                        get_file_names_in_directory,
                                                        params.intermediateInstallerPath,
                                                        ".cab")

    # SIGN CABs AND MSIs
    if params.doSigning and not args.bootstrapOnly:
        # we don't want to modify the original metrics exe, so copy it to where the
        #   other clean files are, and update the path to it
        if params.metricsPath is not params.intermediateInstallerPath:
            describeStep("Copying metrics to signing path")
            if operationMode == OperationMode.BuildInstaller:
                params.metricsPath = params.intermediateInstallerPath
                safe_shutil_file_copy(params.fullPathToMetrics, os.path.join(params.metricsPath, params.metricsExe))

        # copy the clean cab and msi files to a new directory to sign them.
        if params.installerPath is not params.intermediateInstallerPath:
            describeStep("Copying clean MSI, CAB, and EXE files to signing path")
            if operationMode == OperationMode.BuildInstaller:
                if os.path.exists(params.installerPath):
                    verbose_print(args.verbose, "Removing old files from signing path.")
                    shutil.rmtree(params.installerPath)
                shutil.copytree(params.intermediateInstallerPath, params.installerPath)
                # don't need the wixpdb files in the signing folder, so get rid of them
                for filepath in glob.glob(os.path.join(params.installerPath, "*.wixpdb")):
                    os.remove(filepath)

        # Sign the Cab files, verify signing was successful
        performStep(signtoolSignAndVerifyFiles,
                    params.cabFileNameList,
                    params.installerPath,
                    params.intermediateInstallerPath,
                    params.signingType,
                    args.timestampServer,
                    args.verbose)

        # Run Insignia on the MSI files
        performStep(insigniaMSIs,
                    params.installerPath,
                    args.verbose,
                    params.msiFileNameList)

        # Sign the MSIs, verify signing was successful
        performStepWithMessage("Signing and verifying MSIs",
                               signtoolSignAndVerifyFiles,
                               params.msiFileNameList,
                               params.installerPath,
                               params.intermediateInstallerPath,
                               params.signingType,
                               args.timestampServer,
                               args.verbose)

        # Sign the Metrics executable
        performStepWithMessage("Signing and verifying metrics.exe",
                               signtoolSignAndVerifyFile,
                               params.metricsExe,
                               params.installerPath,
                               params.intermediateInstallerPath,
                               params.signingType,
                               args.timestampServer,
                               args.verbose)

        # make sure that the bootstrapper will get the metrics exe from the right place
        params.metricsPath = params.installerPath

    # CREATE BOOTSTRAP
    packageNameList = ""
    if operationMode is OperationMode.BuildInstaller:
        for msiName in params.msiFileNameList:
            packageName = os.path.splitext(msiName)[0]
            packageNameList += '{};'.format(packageName)
        # Remove the last semi-colon from the list.
        packageNameList = packageNameList[:-1]

    # CANDLE BOOTSTRAP
    success = performStep(candleBootstrap,
                          params.bootstrapWixObjDir,
                          params.installerPath,
                          packageNameList,
                          "LumberyardBootstrapper.wxs Redistributables.wxs",
                          args.hostURL,
                          args.verbose,
                          args.lyVersion,
                          params.metricsPath,
                          params.metricsExe,
                          params.pathTo2015Thru2019Redist,
                          params.redist2015Thru2019Exe,
                          create_id('LumberyardBootstrapper', 'BOOTSTRAPPER', args.lyVersion, args.buildId))

    assert (operationMode is not OperationMode.BuildInstaller or success == 0), \
        "Failed to generate wixobj file for bootstrapper."

    # LIGHT BOOTSTRAP
    success = performStep(lightBootstrap,
                          params.bootstrapOutputPath,
                          os.path.join(params.bootstrapWixObjDir, "*.wixobj"),
                          args.verbose,
                          args.cabCachePath)

    assert (operationMode is not OperationMode.BuildInstaller or success == 0), \
        "Failed to generate executable file for bootstrapper."

    # SIGN ENGINE AND BOOTSTRAPPER
    if params.doSigning:
        # make sure the c++ redist gets copied from the temp directory to the actual
        #   installer output path with the bootstrapper.
        if operationMode == OperationMode.BuildInstaller:
            safe_shutil_file_copy(os.path.join(params.tempBootstrapOutputDir, params.redist2015Thru2019Exe),
                                  os.path.join(params.installerPath, params.redist2015Thru2019Exe))

        unsignedBootstrapPath = os.path.join(params.installerPath, params.tempBootstrapName)
        signingBootstrapPath = os.path.join(params.installerPath, params.bootstrapName)
        signingEnginePath = os.path.join(params.installerPath, "engine.exe")
        if operationMode == OperationMode.BuildInstaller:
            shutil.copy(params.bootstrapOutputPath, unsignedBootstrapPath)

        # copy bootstrapper to installerPath?
        if operationMode == OperationMode.BuildInstaller and \
           params.installerPath is not params.intermediateInstallerPath and \
           os.path.exists(signingEnginePath):
            os.remove(signingEnginePath)

        # extract the engine from the bootstrapper with Insignia
        success = performStep(insigniaDetachBurnEngine,
                              unsignedBootstrapPath,
                              signingEnginePath,
                              args.verbose)
        assert (operationMode is not OperationMode.BuildInstaller or success == 0), \
            "Failed to detach burn engine from bootstrapper."

        # sign the engine, verify signing was successful
        performStep(signtoolSignAndVerifyFile,
                    signingEnginePath,
                    params.installerPath,
                    params.intermediateInstallerPath,
                    params.signingType,
                    args.timestampServer,
                    args.verbose)

        # attach the engine back to the bootstrapper with Insignia
        success = performStep(insigniaAttachBurnEngine,
                              unsignedBootstrapPath,
                              signingEnginePath,
                              signingBootstrapPath,
                              args.verbose)
        assert (operationMode is not OperationMode.BuildInstaller or success == -1 or success == 0), \
            "Failed to reattach burn engine to bootstrapper with error {}.".format(success)

        # delete the stray engine file since it has been reattached to the installer
        if operationMode is OperationMode.BuildInstaller:
            os.remove(signingEnginePath)
            os.remove(unsignedBootstrapPath)

        # sign the bootstrapper, verify the signing was successful
        performStep(signtoolSignAndVerifyFile,
                    signingBootstrapPath,
                    params.installerPath,
                    params.intermediateInstallerPath,
                    params.signingType,
                    args.timestampServer,
                    args.verbose)

    if args.buildId is not None:
        performStep(create_version_file, args.buildId, params.installerPath)

    if not args.keep:
        performStep(cleanTempFiles, params)
    describeStep("All steps completed")


operationMode = OperationMode.StepCounting
maxSteps = 0
buildInstaller()

operationMode = OperationMode.BuildInstaller
maxSteps = stepsTaken
buildInstaller()
