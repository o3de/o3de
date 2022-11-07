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
class TestsAssetProcessorBatch_DependencyTests(object):
    """
    AssetProcessorBatch Dependency tests
    """
    @pytest.mark.test_case_id("C18108049")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_SchemaGem_BatchLoadsSchema(
            self, asset_processor, workspace, ap_setup_fixture):
        """

        Test Steps:
        1. Run Missing Dependency Scanner against a font XML file when product dependencies are missing
        2. Verify a file was scanned
        3. Verify that missing dependencies are found for for that file
        4. Add font xmlschema to game project, that defines where to search for missing dependencies
        5. Run Missing Dependency Scanner against the file again
        6. Verify that same amount of fonts are scanned
        7. Verify that there are no longer missing dependencies
        """
        import shutil
        
        file_scan_pattern = "--dsp=%font_with_dependency.xml"

        # Verifies the XML schema can be used to address a missing dependency
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "test_xml_schema")
        _, ap_batch_output = asset_processor.batch_process(capture_output=True, extra_params=file_scan_pattern)
        
        font_lines = [line for line in ap_batch_output if "Scanning for missing" in line]
        missing_lines = [line for line in ap_batch_output if "Missing dependency" in line]
        
        # If the test is going to fail, print the log output from AP to assist in debugging.
        # Skip printing otherwise, to keep the tests from slowing down due to excessive logging.
        if len(font_lines) == 0 or len(missing_lines) == 0:
            logger.info(f"Asset Processor Output:\n")
            for log_line in ap_batch_output:
                logger.info(log_line)

        assert len(font_lines) > 0, "Failed to scan any fonts"
        assert len(missing_lines) > 0, "Some fonts did not show missing dependencies"

        # Copy the schema out of the engine, so if the font schema changes, this test will use the most current schema.
        # Don't use the engine folder directly, to keep test run time fast.
        engine_schema_path = os.path.join(workspace.paths.engine_root(), "Assets", "Engine", "Schema", "Font.xmlschema")
        
        # The project_test_source_folder is the source folder for the test assets, and not the test project,
        # so use a directory change marker to go up one folder.
        # This is because the XML schema files have to be in a very specific relative path location to be used.
        target_schema_folder = os.path.join(asset_processor.project_test_source_folder(), "..", "Schema")
        os.makedirs(target_schema_folder)
        shutil.copy(engine_schema_path, os.path.join(target_schema_folder, "Font.xmlschema"))

        _, ap_batch_output_second = asset_processor.batch_process(capture_output=True, extra_params=file_scan_pattern)
        
        font_lines_second = [line for line in ap_batch_output_second if "Scanning for missing" in line]
        missing_lines_second = [line for line in ap_batch_output_second if "Missing dependency" in line]

        if len(font_lines_second) != len(font_lines) or len(missing_lines_second) != 0:
            # If the test is going to fail, print the before and after logs for asset processor, having both assists in debugging.
            logger.info(f"Asset Processor Output:\n")
            for log_line in ap_batch_output:
                logger.info(log_line)
            logger.info(f"Second Run Asset Processor Output:\n")
            for log_line in ap_batch_output_second:
                logger.info(log_line)

        assert len(font_lines_second) == len(font_lines), "Failed to scan the same set of fonts"
        assert len(missing_lines_second) == 0, "Some fonts still had missing dependencies"
