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

import os
import pytest

import LmbrBuildInfo
import SubprocessUtils
import TestCleanup
import TestSetup

@pytest.fixture(scope="session")
def EmptyProjectFixture(request):
    print ("EmptyProjectFixture")
    engineRoot = TestSetup.FindEngineRoot()
    assert engineRoot
    projectName = "LevelDepTestProj"
    buildInfo = LmbrBuildInfo.GetBuildInfo(request.config.option.buildFlavor, engineRoot)
    TestCleanup.cleanUpArtifacts(engineRoot, projectName, buildInfo)

    TestSetup.CreateLvlDepTestProject(engineRoot, projectName, buildInfo, request.config.option.thirdPartyPath)
    # cleanUpArtifacts deleted the temp dir, so create it.
    print ("Finished EmptyProjectFixture setup")
    yield engineRoot, projectName, buildInfo, int(request.config.option.dbWaitTimes)

    print ("Tearing down EmptyProjectFixture")
    TestCleanup.cleanUpArtifacts(engineRoot, projectName, buildInfo)
    print ("Finished EmptyProjectFixture tear down")


# These tests require the Helios project to be built in profile and active before they are run.
# This external requirement allows faster iteration on these tests on Jenkins and locally.
@pytest.fixture(scope="session")
def HeliosProjectFixture(request):
    print ("HeliosProjectFixture")
    engineRoot = TestSetup.FindEngineRoot()
    assert engineRoot
    projectName = "Helios"
    buildInfo = LmbrBuildInfo.GetBuildInfo(request.config.option.buildFlavor, engineRoot)

    # These tests are run on Jenkins after other tests, to minimize time spent on Jenkins jobs.
    # Verify that the correct project has been set before this test starts.

    # Temporarily disabling while https://jira.agscollab.com/browse/LY-103017 is not in Helios branch
    # Creating a task to revert this change later: https://jira.agscollab.com/browse/LY-103334

    # Run asset processor once to process all assets, so the tests themselves can run at consistent speeds.
    SubprocessUtils.SubprocessWithTimeout([buildInfo.assetProcessorBatch], engineRoot, 120)

    print ("Finished HeliosProjectFixture setup")
    yield engineRoot, projectName, buildInfo, int(request.config.option.dbWaitTimes)
    print ("Tearing down EmptyProjectFixture")
