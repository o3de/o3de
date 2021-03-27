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
from BuildInstallerWixUtils import *

# Insignia COMMANDLINE TEMPLATES
insigniaCommandBase = "insignia.exe -nologo {verbose} {commandType} {filename} {commandTypeParams}"
insigniaDetachEngineParams = "-o {outputPath}"
insigniaAttachEngineParams = "{bootstrapName} -o {outputPath}"


def insigniaMSI(filename, verbose):
    verboseCmd = getVerboseCommand(verbose)
    insigniaCommand = insigniaCommandBase.format(verbose=verboseCmd,
        commandType="-im", filename=filename, commandTypeParams="")

    verbose_print(verbose, '\n{}\n'.format(insigniaCommand))
    return os.system(insigniaCommand)


def insigniaMSIs(directory, verbose, fileList=None):
    if fileList is None:
        for file in os.listdir(directory):
            if file.endswith(".msi"):
                success = insigniaMSI(file, verbose)
                assert (success == 0), "Failed to update {} with the signed CAB files' information.".format(os.path.basename(file))
    else:
        for file in fileList:
            filepath = os.path.join(directory, file)
            success = insigniaMSI(filepath, verbose)
            assert (success == 0), "Failed to update {} with the signed CAB files' information.".format(os.path.basename(file))


def insigniaDetachBurnEngine(bootstrapName, engineName, verbose):
    verboseCmd = getVerboseCommand(verbose)

    insigniaParams = insigniaDetachEngineParams.format(outputPath=engineName)
    insigniaCommand = insigniaCommandBase.format(verbose=verboseCmd,
                                                 commandType="-ib",
                                                 filename=bootstrapName,
                                                 commandTypeParams=insigniaParams)

    verbose_print(verbose, '\n{}\n'.format(insigniaCommand))
    return os.system(insigniaCommand)


def insigniaAttachBurnEngine(bootstrapName, engineName, outputName, verbose):
    verboseCmd = getVerboseCommand(verbose)

    insigniaParams = insigniaAttachEngineParams.format(outputPath=outputName,
                                                       bootstrapName=bootstrapName)
    insigniaCommand = insigniaCommandBase.format(verbose=verboseCmd,
                                                 commandType="-ab",
                                                 filename=engineName,
                                                 commandTypeParams=insigniaParams)

    verbose_print(verbose, '\n{}\n'.format(insigniaCommand))
    return os.system(insigniaCommand)
