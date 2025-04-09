"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

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