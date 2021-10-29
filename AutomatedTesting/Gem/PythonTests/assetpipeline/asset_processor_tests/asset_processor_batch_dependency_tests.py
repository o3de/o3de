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
class TestsAssetProcessorBatch_DependenycyTests(object):
    """
    AssetProcessorBatch Dependency tests
    """

    @pytest.mark.test_case_id("C16877166")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    # fmt:off
    def test_WindowsMacPlatforms_RunAPBatch_NotMissingDependency(self, ap_setup_fixture, asset_processor,
                                                                 workspace):
        # fmt:on
        """
        Engine Schema
            This test case has a conditional scenario depending on the existence of surfacetypes.xml in a project.
            Some projects have this file and others do not. Run the conditional scenario depending on the existence
            of the file in the project
                libs/materialeffects/surfacetypes.xml is listed as an entry engine_dependencies.xml
                libs/materialeffects/surfacetypes.xml is not listed as a missing dependency
                    in the 'assetprocessorbatch' console output

        Test Steps:
        1. Assets are pre-processed
        2. Verify that engine_dependencies.xml exists
        3. Verify engine_dependencies.xml has surfacetypes.xml present
        4. Run Missing Dependency scanner against the engine_dependenciese.xml
        5. Verify that Surfacetypes.xml is NOT in the missing depdencies output
        6. Add the schema file which allows our xml parser to understand dependencies for our engine_dependencies file
        7. Process assets
        8. Run Missing Dependency scanner against the engine_dependenciese.xml
        9. Verify that surfacetypes.xml is in the missing dependencies out
        """

        env = ap_setup_fixture
        BATCH_LOG_PATH = env["ap_batch_log_file"]
        asset_processor.create_temp_asset_root()
        asset_processor.add_relative_source_asset(os.path.join("Assets", "Engine", "Engine_Dependencies.xml"))
        asset_processor.add_scan_folder(os.path.join("Assets", "Engine"))
        asset_processor.add_relative_source_asset(os.path.join("Assets", "Engine", "Libs", "MaterialEffects", "surfacetypes.xml"))

        # Precondition: Assets are all processed
        asset_processor.batch_process()

        DEPENDENCIES_PATH = os.path.join(asset_processor.temp_project_cache(), "engine_dependencies.xml")
        assert os.path.exists(DEPENDENCIES_PATH), "The engine_dependencies.xml does not exist."
        surfacetypes_in_dependencies = False
        surfacetypes_missing_logline = False

        # Read engine_dependencies.xml to see if surfacetypes.xml is present
        with open(DEPENDENCIES_PATH, "r") as dependencies_file:
            for line in dependencies_file.readlines():
                if "surfacetypes.xml" in line:
                    surfacetypes_in_dependencies = True
                    logger.info("Surfacetypes.xml was listed in the engine_dependencies.xml file.")
                    break

        if not surfacetypes_in_dependencies:
            logger.info("Surfacetypes.xml was not listed in the engine_dependencies.xml file.")

        _, output = asset_processor.batch_process(capture_output=True,
                                                  extra_params="--dsp=%engine_dependencies.xml")
        log = APOutputParser(output)
        for _ in log.get_lines(run=-1, contains=["surfacetypes.xml", "Missing"]):
            surfacetypes_missing_logline = True

        assert surfacetypes_missing_logline, "Surfacetypes.xml not seen in the batch log as missing."

        # Add the schema file which allows our xml parser to understand dependencies for our engine_dependencies file
        asset_processor.add_relative_source_asset(os.path.join("Assets", "Engine", "Schema", "enginedependency.xmlschema"))
        asset_processor.batch_process()

        _, output = asset_processor.batch_process(capture_output=True,
                                                  extra_params="--dsp=%engine_dependencies.xml")
        log = APOutputParser(output)
        surfacetypes_missing_logline = False
        for _ in log.get_lines(run=-1, contains=["surfacetypes.xml", "Missing"]):
            surfacetypes_missing_logline = True

        assert not surfacetypes_missing_logline, "Surfacetypes.xml not seen in the batch log as missing."

    schemas = [
        ("C16877167", ".ent"),
        ("C16877168", "Environment.xml"),
        ("C16877169", "flares%.xml"),
        ("C16877170", "fonts%.xml"),
        ("C16877171", "game.xml"),
        ("C16877172", "particles%.xml"),
    ]

    @pytest.mark.test_case_id("C16877167")
    @pytest.mark.test_case_id("C16877168")
    @pytest.mark.test_case_id("C16877169")
    @pytest.mark.test_case_id("C16877170")
    @pytest.mark.test_case_id("C16877171")
    @pytest.mark.test_case_id("C16877172")
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

