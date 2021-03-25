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
import os
import shutil
from BuildInstallerUtils import *


def boolToWixBool(value):
    """
    WiX uses strings "yes" and "no" for their boolean values.
    @return - "yes" if value == True, otherwise "no".
    """
    if value:
        return "yes"
    else:
        return "no"


def createPackageInfo(directoryName, rootDirectory, outputDirectory, outputPrefix=""):
    """
    Generate commonly used information about a package that is used for WiX functions.
    @param directoryName - The name of the directory that will be packaged.
    @param rootDirectory - The path to the given package.
    @param outputDirectory - The full path (including file name) to the wxs file
        generated for this package.
    @param outputPrefix - A string to append to the beginning of the name of the
        package when creating the output file. (Default = "").
    @return - A dictionary containing the package name, source information, and
        output information.
    """
    safeDirectoryName = directoryName
    if not safeDirectoryName:
        safeDirectoryName = "packageRoot"
    moduleName = strip_special_characters(safeDirectoryName)
    sourceDirectory = os.path.join(rootDirectory, directoryName)
    wxsName = '{}{}'.format(outputPrefix, moduleName)
    outputPath = os.path.join(outputDirectory, '{}.wxs'.format(wxsName))
    componentGroupRef = '{}CG'.format(replace_leading_numbers(wxsName))

    packageInfo = {
        'name': moduleName,
        'wxsName': wxsName,
        'wxsPath': outputPath,
        'sourceName': safeDirectoryName,
        'sourcePath': sourceDirectory,
        'componentGroupRefs': componentGroupRef
    }
    return packageInfo


def getVerboseCommand(verboseMode):
    if verboseMode:
        return " -v"
    else:
        return ""


def printProgress(message, stepCount, maxSteps):
    # We want the step count to line up at the end to the total steps, so add one.
    print('{}/{}: {}'.format(stepCount + 1, maxSteps, message))


def cleanTempFiles(params):
    dirs_to_delete = [
        params.wxsRoot,
        params.wixObjOutput,
        params.bootstrapWixObjDir,
        params.tempBootstrapOutputDir,
    ]

    for del_dir in dirs_to_delete:
        if os.path.exists(del_dir):
            shutil.rmtree(del_dir)
    for filepath in glob.glob(os.path.join(params.intermediateInstallerPath, "*.wixpdb")):
        os.remove(filepath)
