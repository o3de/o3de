"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

General Asset Processor Batch Tests
"""

# Import builtin libraries
import pytest
import logging
import os

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture



# Import LyShared
from automatedtesting_shared import file_utils as file_utils
from ly_test_tools.o3de.ap_log_parser import APLogParser, APOutputParser
import ly_test_tools.o3de.pipeline_utils as utils

# Use the following logging pattern to hook all test logging together:
logger = logging.getLogger(__name__)
# Configuring the logging is done in ly_test_tools at the following location:
# ~/dev/Tools/LyTestTools/ly_test_tools/log/py_logging_util.py

# Helper: variables we will use for parameter values in the test:
targetProjects = ["AutomatedTesting"]


@pytest.fixture
def local_resources(request, workspace, ap_setup_fixture):
    # Test-level asset folder. Directory contains a subfolder for each test (i.e. C1234567)
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))


@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_periodic
class TestsAssetProcessorBatch_DependenycyTests(object):
    """
    AssetProcessorBatch Dependency tests
    """
    schemas = [
        ("C16877170", "fonts%.xml"),
    ]

    @pytest.mark.test_case_id("C16877170")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    @pytest.mark.parametrize("folder, schema", schemas)
    # fmt:off
    def test_WindowsMacPlatforms_BatchCheckSchema_ValidateErrorChecking(self, workspace, asset_processor,
                                                                        ap_setup_fixture, folder, schema):
        # fmt:on
        """
        Test Steps:
        1. Run the Missing Dependency Scanner against everything
        2. Verify that there are no missing dependencies.
        """
        env = ap_setup_fixture

        def missing_dependency_log_lines(log) -> [str]:
            """
            Used to search the output for lines that include "Missing dependency"
             After reading the log file it prints out each grabbed line to the STDOUT using the
             logger functionality and then returns the list of grabbed lines

            :returns: missing_lines (list): Lines in most recent APBatch run that contain "Missing dependency"
            """

            missing_lines = []

            for _ in log.get_lines(run=-1, contains=["Missing dependency"]):
                missing_lines.append(_)

            for entry in missing_lines:
                logger.info(entry)
            return missing_lines

        # Prepare test assets
        asset_processor.prepare_test_environment(env["tests_dir"], folder)
        assert asset_processor.batch_process(), "AP Batch Failed while setting up test assets"

        # Run the Missing Dependency Scanner
        dsp_tag = "--dsp=%" + schema
        _, output = asset_processor.batch_process(capture_output=True,
                                                  extra_params=dsp_tag)
        log = APOutputParser(output)

        # Check the log to see if there are missing dependencies
        assert missing_dependency_log_lines(log), f"No missing dependency log messages found for {dsp_tag}"

