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
        # This is a regression test. The situation it's testing is, the Python script to run
        # defined in scene manifest files was persisting in a single builder. So if
        # that builder processed file a.fbx, then b.fbx, and a.fbx has a Python script to run,
        # it was also running that Python script on b.fbx.

        asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], "TwoSceneFiles_OneWithPythonOneWithout_PythonOnlyRunsOnFirstScene")

        asset_processor_extra_params = [
        # Disabling Atom assets disables most products, using the debugOutput flag ensures one product is output.
        "--debugOutput",
        # By default, if job priorities are equal, jobs run in an arbitrary order. This makes sure
        # jobs are run by sorting on the database source name, so they run in the same order each time
        # when this test is run.
        "--sortJobsByDBSourceName",
        # Disabling Atom products means this asset won't need a lot of source dependencies to be processed,
        # keeping the scope of this test down.
        "--regset=\"/O3DE/SceneAPI/AssetImporter/SkipAtomOutput=true\"",
        # The bug this regression test happened when the same builder processed FBX files with and without Python.
        # This flag ensures that only one builder is launched, so that situation can be replicated.
        "--regset=\"/Amazon/AssetProcessor/Settings/Jobs/maxJobs=1\""]

        result, _ = asset_processor.batch_process(extra_params=asset_processor_extra_params)
        assert result, "AP Batch failed"

        expected_product_list = [
            "a_simple_box_with_script.dbgsg",
            "b_simple_box_no_script.dbgsg"
        ]

        missing_assets, _ = utils.compare_assets_with_cache(expected_product_list,
                                                asset_processor.project_test_cache_folder())
        assert not missing_assets, f'The following assets were expected to be in, but not found in cache: {str(missing_assets)}'

        # The Python script loaded in the scene manifest will write a log file with the source file's name
        # to the temp folder. This is the easiest way to have the internal Python there communicate with this test.
        expected_path = os.path.join(asset_processor.project_test_source_folder(), "a_simple_box_with_script_fbx.log")
        unexpected_path = os.path.join(asset_processor.project_test_source_folder(), "b_simple_box_no_script_fbx.log")
        
        # Simple check to make sure the Python script in the scene manifest ran on the file it should have ran on.
        assert os.path.exists(expected_path), f"Did not find expected output test asset {expected_path}"
        # If this test fails here, it means the Python script from the first processed FBX file is being run
        # on the second FBX file, when it should not be.
        assert not os.path.exists(unexpected_path), f"Found unexpected output test asset {unexpected_path}"

