"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Automated scripts for tests calling AssetProcessorBatch validating basic features.

"""

import os, subprocess, shutil, time

import SubprocessUtils

def KillProcess_Windows(processName):
    # This only runs on Windows
    processList = subprocess.check_output(str.format('tasklist /NH /FO CSV /FI "IMAGENAME eq {}"', processName))
    if processList is None or processList.startswith("INFO: No tasks are running which match the specified criteria."):
        # Asset processor isn't running, no need to kill it.
        return
    os.system(str.format('taskkill /F /IM {}', processName))

def KillLumberyardTools():
    if os.name == 'nt':
        KillProcess_Windows("Editor.exe")
        KillProcess_Windows("AssetProcessor.exe")
    else:
        # Other operating systems are not yet supported, so have the test fail
        assert False


def RemoveFolder(folderPath):
    # The Asset Processor may take a bit to shut down, retry a few times if it's still holding a lock on a file.
    retryCount = 5
    for retry in range(retryCount):
        if os.path.exists(folderPath) == False:
            return

        try:
            shutil.rmtree(folderPath)
        except:
            if retry < retryCount-1:
                # Wait a few seconds for whatever has the file handle open to close it.
                time.sleep(5)
                continue
            else:
                raise


def cleanUpArtifacts(engineRoot, projectName, buildInfo):
    print ("cleanUpArtifacts")

    print ("  * Shutting down Asset Processor")
    KillLumberyardTools()

    # Setting the active project to one that is in Perforce will make sure other commands work correctly. Once the
    # project created here is destroyed, lmbr_waf commands won't work.
    if os.path.exists(os.path.join(buildInfo.buildFolder,buildInfo.lmbrCommand)):
        print ("  * Setting project to Helios")
        SubprocessUtils.SubprocessWithTimeout(str.format("{} projects set-active Helios", buildInfo.lmbrCommand), buildInfo.buildFolder, 60)
    else:
        print ("  * Cannot set project to FeatureTests, lmbr executable is not available.")
    # Clearing the cache to guarantee that no data persists between tests.
    print ("  * Clearing the asset cache")
    cachePath = os.path.join(engineRoot, "Cache", projectName)
    RemoveFolder(cachePath)

    print ("  * Clearing generated data")
    projectPath = os.path.join(engineRoot, projectName)
    RemoveFolder(projectPath)

    print ("/cleanUpArtifacts")
