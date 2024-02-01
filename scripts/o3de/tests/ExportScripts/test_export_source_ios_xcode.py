#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pytest
import pathlib
from unittest.mock import patch, create_autospec, MagicMock, Mock, PropertyMock
from o3de.export_project import O3DEScriptExportContext
import shutil

# Because the export scripts are standalone, we have to manually import them to test
import o3de.utils as utils
utils.prepend_to_system_path(pathlib.Path(__file__).parent.parent.parent / 'ExportScripts')
import export_source_ios_xcode as exp_ios
from export_source_ios_xcode import export_ios_xcode_project
import export_utility as eutil

from export_test_utility import setup_local_export_config_test




@pytest.mark.parametrize("skip_assets_flag, should_build_tools_flag, expect_get_asset_path, expect_process_command_count", [
    pytest.param(True, True, False, 8),
    pytest.param(True, False, False, 6),
    pytest.param(False, True, True, 8),
    pytest.param(False, False, True, 6)
])
def test_asset_and_build_combinations(tmp_path, skip_assets_flag, should_build_tools_flag, expect_get_asset_path, expect_process_command_count):
    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"
    test_tools_build_path = (test_project_path / "build" / "test") 
    test_ios_build_path = (test_project_path / "build" / "game-ios") 
    test_tools_build_path.mkdir(parents=True)
    test_ios_build_path.mkdir(parents=True)
    test_asset_processor_batch_path = test_tools_build_path / "AssetProcessorBatch"
    test_asset_processor_batch_path.write_bytes(b'fake processor')

    with patch('o3de.export_project.process_command') as mock_process_command,\
         patch('export_utility.process_level_names'),\
         patch('export_utility.build_and_bundle_assets'),\
         patch('o3de.export_project.validate_project_artifact_paths'),\
         patch('o3de.export_project.preprocess_seed_path_list'),\
         patch('shutil.make_archive'),\
         patch('o3de.export_project.get_asset_processor_batch_path')  as mock_asset_processor_path:
        
        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name

        ret = export_ios_xcode_project(mock_ctx,
                                       test_ios_build_path,
                                        [],
                                        [],
                                        [],
                                        should_build_tools_flag,
                                        skip_assets_flag,
                                        'release',
                                        'profile',
                                        test_tools_build_path,
                                        2048,
                                        False,
                                        None)
        
        mock_asset_processor_path.assert_not_called()
        
        assert mock_process_command.call_count == expect_process_command_count
