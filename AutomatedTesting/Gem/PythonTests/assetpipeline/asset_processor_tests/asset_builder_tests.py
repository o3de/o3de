"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

General Asset Processor Builder Tests
"""

# Import builtin libraries
import pytest
import logging

import os

# Import LyTestTools
import ly_test_tools.builtin.helpers as helpers

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture

# Import LyShared
import ly_test_tools.o3de.pipeline_utils as utils
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP
# Use the following logging pattern to hook all test logging together:
logger = logging.getLogger(__name__)
# Configuring the logging is done in ly_test_tools at the following location:
# ~/dev/Tools/LyTestTools/ly_test_tools/log/py_logging_util.py

# Helper: variables we will use for parameter values in the test:
win_and_mac_platforms = [ASSET_PROCESSOR_PLATFORM_MAP['windows'],
                         ASSET_PROCESSOR_PLATFORM_MAP['mac']]

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
class TestsAssetBuilder_WindowsAndMac(object):
    """
    Specific Tests for Asset Processor Builder To Only Run on Windows and Mac
    """

    @pytest.mark.test_case_id("C1569087")
    @pytest.mark.BAT
    @pytest.mark.assetpipeline
    def test_WindowsAndMacPlatforms_AssetBuilderDebug_ShowProcessStatus(
        self, asset_processor, ap_setup_fixture, workspace
    ):
        """
        Verifying -debug parameter for AssetBuilder

        Test Steps:
        1. Create temporary workspace
        2. Launch Asset Processor GUI
        3. Add test assets to workspace
        4. Run Asset Builder with debug on an intact slice
        5. Check Asset Builder didn't fail to build
        6. Run Asset Builder with debug on a corrupted slice
        7. Verify corrupted slice produced an error
        """
        env = ap_setup_fixture
        intact_slice_failed = False
        corrupted_slice_failed = False

        asset_processor.create_temp_asset_root()
        # Run Asset Processor and don't close it
        result, _ = asset_processor.gui_process()
        assert result, "AP GUI failed to launch"

        # Add test assets to project folder and save the new file paths
        temp_project_source, _ = asset_processor.prepare_test_environment(env["tests_dir"], "C1569087",
                                                                          use_current_root=True)
        INTACT_SLICE_PATH = os.path.join(temp_project_source, "C1569087_intact.slice")
        CORRUPTED_SLICE_PATH = os.path.join(temp_project_source, "C1569087_corrupted.slice")

        # Verify test assets were added to the project folder
        assert os.path.exists(INTACT_SLICE_PATH), "Intact slice was not added to the project folder"
        assert os.path.exists(CORRUPTED_SLICE_PATH), "Corrupted slice was not added to the project folder"

        # Run AssetBuilder with debug on intact slice and save the output
        intact_slice_command = [os.path.join(workspace.paths.build_directory(), "AssetBuilder"),
                                f'-debug="{INTACT_SLICE_PATH}"']
        listening_port = asset_processor.read_listening_port()
        if listening_port:
            intact_slice_command.append(f'-port={listening_port}')
        if workspace.project:
            intact_slice_command.append(f'-gamename={workspace.project}')
        intact_slice_output = utils.safe_subprocess(intact_slice_command)

        # Verify intact slice did not fail
        if 'C1569087_intact.slice" failed' in intact_slice_output.stdout:
            intact_slice_failed = True

        assert not intact_slice_failed, "Intact slice failed"

        # Run AssetBuilder with debug on corrupted slice and save the output
        corrupted_slice_command = [
            os.path.join(workspace.paths.build_directory(), "AssetBuilder"),
            f'-debug="{CORRUPTED_SLICE_PATH}"',
        ]
        if listening_port:
            corrupted_slice_command.append(f'-port={listening_port}')
        if workspace.project:
            corrupted_slice_command.append(f'-gamename={workspace.project}')
        corrupted_slice_output = utils.safe_subprocess(corrupted_slice_command)

        # Verify corrupted slice produced error
        if 'C1569087_corrupted.slice" failed' in corrupted_slice_output.stdout:
            corrupted_slice_failed = True

        assert corrupted_slice_failed, "Corrupted slice did not produce error"
