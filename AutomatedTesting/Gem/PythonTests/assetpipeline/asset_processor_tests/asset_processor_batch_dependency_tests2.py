"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

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
    @pytest.mark.test_case_id("C18108049")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_WindowsMacPlatforms_SchemaGem_BatchLoadsSchemaFromGem(
            self, asset_processor, workspace):
        """
        Verify that Schemas can be loaded via Gems utilizing the fonts schema

        :returns: None

        Test Steps:
        1. Run Missing Dependency Scanner against %fonts%.xml when no fonts are present
        2. Verify fonts are scanned
        3. Verify that missing dependencies are found for fonts
        4. Add fonts to game project
        5. Run Missing Dependency Scanner against %fonts%.xml when fonts are present
        6. Verify that same amount of fonts are scanned
        7. Verify that there are no missing dependencies.
        """
        schema_name = "Font.xmlschema"
        asset_processor.create_temp_asset_root()
        asset_processor.add_source_folder_assets("Engine/Fonts")
        asset_processor.add_source_folder_assets("Gems/LyShineExamples/Assets/UI/Fonts")
        asset_processor.add_scan_folder("Engine")
        asset_processor.add_scan_folder("Gems/LyShineExamples/Assets")
        gem_asset_path = "Gems/CertificateManager/Assets"
        asset_processor.add_scan_folder(gem_asset_path)
        engine_schema_path = os.path.join(workspace.paths.engine_root(), "Engine", "Schema")
        gem_schema_path = os.path.join(asset_processor.temp_asset_root(), gem_asset_path, "Schema")

        # EXPECT Assets process successfully
        _, ap_batch_output = asset_processor.batch_process(capture_output=True, extra_params="/dsp=%fonts%.xml")

        font_lines = [line for line in ap_batch_output if "Scanning for missing" in line]
        missing_lines = [line for line in ap_batch_output if "Missing dependency" in line]

        assert len(font_lines) > 0, "Failed to scan any fonts"
        assert len(missing_lines) > 0, "Some fonts did not show missing dependencies"
        asset_processor.copy_assets_to_project([schema_name], engine_schema_path, gem_schema_path)
        _, ap_batch_output = asset_processor.batch_process(capture_output=True, extra_params="/dsp=%fonts%.xml")

        font_lines_second = [line for line in ap_batch_output if "Scanning for missing" in line]
        missing_lines_second = [line for line in ap_batch_output if "Missing dependency" in line]

        assert len(font_lines_second) == len(font_lines), "Failed to scan the same set of fonts"
        assert len(missing_lines_second) == 0, "Some fonts still had missing dependencies"
