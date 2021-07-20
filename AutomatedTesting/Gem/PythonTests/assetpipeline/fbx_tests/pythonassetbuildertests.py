"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Import builtin libraries
import pytest
import logging
import os
import stat

# Import LyTestTools
from ly_test_tools.o3de.asset_processor import AssetProcessor
from ly_test_tools.o3de import asset_processor as asset_processor_utils
import ly_test_tools.environment.file_system as fs

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture

# Import LyShared
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
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))


@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_main
class TestsPythonAssetProcessing_APBatch(object):   

    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_ProcessAssetWithoutScriptAfterAssetWithScript_ScriptOnlyRunsOnExpectedAsset(self, workspace, ap_setup_fixture, asset_processor):
        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "TwoSceneFiles_OneWithPythonOneWithout_PythonOnlyRunsOnFirstScene")

        asset_processor_extra_params = ["--debugOutput", "--regset=\"/O3DE/SceneAPI/AssetImporter/SkipAtomOutput=true\""]

        result, _ = asset_processor.batch_process(extra_params=asset_processor_extra_params)
        assert result, "AP Batch failed"

        expected_product_list = [
            #"simple_box_no_script.dbgsg",
            "simple_box_with_script.dbgsg"
        ]

        missing_assets, _ = utils.compare_assets_with_cache(expected_product_list,
                                                asset_processor.project_test_cache_folder())
        assert not missing_assets, f'The following assets were expected to be in, but not found in cache: {str(missing_assets)}'

        expected_path = os.path.join(asset_processor.project_test_source_folder(), "simple_box_with_script_fbx.log")
        unexpected_path = os.path.join(asset_processor.project_test_source_folder(), "simple_box_no_script_fbx.log")
        assert os.path.exists(expected_path), f"Did not find expected output test asset {expected_path}"
        #assert not os.path.exists()
        #debug_graph_path = os.path.join(asset_processor.project_test_cache_folder(), "simple_box_no_script.dbgsg")
        #assert False, f"debug_graph_path : {debug_graph_path}"
