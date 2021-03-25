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

import shutil
import subprocess

from BuildInstallerUtils import *

# Signing cmd example
# signtool.exe sign /f c:\codesigning\lumberyard.pfx /tr http://tsa.starfieldtech.com /td SHA256 /p thePasswordSeeNotesBelow %f

# Verify cmd example.
# signtool.exe verify /v /pa %f

# SignTool COMMANDLINE TEMPLATES
signtoolCommandBase = "signtool.exe {commandType} {verbose} {commandTypeParams} {filename}"
signtoolKeySignParams = "/fd SHA256 /f {certificatePath} /p {password}"
signtoolNameSignParams = '/fd SHA256 /n "{certificateName}"'
signtoolTimestampParams = "/tr {timestampServerURL} /td SHA256"
signtoolVerifyParams = "/pa /tw /hash SHA256"


class SignType:
    def __init__(self, signParamsString):
        self.signtoolParamsTemplate = signParamsString

    def getSigntoolParamsTemplate(self):
        return self.signtoolParamsTemplate

    def getSigntoolParams(self):
        raise NotImplementedError(self.getSigntoolParams.__name__)


class KeySigning(SignType):
    def __init__(self, key, password):
        SignType.__init__(self, signtoolKeySignParams)
        self.privateKey = key
        self.password = password

    def getSigntoolParams(self):
        return self.getSigntoolParamsTemplate().format(certificatePath=self.privateKey, password=self.password)


class NameSigning(SignType):
    def __init__(self, name):
        SignType.__init__(self, signtoolNameSignParams)
        self.certName = name

    def getSigntoolParams(self):
        return self.getSigntoolParamsTemplate().format(certificateName=self.certName)


def getSignToolVerboseCommand(verboseMode):
    if verboseMode:
        return " /v"
    else:
        return ""


def buildSignCommand(filename, signingType, verbose):
    verboseCmd = getSignToolVerboseCommand(verbose)

    signtoolParams = signingType.getSigntoolParams()
    signtoolCommand = signtoolCommandBase.format(commandType="sign",
        verbose=verboseCmd, commandTypeParams=signtoolParams, filename = filename)
    return signtoolCommand


def signtoolTestCredentials(signingType, timestampServer, verbose):
    testFileName = "testFile"
    if os.path.exists(testFileName):
        os.remove(testFileName)

    with open(testFileName, "wb") as testSign:
        testSign.seek(1023)
        testSign.write("0")

    # error codes for sign tool are only 0 or 1. No way to get a success without
    #   a valid .exe to sign, so the only way to test is to try to sign a file
    #   that will be considered an unrecognized format, and parse the output to
    #   see if the error was with the password.
    signtoolCommand = buildSignCommand(testFileName, signingType, verbose)
    sp = subprocess.Popen(signtoolCommand, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, error = sp.communicate()

    validPassword = False
    if error:
        # if find returns -1, it means it couldnt find password incorrect text,
        #   meaning it is a valid password.
        validPassword = error.splitlines()[0].lower().find("password is not correct") == -1

    os.remove(testFileName)
    return validPassword


def signtoolSignFile(filename, signingType, verbose):
    signtoolCommand = buildSignCommand(filename, signingType, verbose)

    verbose_print(verbose, '\n{}\n'.format(signtoolCommand))
    return os.system(signtoolCommand) == 0


def signtoolTimestamp(filename, timestampServer, verbose):
    verboseCmd = getSignToolVerboseCommand(verbose)

    signtoolParams = signtoolTimestampParams.format(timestampServerURL=timestampServer)
    signtoolCommand = signtoolCommandBase.format(commandType="timestamp",
                                                 verbose=verboseCmd,
                                                 commandTypeParams=signtoolParams,
                                                 filename=filename)

    verbose_print(verbose, '\n{}\n'.format(signtoolCommand))

    success = False
    while not success:
        success = (os.system(signtoolCommand) == 0)

    return success


def signtoolVerifySign(filename, verbose):
    verboseCmd = getSignToolVerboseCommand(verbose)

    signtoolCommand = signtoolCommandBase.format(commandType="verify",
                                                 verbose=verboseCmd,
                                                 commandTypeParams=signtoolVerifyParams,
                                                 filename=filename)

    verbose_print(verbose, '\n{}\n'.format(signtoolCommand))
    return_result = os.system(signtoolCommand) == 0
    return return_result


def signtoolSignAndVerifyFile(filename,
                              workingDir,
                              sourceDir,
                              signingType,
                              timestampServer,
                              verbose):
    shouldCopy = workingDir is not sourceDir
    filePath = os.path.join(workingDir, filename)

    # most of the time that signing fails, it is due to the signing server not responding.
    # keep retrying until the signing is successful.
    # NOTE: might want to change this to a for loop to limit number of retries per file.
    success = False
    attemptsMade = 0

    while not success:
        # SIGN THE FILE
        success = signtoolSignFile(filePath, signingType, verbose)
        assert success, 'Failed to sign file {}. Most likely the password entered is incorrect.'.format(filename)

        # TIMESTAMP THE FILE
        success = signtoolTimestamp(filePath, timestampServer, verbose)
        assert success, "Failed to contact the timestamp server."

        # VERIFY SUCCESSFUL SIGNING
        success = signtoolVerifySign(filePath, verbose)
        attemptsMade += 1

        if not success:
            verbose_print(verbose, 'Failed to sign file {}'.format(filename))
            if shouldCopy:
                # delete the original file, copy back from source, and re-sign
                os.remove(filePath)
                shutil.copy(os.path.join(sourceDir, filename), filePath)

        verbose_print(verbose, 'Attempts made to sign file {}: {}\n'.format(filename, attemptsMade))

    return success


def signtoolSignAndVerifyFiles(fileList,
                               workingDir,
                               sourceDir,
                               signingType,
                               timestampServer,
                               verbose):

    for filename in fileList:
        signtoolSignAndVerifyFile(filename,
                                  workingDir,
                                  sourceDir,
                                  signingType,
                                  timestampServer,
                                  verbose)


def signtoolSignAndVerifyType(fileExtension,
                              workingDir,
                              sourceDir,
                              signingType,
                              timestampServer,
                              verbose):
    # Sign each file in a directory that has the given file extension
    for file in os.listdir(workingDir):
        if file.endswith(fileExtension):
            signtoolSignAndVerifyFile(os.path.basename(file),
                                      workingDir,
                                      sourceDir,
                                      signingType,
                                      timestampServer,
                                      verbose)
