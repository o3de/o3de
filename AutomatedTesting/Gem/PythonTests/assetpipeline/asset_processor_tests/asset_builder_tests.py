"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

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
        4. Run Asset Builder with debug on an intact prefab
        5. Check Asset Builder didn't fail to build
        6. Run Asset Builder with debug on a corrupted prefab
        7. Verify corrupted prefab produced an error
        """
        env = ap_setup_fixture
        intact_prefab_failed = False
        corrupted_prefab_failed = False

        asset_processor.create_temp_asset_root()
        # Run Asset Processor and don't close it
        result, _ = asset_processor.gui_process()
        assert result, "AP GUI failed to launch"

        # Add test assets to project folder and save the new file paths
        temp_project_source, _ = asset_processor.prepare_test_environment(env["tests_dir"], "TestAssets",
                                                                          use_current_root=True)
        INTACT_PREFAB_PATH = os.path.join(temp_project_source, "single_working_prefab", "working_prefab.prefab")
        CORRUPTED_PREFAB_PATH = os.path.join(temp_project_source, "single_corrupted_prefab", "corrupted_prefab.prefab")

        # Verify test assets were added to the project folder
        assert os.path.exists(INTACT_PREFAB_PATH), "Intact prefab was not added to the project folder"
        assert os.path.exists(CORRUPTED_PREFAB_PATH), "Corrupted prefab was not added to the project folder"

        # Run AssetBuilder with debug on intact prefab and save the output
        intact_prefab_command = [os.path.join(workspace.paths.build_directory(), "AssetBuilder"),
                                f'-debug="{INTACT_PREFAB_PATH}"']
        listening_port = asset_processor.read_listening_port()
        if listening_port:
            intact_prefab_command.append(f'-port={listening_port}')
        if workspace.project:
            intact_prefab_command.append(f'-gamename={workspace.project}')
        intact_prefab_output = utils.safe_subprocess(intact_prefab_command)

        # Verify intact prefab did not fail
        if 'Working_Prefab.prefab" failed' in intact_prefab_output.stdout:
            intact_prefab_failed = True

        assert not intact_prefab_failed, "Intact prefab failed"

        # Run AssetBuilder with debug on corrupted prefab and save the output
        corrupted_prefab_command = [
            os.path.join(workspace.paths.build_directory(), "AssetBuilder"),
            f'-debug="{CORRUPTED_PREFAB_PATH}"',
        ]
        if listening_port:
            corrupted_prefab_command.append(f'-port={listening_port}')
        if workspace.project:
            corrupted_prefab_command.append(f'--project-path={workspace.project}')
        corrupted_prefab_output = utils.safe_subprocess(corrupted_prefab_command)

        # Verify corrupted prefab produced error
        if 'JSON parse error at line 1: Invalid value.' in corrupted_prefab_output.stdout:
            corrupted_prefab_failed = True

        assert corrupted_prefab_failed, "Corrupted prefab did not produce error"
