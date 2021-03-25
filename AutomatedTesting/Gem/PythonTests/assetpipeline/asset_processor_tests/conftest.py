"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Pytest test configuration file.

"""


def pytest_addoption(parser):
    """
    PyTest parser to collect command line argument for Asset Processor tests
    """
    parser.addoption("--ap_timeout", action="store")
    parser.addoption("--bundle_platforms", action="store")

def pytest_configure(config):

    # Register additional markers used in Asset Pipeline component tests
    config.addinivalue_line("markers",
                            "BVT: Simple quick Build Verification Tests used to quickly validate changes.")
    config.addinivalue_line("markers",
                            "BAT: Minimal set of Build Acceptance Tests used to validate P0/P1 functionality.")
    config.addinivalue_line("markers",
                            "assetpipeline: Tests that belong to the Asset Pipeline.")