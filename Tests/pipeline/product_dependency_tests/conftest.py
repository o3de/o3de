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

def pytest_addoption(parser):
    parser.addoption(
        "--buildFlavor", action="store", default="WindowsVS2017", help="Sets the build type (see lmbrBuildInfo.py for what's available)"
    )

    parser.addoption(
        "--dbWaitTimes", action="store", default=60, help="How long (in seconds) to wait for a condition to be met in the database before timing out"
    )

    parser.addoption(
        "--thirdPartyPath", action="store", help="Path to the 3rd party folder"
    )

def pytest_generate_tests(metafunc):
    if "buildFlavor" in metafunc.fixturenames:
        metafunc.parametrize("buildFlavor", metafunc.config.getoption("buildFlavor"), scope="session")

    if "dbWaitTimes" in metafunc.fixturenames:
         metafunc.parametrize("dbWaitTimes", metafunc.config.getoption("dbWaitTimes"), scope="session")

    if "thirdPartyPath" in metafunc.fixturenames:
         metafunc.parametrize("thirdPartyPath", metafunc.config.getoption("thirdPartyPath"), scope="session")