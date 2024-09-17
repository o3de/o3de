#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pytest
import pathlib
from unittest.mock import patch, create_autospec, MagicMock, Mock, PropertyMock, ANY
from o3de.export_project import ExportProjectError, O3DEScriptExportContext, LauncherType
import logging
import configparser

# Because the export scripts are standalone, we have to manually import them to test
import o3de.utils as utils
from o3de import command_utils
utils.prepend_to_system_path(pathlib.Path(__file__).parent.parent.parent / 'ExportScripts')
from export_source_built_project import export_standalone_project, export_standalone_parse_args, export_standalone_run_command
import export_utility as eutil


def test_should_fail_immediately_for_installer_mono_build_with_no_artifacts(tmp_path):
    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"
    test_tools_build_path = (test_project_path / "build" / "test") 
    test_tools_build_path.mkdir(parents=True)

    test_output_path = tmp_path / "output"

    with patch('o3de.manifest.is_sdk_engine', return_value=True) as mock_is_sdk_engine,\
        patch('o3de.export_project.has_monolithic_artifacts', return_value=False) as mock_has_mono_artifacts,\
        patch('o3de.export_project.kill_existing_processes') as mock_kill_processes,\
        pytest.raises(exp.ExportProjectError):
        
        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name

        mock_platform = 'pc'


        export_standalone_project(mock_ctx,
                                  mock_platform,
                                  test_output_path,
                                  False,
                                  "release",
                                  "profile",
                                  [],[],[])
        
        mock_is_sdk_engine.assert_called_once_with(engine_path=mock_ctx.engine_path)
        mock_has_mono_artifacts.assert_called_once_with(mock_ctx)
        mock_kill_processes.assert_not_called()


@pytest.mark.parametrize("is_engine_centric, use_sdk, should_build_tools_flag, has_monolithic, use_monolithic, expect_toolchain_build_called", [
    pytest.param(False, True, True, True, True,  False),
    pytest.param(False, True, True, True, False, False),
    pytest.param(False, True, True, False, True, False),
    pytest.param(False, True, True, False, False, False),
    pytest.param(False, True, False, False, False, False),
    pytest.param(False, True, False, True, False, False),
    pytest.param(False, True, False, False, True, False),
    pytest.param(False, True, False, True, True, False),

    pytest.param(False, False, True, True, True,  True),
    pytest.param(False, False, True, True, False, True),
    pytest.param(False, False, True, False, True, True),
    pytest.param(False, False, True, False, False, True),
    pytest.param(False, False, False, False, False, False),
    pytest.param(False, False, False, True, False, False),
    pytest.param(False, False, False, False, True, False),
    pytest.param(False, False, False, True, True, False),

    pytest.param(True, True, True, True, True,  False),
    pytest.param(True, True, True, True, False, False),
    pytest.param(True, True, True, False, True, False),
    pytest.param(True, True, True, False, False, False),
    pytest.param(True, True, False, False, False, False),
    pytest.param(True, True, False, True, False, False),
    pytest.param(True, True, False, False, True, False),
    pytest.param(True, True, False, True, True, False),

    pytest.param(True, False, True, True, True,  True),
    pytest.param(True, False, True, True, False, True),
    pytest.param(True, False, True, False, True, True),
    pytest.param(True, False, True, False, False, True),
    pytest.param(True, False, False, False, False, False),
    pytest.param(True, False, False, True, False, False),
    pytest.param(True, False, False, False, True, False),
    pytest.param(True, False, False, True, True, False),
])
def test_build_tools_combinations(tmp_path, is_engine_centric, use_sdk, should_build_tools_flag, has_monolithic, use_monolithic, expect_toolchain_build_called):
    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"

    test_o3de_base_path = test_project_path if not is_engine_centric else test_engine_path
    test_relative_base_path = pathlib.PurePath('this/is/relative')
    test_absolute_base_path  = tmp_path / "other"

    assert test_absolute_base_path.is_absolute()

    mock_logger = create_autospec(logging.Logger)
    test_output_path = tmp_path / "output"

    with patch('o3de.manifest.is_sdk_engine', return_value=use_sdk) as mock_is_sdk_engine,\
         patch('o3de.export_project.has_monolithic_artifacts', return_value=has_monolithic) as mock_has_mono_artifacts,\
         patch('o3de.export_project.get_platform_installer_folder_name', return_value="Windows") as mock_platform_folder_name,\
         patch('o3de.export_project.validate_project_artifact_paths', return_value=[]) as mock_validate_project_artifacts,\
         patch('o3de.export_project.kill_existing_processes') as mock_kill_processes,\
         patch('o3de.export_project.build_export_toolchain') as mock_build_export_toolchain,\
         patch('o3de.export_project.build_game_targets') as mock_build_game_targets,\
         patch('o3de.export_project.get_asset_processor_batch_path') as mock_get_asset_processor_path,\
         patch('o3de.export_project.build_assets') as mock_build_assets,\
         patch('o3de.export_project.get_asset_bundler_batch_path') as mock_get_asset_bundler_path,\
         patch('o3de.export_project.bundle_assets') as mock_bundle_assets,\
         patch('o3de.export_project.setup_launcher_layout_directory') as mock_setup_launcher_layout_directory,\
         patch('logging.getLogger', return_value=mock_logger) as mock_get_logger:
        
        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name

        mock_platform = 'pc'

        for base_path in [None, test_o3de_base_path, test_absolute_base_path, test_relative_base_path]:

            if base_path:
                test_tools_build_path = (base_path / "build" / "tools") 

                if test_tools_build_path.is_absolute():
                    test_tools_build_path.mkdir(parents=True)
                    
            else:
                test_tools_build_path = None

            buildconf = 'release' if use_monolithic else 'profile'
            try:
                export_standalone_project(mock_ctx,
                                        mock_platform,
                                        test_output_path,
                                        should_build_tools_flag,
                                        buildconf,
                                        "profile",
                                        [],[],[],
                                        monolithic_build=use_monolithic,
                                        tools_build_path=test_tools_build_path,
                                        engine_centric=is_engine_centric)
                    
                if expect_toolchain_build_called:
                    if not test_tools_build_path:
                        test_tools_build_path = test_o3de_base_path / 'build/tools'
                    elif not test_tools_build_path.is_absolute():
                        test_tools_build_path  = test_o3de_base_path / test_tools_build_path

                    mock_build_export_toolchain.assert_called_once_with(ctx=mock_ctx,
                                                                        tools_build_path=test_tools_build_path,
                                                                        engine_centric=is_engine_centric,
                                                                        tool_config="profile",
                                                                        logger=mock_logger)
                else:
                    mock_build_export_toolchain.assert_not_called()
                    
            except ExportProjectError:
                assert not has_monolithic, "Error expected if we do not have monolithic binaries what requesting monolithic builds."

            mock_build_export_toolchain.reset_mock()


@pytest.mark.parametrize("is_engine_centric, use_sdk, has_monolithic, use_monolithic", [
    pytest.param(False, True, True, True),
    pytest.param(False, True, True, False),
    pytest.param(False, True, False, True),
    pytest.param(False, True, False, False),
    pytest.param(False, False, True, True),
    pytest.param(False, False, True, False),
    pytest.param(False, False, False, True),
    pytest.param(False, False, False, False),

    pytest.param(True, True, True, True),
    pytest.param(True, True, True, False),
    pytest.param(True, True, False, True),
    pytest.param(True, True, False, False),
    pytest.param(True, False, True, True),
    pytest.param(True, False, True, False),
    pytest.param(True, False, False, True),
    pytest.param(True, False, False, False),
])
def test_asset_bundler_combinations(tmp_path, is_engine_centric, use_sdk, has_monolithic, use_monolithic):
    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"

    test_o3de_base_path = test_project_path if not is_engine_centric else test_engine_path
    test_relative_base_path = pathlib.PurePath('this/is/relative')
    test_absolute_base_path  = tmp_path / "other"

    assert test_absolute_base_path.is_absolute()

    test_output_path = tmp_path / "output"


    with patch('o3de.manifest.is_sdk_engine', return_value=use_sdk) as mock_is_sdk_engine,\
         patch('o3de.export_project.has_monolithic_artifacts', return_value=has_monolithic) as mock_has_mono_artifacts,\
         patch('o3de.export_project.get_platform_installer_folder_name', return_value="Windows") as mock_platform_folder_name,\
         patch('o3de.export_project.validate_project_artifact_paths', return_value=[]) as mock_validate_project_artifacts,\
         patch('o3de.export_project.kill_existing_processes') as mock_kill_processes,\
         patch('o3de.export_project.build_export_toolchain') as mock_build_export_toolchain,\
         patch('o3de.export_project.build_game_targets') as mock_build_game_targets,\
         patch('o3de.export_project.get_asset_processor_batch_path') as mock_get_asset_processor_path,\
         patch('o3de.export_project.build_assets') as mock_build_assets,\
         patch('o3de.export_project.get_asset_bundler_batch_path') as mock_get_asset_bundler_path,\
         patch('o3de.export_project.bundle_assets') as mock_bundle_assets,\
         patch('o3de.export_project.setup_launcher_layout_directory') as mock_setup_launcher_layout_directory:
        
        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name

        mock_platform = 'pc'

        for should_build_tools, buildconf in zip([True, False, True, False],
                                                ["profile", "profile", "release", "release"]):
            for base_path in [None, test_o3de_base_path, test_absolute_base_path, test_relative_base_path]:

                test_tools_build_path = None if not base_path else (base_path / "build" / "tools") 
                test_asset_bundling_path = None if not base_path else (base_path / 'build' / 'asset_bundling')


                test_tools_sdk_path = (test_engine_path / 'bin/Windows/profile/Default')
                

                if test_tools_build_path and test_tools_build_path.is_absolute():
                    test_tools_build_path.mkdir(exist_ok=True, parents=True)

                try:
                    export_standalone_project(mock_ctx,
                                        mock_platform,
                                        test_output_path,
                                        should_build_tools,
                                        buildconf,
                                        'profile',
                                        [],[],[],
                                        monolithic_build=use_monolithic,
                                        engine_centric=is_engine_centric,
                                        tools_build_path=test_tools_build_path,
                                        asset_bundling_path=test_asset_bundling_path)
                    selected_tools_build_path = test_tools_build_path if not use_sdk else test_tools_sdk_path

                    if not selected_tools_build_path:
                        selected_tools_build_path = test_o3de_base_path / 'build/tools'

                    if not selected_tools_build_path.is_absolute():
                        selected_tools_build_path = test_o3de_base_path / selected_tools_build_path

                    selected_asset_bundling_path = test_asset_bundling_path if test_asset_bundling_path else \
                                                        test_o3de_base_path / 'build/asset_bundling'

                    if not selected_asset_bundling_path.is_absolute():
                        selected_asset_bundling_path = test_o3de_base_path / selected_asset_bundling_path

                    mock_get_asset_bundler_path.assert_not_called()

                    mock_bundle_assets.assert_called_once_with(ctx=mock_ctx,
                                                            selected_platforms=[mock_platform],
                                                            seedlist_paths=[],
                                                            seedfile_paths=[],
                                                            tools_build_path=selected_tools_build_path,
                                                            engine_centric=is_engine_centric,
                                                            asset_bundling_path=selected_asset_bundling_path,
                                                            using_installer_sdk=use_sdk,
                                                            tool_config='profile',
                                                            max_bundle_size=2048)
                except ExportProjectError:
                    assert not has_monolithic, "Error expected if we do not have monolithic binaries what requesting monolithic builds."

                mock_get_asset_bundler_path.reset_mock()
                mock_bundle_assets.reset_mock()


@pytest.mark.parametrize("test_seedlists, test_seedfiles, test_levelnames",[
    pytest.param([],[],[]),
    pytest.param([pathlib.PurePath("C:\\test\\test.seedlist")],[pathlib.PurePath("C:\\test1\\test.seed")],[]),
    pytest.param([pathlib.PurePath("C:\\test\\test.seedlist")],[pathlib.PurePath("C:\\test1\\test.seed")],["main"]),
    pytest.param([pathlib.PurePath("C:\\test\\test.seedlist")],[],["main"]),
    pytest.param([pathlib.PurePath("C:\\test\\test.seedlist")],[],["MAIN"]),
    pytest.param([pathlib.PurePath("C:\\test\\test.seedlist")],[],["main",'second']),

    pytest.param([pathlib.PurePath("/test/test.seedlist")],[pathlib.PurePath("/test1/test.seed")],[]),
    pytest.param([pathlib.PurePath("/test/test.seedlist")],[pathlib.PurePath("/test1/test.seed")],["main"]),
    pytest.param([pathlib.PurePath("/test/test.seedlist")],[],["main"]),
    pytest.param([pathlib.PurePath("/test/test.seedlist")],[],["MAIN"]),
    pytest.param([pathlib.PurePath("/test/test.seedlist")],[],["main",'second'])
])
def test_asset_bundler_seed_combinations(tmp_path, test_seedlists, test_seedfiles, test_levelnames):
    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"

    test_o3de_base_path = test_project_path
    test_relative_base_path = pathlib.PurePath('this/is/relative')
    test_absolute_base_path  = tmp_path / "other"

    assert test_absolute_base_path.is_absolute()

    test_output_path = tmp_path / "output"
    test_tools_sdk_path = (test_engine_path / 'bin/Windows/profile/Default')

    with patch('o3de.manifest.is_sdk_engine', return_value=True) as mock_is_sdk_engine,\
         patch('o3de.export_project.has_monolithic_artifacts', return_value=True) as mock_has_mono_artifacts,\
         patch('o3de.export_project.get_platform_installer_folder_name', return_value="Windows") as mock_platform_folder_name,\
         patch('o3de.export_project.validate_project_artifact_paths', return_value=test_seedlists) as mock_validate_project_artifacts,\
         patch('o3de.export_project.kill_existing_processes') as mock_kill_processes,\
         patch('o3de.export_project.build_export_toolchain') as mock_build_export_toolchain,\
         patch('o3de.export_project.build_game_targets') as mock_build_game_targets,\
         patch('o3de.export_project.get_asset_processor_batch_path') as mock_get_asset_processor_path,\
         patch('o3de.export_project.build_assets') as mock_build_assets,\
         patch('o3de.export_project.get_asset_bundler_batch_path') as mock_get_asset_bundler_path,\
         patch('o3de.export_project.bundle_assets') as mock_bundle_assets,\
         patch('pathlib.Path.is_file'),\
         patch('o3de.export_project.setup_launcher_layout_directory') as mock_setup_launcher_layout_directory:
        
        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name

        mock_platform = 'pc'
        export_standalone_project(mock_ctx,
                                mock_platform,
                                test_output_path,
                                False, 
                                "release",
                                'profile',
                                test_seedlists,
                                test_seedfiles,
                                test_levelnames)
        
        combined_seedfiles = test_seedfiles 
        for ln in test_levelnames:
            combined_seedfiles.append(test_project_path / f'Cache/{mock_platform}/levels' / ln.lower() / (ln.lower() + ".spawnable"))
        
        _, kwargs = mock_bundle_assets.call_args

        
        assert kwargs['ctx'] == mock_ctx
        assert kwargs['selected_platforms'] == [mock_platform]
        assert sorted(kwargs['seedlist_paths']) == sorted(test_seedlists)
        assert sorted(kwargs['seedfile_paths']) == sorted(combined_seedfiles)
        assert kwargs['tools_build_path'] == test_tools_sdk_path
        assert kwargs['engine_centric'] == False
        assert kwargs['asset_bundling_path'] == test_o3de_base_path / 'build/asset_bundling'
        assert kwargs['using_installer_sdk'] == True
        assert kwargs['tool_config'] == 'profile'
        assert kwargs['max_bundle_size'] == 2048


@pytest.mark.parametrize("is_engine_centric, use_sdk, has_monolithic, use_monolithic", [
    pytest.param(False, True, True, True),
    pytest.param(False, True, True, False),
    pytest.param(False, True, False, True),
    pytest.param(False, True, False, False),
    pytest.param(False, False, True, True),
    pytest.param(False, False, True, False),
    pytest.param(False, False, False, True),
    pytest.param(False, False, False, False),

    pytest.param(True, True, True, True),
    pytest.param(True, True, True, False),
    pytest.param(True, True, False, True),
    pytest.param(True, True, False, False),
    pytest.param(True, False, True, True),
    pytest.param(True, False, True, False),
    pytest.param(True, False, False, True),
    pytest.param(True, False, False, False),
])
def test_asset_processor_combinations(tmp_path, is_engine_centric, use_sdk, has_monolithic, use_monolithic):
    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"

    test_o3de_base_path = test_project_path if not is_engine_centric else test_engine_path
    test_relative_base_path = pathlib.PurePath('this/is/relative')
    test_absolute_base_path  = tmp_path / "other"

    assert test_absolute_base_path.is_absolute()

    test_output_path = tmp_path / "output"

    mock_logger = create_autospec(logging.Logger)

    with patch('o3de.manifest.is_sdk_engine', return_value=use_sdk) as mock_is_sdk_engine,\
         patch('o3de.export_project.has_monolithic_artifacts', return_value=has_monolithic) as mock_has_mono_artifacts,\
         patch('o3de.export_project.get_platform_installer_folder_name', return_value="Windows") as mock_platform_folder_name,\
         patch('o3de.export_project.validate_project_artifact_paths', return_value=[]) as mock_validate_project_artifacts,\
         patch('o3de.export_project.kill_existing_processes') as mock_kill_processes,\
         patch('o3de.export_project.build_export_toolchain') as mock_build_export_toolchain,\
         patch('o3de.export_project.build_game_targets') as mock_build_game_targets,\
         patch('o3de.export_project.get_asset_processor_batch_path') as mock_get_asset_processor_path,\
         patch('o3de.export_project.build_assets') as mock_build_assets,\
         patch('o3de.export_project.get_asset_bundler_batch_path') as mock_get_asset_bundler_path,\
         patch('o3de.export_project.bundle_assets') as mock_bundle_assets,\
         patch('o3de.export_project.setup_launcher_layout_directory') as mock_setup_launcher_layout_directory,\
         patch('logging.getLogger', return_value=mock_logger) as mock_get_logger:


        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name

        mock_platform = 'pc'

        for should_build_tools, buildconf in zip([True, False, True, False],
                                                ['profile', 'profile', 'release', 'release']):
            
            for base_path in [None, test_o3de_base_path, test_absolute_base_path, test_relative_base_path]:
                test_tools_build_path = None if not base_path else base_path / 'build/tools'
                test_tools_sdk_path = (test_engine_path / 'bin/Windows/profile/Default')

                selected_tools_build_path = test_tools_build_path if not use_sdk else test_tools_sdk_path

                if not selected_tools_build_path:
                    selected_tools_build_path = test_o3de_base_path / 'build/tools'
                
                if not selected_tools_build_path.is_absolute():
                    selected_tools_build_path = test_o3de_base_path / selected_tools_build_path                

                try:
                    export_standalone_project(mock_ctx,
                                        mock_platform,
                                        test_output_path,
                                        should_build_tools,
                                        buildconf,
                                        'profile',
                                        [],[],[],
                                        monolithic_build=use_monolithic,
                                        engine_centric=is_engine_centric,
                                        tools_build_path=test_tools_build_path,
                                        should_build_all_assets=True)
                    mock_get_asset_processor_path.assert_called_once_with(tools_build_path=selected_tools_build_path,
                                                                        using_installer_sdk=use_sdk,
                                                                        tool_config='profile',
                                                                        required=True)

                    mock_build_assets.assert_called_once_with(ctx=mock_ctx,
                                                            tools_build_path=selected_tools_build_path,
                                                            engine_centric=is_engine_centric,
                                                            fail_on_ap_errors=False,
                                                            using_installer_sdk=use_sdk,
                                                            tool_config='profile',
                                                            selected_platforms=[mock_platform],
                                                            logger=mock_logger)

                    mock_get_asset_processor_path.reset_mock()
                    mock_build_assets.reset_mock()
                except ExportProjectError:
                    assert not has_monolithic, "Error expected if we do not have monolithic binaries what requesting monolithic builds."

                #now test for when we skip the asset build process
                try:
                    export_standalone_project(mock_ctx,
                                            mock_platform,
                                            test_output_path,
                                            should_build_tools, 
                                            buildconf,
                                            'profile',
                                            [],[],[],
                                            monolithic_build=use_monolithic,
                                            engine_centric=is_engine_centric,
                                            tools_build_path=test_tools_build_path,
                                            should_build_all_assets=False)
                    
                    mock_get_asset_processor_path.assert_not_called()
                    mock_build_assets.assert_not_called()
                except ExportProjectError:
                    assert not has_monolithic, "Error expected if we do not have monolithic binaries what requesting monolithic builds."

                mock_get_asset_processor_path.reset_mock()
                mock_build_assets.reset_mock()


from itertools import chain, combinations


# helper function for generating launcher combinations
def launcher_powerset_indices():
    s = list([1,2,3,4])
    return list(chain.from_iterable(combinations(s, r) for r in range(len(s) +  1))) 


@pytest.mark.parametrize("is_engine_centric, use_sdk, has_monolithic, use_monolithic", [
    pytest.param(False, True, True, True),
    pytest.param(False, True, True, False),
    pytest.param(False, True, False, True),
    pytest.param(False, True, False, False),
    pytest.param(False, False, True, True),
    pytest.param(False, False, True, False),
    pytest.param(False, False, False, True),
    pytest.param(False, False, False, False),

    pytest.param(True, True, True, True),
    pytest.param(True, True, True, False),
    pytest.param(True, True, False, True),
    pytest.param(True, True, False, False),
    pytest.param(True, False, True, True),
    pytest.param(True, False, True, False),
    pytest.param(True, False, False, True),
    pytest.param(True, False, False, False),
])
def test_build_game_targets_combinations(tmp_path, is_engine_centric, use_sdk, has_monolithic, use_monolithic):
    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"

    test_o3de_base_path = test_project_path if not is_engine_centric else test_engine_path
    test_relative_base_path = pathlib.PurePath('this/is/relative')
    test_absolute_base_path  = tmp_path / "other"

    assert test_absolute_base_path.is_absolute()

    test_output_path = tmp_path / "output"

    mock_logger = create_autospec(logging.Logger)

    with patch('o3de.manifest.is_sdk_engine', return_value=use_sdk) as mock_is_sdk_engine,\
         patch('o3de.export_project.has_monolithic_artifacts', return_value=True) as mock_has_mono_artifacts,\
         patch('o3de.export_project.get_platform_installer_folder_name', return_value="Windows") as mock_platform_folder_name,\
         patch('o3de.export_project.validate_project_artifact_paths', return_value=[]) as mock_validate_project_artifacts,\
         patch('o3de.export_project.kill_existing_processes') as mock_kill_processes,\
         patch('o3de.export_project.build_export_toolchain') as mock_build_export_toolchain,\
         patch('o3de.export_project.build_game_targets') as mock_build_game_targets,\
         patch('o3de.export_project.get_asset_processor_batch_path') as mock_get_asset_processor_path,\
         patch('o3de.export_project.build_assets') as mock_build_assets,\
         patch('o3de.export_project.get_asset_bundler_batch_path') as mock_get_asset_bundler_path,\
         patch('o3de.export_project.bundle_assets') as mock_bundle_assets,\
         patch('o3de.export_project.setup_launcher_layout_directory') as mock_setup_launcher_layout_directory,\
         patch('logging.getLogger', return_value=mock_logger) as mock_get_logger:
        
        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name

        mock_platform = 'pc'

        for should_build_tools, buildconf in zip([True, False, True, False],
                                                ['profile', 'profile', 'release', 'release']):
            
            for base_path in [None, test_o3de_base_path, test_absolute_base_path, test_relative_base_path]:
                
                for launcher_indices in launcher_powerset_indices():
                    launcher_set = set(launcher_indices)
                    check_launcher_type = 0

                    should_build_game  = 1 in launcher_set
                    should_build_server = 2 in launcher_set
                    should_build_headless_server = 3 in launcher_set
                    should_build_unified = 4 in launcher_set

                    if should_build_game:
                        check_launcher_type |= LauncherType.GAME
                    if should_build_server:
                        check_launcher_type |= LauncherType.SERVER
                    if should_build_headless_server:
                        check_launcher_type |= LauncherType.HEADLESS_SERVER
                    if should_build_unified:
                        check_launcher_type |= LauncherType.UNIFIED

                    test_tools_build_path = None if not base_path else base_path / 'build/tools'
                    test_tools_sdk_path = (test_engine_path / 'bin/Windows/profile/Default')

                    test_launcher_build_path = None if not base_path else base_path / 'build/launcher'

                    selected_tools_build_path = test_tools_build_path if not use_sdk else test_tools_sdk_path
                    selected_launcher_build_path = test_launcher_build_path

                    if not selected_tools_build_path:
                        selected_tools_build_path = test_o3de_base_path / 'build/tools'
                    
                    if not selected_tools_build_path.is_absolute():
                        selected_tools_build_path = test_o3de_base_path / selected_tools_build_path
                    
                    if not selected_launcher_build_path:
                        selected_launcher_build_path = test_o3de_base_path / 'build/launcher'

                    if not selected_launcher_build_path.is_absolute():
                        selected_launcher_build_path = test_o3de_base_path / selected_launcher_build_path

                    export_standalone_project(mock_ctx,
                                    mock_platform,
                                    test_output_path,
                                    should_build_tools,
                                    buildconf,
                                    'profile',
                                    [],[],[],
                                    monolithic_build=use_monolithic,
                                    engine_centric=is_engine_centric,
                                    tools_build_path=test_tools_build_path,
                                    should_build_all_assets=True,
                                    launcher_build_path=test_launcher_build_path,
                                    should_build_game_launcher=should_build_game,
                                    should_build_server_launcher=should_build_server,
                                    should_build_headless_server_launcher=should_build_headless_server,
                                    should_build_unified_launcher=should_build_unified)
                    if check_launcher_type == 0:
                        mock_build_game_targets.assert_not_called()
                    else:
                        mock_build_game_targets.assert_called_once_with(ctx=mock_ctx,
                                                                        build_config=buildconf,
                                                                        game_build_path=selected_launcher_build_path,
                                                                        engine_centric=is_engine_centric,
                                                                        launcher_types=check_launcher_type,
                                                                        allow_registry_overrides=False,
                                                                        tool_config='profile',
                                                                        monolithic_build=ANY,
                                                                        logger=mock_logger)
                    mock_build_game_targets.reset_mock()


@pytest.mark.parametrize("is_engine_centric, use_sdk, has_monolithic, use_monolithic", [
    pytest.param(False, True, True, True),
    pytest.param(False, True, True, False),
    pytest.param(False, True, False, True),
    pytest.param(False, True, False, False),
    pytest.param(False, False, True, True),
    pytest.param(False, False, True, False),
    pytest.param(False, False, False, True),
    pytest.param(False, False, False, False),

    pytest.param(True, True, True, True),
    pytest.param(True, True, True, False),
    pytest.param(True, True, False, True),
    pytest.param(True, True, False, False),
    pytest.param(True, False, True, True),
    pytest.param(True, False, True, False),
    pytest.param(True, False, False, True),
    pytest.param(True, False, False, False),
])
def test_setup_launcher_layout_directory(tmp_path, is_engine_centric, use_sdk, has_monolithic, use_monolithic):
    setup_dir_cache = []
    def cache_params(**kwargs):
        nonlocal setup_dir_cache
        setup_dir_cache.append(kwargs)
    
    def reset_cache():
        nonlocal setup_dir_cache
        setup_dir_cache = []

    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"

    test_o3de_base_path = test_project_path if not is_engine_centric else test_engine_path
    test_relative_base_path = pathlib.PurePath('this/is/relative')
    test_absolute_base_path  = tmp_path / "other"

    assert test_absolute_base_path.is_absolute()

    test_output_path = tmp_path / "output"

    mock_logger = create_autospec(logging.Logger)

    bundle_output_path = test_o3de_base_path/'build/asset_bundling/Bundles'

    with patch('o3de.manifest.is_sdk_engine', return_value=use_sdk) as mock_is_sdk_engine,\
         patch('o3de.export_project.has_monolithic_artifacts', return_value=has_monolithic) as mock_has_mono_artifacts,\
         patch('o3de.export_project.get_platform_installer_folder_name', return_value="Windows") as mock_platform_folder_name,\
         patch('o3de.export_project.validate_project_artifact_paths', return_value=[]) as mock_validate_project_artifacts,\
         patch('o3de.export_project.kill_existing_processes') as mock_kill_processes,\
         patch('o3de.export_project.build_export_toolchain') as mock_build_export_toolchain,\
         patch('o3de.export_project.build_game_targets') as mock_build_game_targets,\
         patch('o3de.export_project.get_asset_processor_batch_path') as mock_get_asset_processor_path,\
         patch('o3de.export_project.build_assets') as mock_build_assets,\
         patch('o3de.export_project.get_asset_bundler_batch_path') as mock_get_asset_bundler_path,\
         patch('o3de.export_project.bundle_assets', return_value=bundle_output_path) as mock_bundle_assets,\
         patch('o3de.export_project.setup_launcher_layout_directory', side_effect=cache_params) as mock_setup_launcher_layout_directory,\
         patch('logging.getLogger', return_value=mock_logger) as mock_get_logger:
        
        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name

        mock_platform = 'pc'

        launcher_stubs = ['GamePackage','ServerPackage','HeadlessServerPackage','UnifiedPackage']

        launcher_map = {"GamePackage":LauncherType.GAME,
                        "ServerPackage":LauncherType.SERVER,
                        "HeadlessServerPackage":LauncherType.HEADLESS_SERVER,
                        "UnifiedPackage":LauncherType.UNIFIED}

        for should_build_tools, buildconf in zip([True, False, True, False],
                                                ['profile', 'profile', 'release', 'release']):
            
            for base_path in [None, test_o3de_base_path, test_absolute_base_path, test_relative_base_path]:
                
                for launcher_indices in launcher_powerset_indices():
                    launcher_set = set(launcher_indices)
                    settings_count = len(launcher_set)
                    check_launcher_type = 0

                    should_build_game  = 1 in launcher_set
                    should_build_server = 2 in launcher_set
                    should_build_headless_server = 3 in launcher_set
                    should_build_unified = 4 in launcher_set

                    if should_build_game:
                        check_launcher_type |= LauncherType.GAME
                    if should_build_server:
                        check_launcher_type |= LauncherType.SERVER
                    if should_build_headless_server:
                        check_launcher_type |= LauncherType.HEADLESS_SERVER
                    if should_build_unified:
                        check_launcher_type |= LauncherType.UNIFIED

                    test_tools_build_path = None if not base_path else base_path / 'build/tools'
                    test_tools_sdk_path = (test_engine_path / 'bin/Windows/profile/Default')

                    test_launcher_build_path = None if not base_path else base_path / 'build/launcher'

                    selected_tools_build_path = test_tools_build_path if not use_sdk else test_tools_sdk_path
                    selected_launcher_build_path = test_launcher_build_path

                    if not selected_tools_build_path:
                        selected_tools_build_path = test_o3de_base_path / 'build/tools'
                    
                    if not selected_tools_build_path.is_absolute():
                        selected_tools_build_path = test_o3de_base_path / selected_tools_build_path
                    
                    if not selected_launcher_build_path:
                        selected_launcher_build_path = test_o3de_base_path / 'build/launcher'

                    if not selected_launcher_build_path.is_absolute():
                        selected_launcher_build_path = test_o3de_base_path / selected_launcher_build_path

                    try:
                        export_standalone_project(mock_ctx,
                                        mock_platform,
                                        test_output_path,
                                        should_build_tools, 
                                        buildconf,
                                        'profile',
                                        [],[],[],
                                        monolithic_build=use_monolithic,
                                        engine_centric=is_engine_centric,
                                        tools_build_path=test_tools_build_path,
                                        should_build_all_assets=True,
                                        launcher_build_path=test_launcher_build_path,
                                        should_build_game_launcher=should_build_game,
                                        should_build_server_launcher=should_build_server,
                                        should_build_headless_server_launcher=should_build_headless_server,
                                        should_build_unified_launcher=should_build_unified)
                        assert len(setup_dir_cache) == settings_count, check_launcher_type

                        assert mock_setup_launcher_layout_directory.call_count == settings_count

                        settings_remaining = settings_count

                        for argset in setup_dir_cache:
                            assert argset['project_path'] == mock_ctx.project_path
                            assert argset['project_name'] == mock_ctx.project_name
                            assert argset['asset_platform'] == mock_platform
                            assert argset['build_config'] == buildconf
                            assert argset['archive_output_format'] == 'none'
                            assert argset['logger'] == mock_logger
                            assert argset['launcher_build_path'] == selected_launcher_build_path

                            test_bundles_to_copy = argset['bundles_to_copy']

                            assert test_bundles_to_copy[0] == bundle_output_path / f'game_{mock_platform}.pak'
                            assert test_bundles_to_copy[1] == bundle_output_path / f'engine_{mock_platform}.pak'

                            test_export_layout = argset['export_layout']

                            export_folder_name = test_export_layout.output_path.name

                            for stub in launcher_stubs:
                                if export_folder_name == f'{mock_ctx.project_name}{stub}' and launcher_map[stub] & check_launcher_type > 0:
                                    settings_remaining -= 1
                                    break
                        
                        assert settings_remaining == 0
                    except ExportProjectError:
                        assert not has_monolithic, "Error expected if we do not have monolithic binaries what requesting monolithic builds."

                    reset_cache()
                    mock_setup_launcher_layout_directory.reset_mock()


from export_test_utility import create_dummy_commands_settings_file, setup_local_export_config_test
import o3de.export_project as exp
from itertools import product


def test_export_standalone_parse_args_should_require_output(tmpdir):
    # Test Data
    test_project_name, test_project_path, test_engine_path = setup_local_export_config_test(tmpdir)
    
    #this should run fine, note that all parameters are considered optional except for project-path now
    with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder,\
         patch('o3de.export_project.get_default_asset_platform', return_value='pc') as mock_get_asset_platform:
        
        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        mock_ctx.args = ['--output-path', str(tmpdir/'output')]

        test_export_config = exp.get_export_project_config(project_path=None)

        export_standalone_parse_args(mock_ctx, test_export_config)


@pytest.mark.parametrize('dum_fail_asset_err', [True, False])
@pytest.mark.parametrize('dum_build_tools', [True, False])
@pytest.mark.parametrize('dum_build_assets', [True, False])
@pytest.mark.parametrize('dum_build_game', [True, False])
@pytest.mark.parametrize('dum_build_server', [True, False])
@pytest.mark.parametrize('dum_build_headless_server', [False])
@pytest.mark.parametrize('dum_build_unified', [True, False])
@pytest.mark.parametrize('dum_engine_centric', [True, False])
@pytest.mark.parametrize('dum_monolithic', [True, False])
@pytest.mark.parametrize('dum_reg_override', [False])
def test_export_standalone_exhaustive(tmpdir, dum_fail_asset_err, dum_build_tools, dum_build_assets,
                                                     dum_build_game, dum_build_server, dum_build_headless_server, dum_build_unified,
                                                     dum_engine_centric, dum_monolithic, dum_reg_override):
    test_export_standalone_single(tmpdir, dum_fail_asset_err, dum_build_tools, dum_build_assets,
                                                     dum_build_game, dum_build_server, dum_build_headless_server, dum_build_unified,
                                                     dum_engine_centric, dum_monolithic, dum_reg_override)


@pytest.mark.parametrize("dum_fail_asset_err, dum_build_tools, dum_build_assets,dum_build_game, dum_build_server, dum_build_headless_server, dum_build_unified,dum_engine_centric, dum_monolithic, dum_reg_override",[])
def test_export_standalone_single(tmpdir, dum_fail_asset_err, dum_build_tools, dum_build_assets,
                                                     dum_build_game, dum_build_server, dum_build_headless_server, dum_build_unified,
                                                     dum_engine_centric, dum_monolithic, dum_reg_override):
    dummy_build_config = 'release'
    dummy_tool_config = 'profile'
    dummy_archive_format = 'none'

    test_project_name, test_project_path, test_engine_path = \
        setup_local_export_config_test(tmpdir, build_config=dummy_build_config, tool_config=dummy_tool_config, archive_format=dummy_archive_format,
            fail_asset_errors=dum_fail_asset_err, build_assets=dum_build_assets, build_tools=dum_build_tools,
            build_game_launcher=dum_build_game, build_server_launcher=dum_build_server, build_headless_server_launcher=dum_build_headless_server, build_unified_launcher=dum_build_unified,
            engine_centric=dum_engine_centric, monolithic=dum_monolithic, allow_reg_overrides=dum_reg_override)
    
    with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder,\
         patch('o3de.export_project.get_default_asset_platform', return_value='pc') as mock_get_asset_platform,\
         patch('export_source_built_project.export_standalone_project') as mock_export_func:
        
        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        
        mock_logger = create_autospec(logging.Logger)
        test_export_config = exp.get_export_project_config(project_path=None)

        mock_platform='pc'
        mock_archive_format='none'

        for i in range(5):
            use_asset_arg = True
            use_build_tool_arg = True
            
            use_asset_fail_arg = True
            use_mono = True
            
            use_engine_centric, use_reg_override = True, True 
            use_game_build, use_server_build, use_headless_server_build, use_unified_build = True, True, False, True
            for product_entry in product(
                        [True, False],
                        [True, False]):
                    
                    build_config = 'release'
                    tool_config = 'profile'

                    # batching arguments will help us speed up the testing loop, since we only need to individually verify booleans
                    match i:
                        case 0:
                            use_build_tool_arg = product_entry[0]
                            use_asset_arg = product_entry[1]
                        case 1:
                            use_asset_fail_arg = product_entry[0]
                            use_mono = product_entry[1]
                        case 2:
                            use_engine_centric = product_entry[0]
                            use_reg_override = product_entry[1]
                        case 3:
                            use_game_build = product_entry[0]
                            use_server_build = product_entry[1]
                        case 4:
                            use_unified_build = product_entry[0]
                            use_asset_arg = product_entry[1]
                        

                    mock_ctx.args = ['--output-path', str(tmpdir/'output')]

                    #add all necessary arguments

                    check_build_config = build_config
                    if build_config:
                        mock_ctx.args += ['-cfg', build_config]
                    if not check_build_config:
                        check_build_config = test_export_config.get_value("project.build.config")
                    
                    check_tool_config = tool_config
                    if tool_config:
                        mock_ctx.args += ['-tcfg', tool_config]
                    if not check_tool_config:
                        check_tool_config = test_export_config.get_value("tool.build.config")

                    check_archive_format = mock_archive_format
                    if mock_archive_format:
                        mock_ctx.args += ['--archive-output', mock_archive_format]
                    if not check_archive_format:
                        check_archive_format = test_export_config.get_value("archive.output.format")
                    
                    check_platform = mock_platform
                    if mock_platform:
                        mock_ctx.args += ['--platform', mock_platform]
                    if not check_platform:
                        check_platform = 'pc'
                    
                    #now check for the boolean arguments

                    check_asset_build = dum_build_assets
                    if use_asset_arg:
                        check_asset_build = not check_asset_build
                        if dum_build_assets:
                            mock_ctx.args += ['--skip-build-assets']
                        else:
                            mock_ctx.args += ['--build-assets']
                    
                    check_fail_asset_err = dum_fail_asset_err
                    if use_asset_fail_arg:
                        check_fail_asset_err = not check_fail_asset_err
                        if dum_fail_asset_err:
                            mock_ctx.args += ['-coa']
                        else:
                            mock_ctx.args += ['-foa']

                    check_tool_build  = dum_build_tools
                    if use_build_tool_arg:
                        check_tool_build = not check_tool_build
                        if dum_build_tools:
                            mock_ctx.args += ['--skip-build-tools']
                        else:
                            mock_ctx.args += ['--build-tools']
                    
                    check_game_build  = dum_build_game
                    if use_game_build:
                        check_game_build = not check_game_build
                        if dum_build_game:
                            mock_ctx.args += ['--no-game-launcher']
                        else:
                            mock_ctx.args += ['--game-launcher']
                    
                    check_server_build  = dum_build_server
                    if use_server_build:
                        check_server_build = not check_server_build
                        if dum_build_server:
                            mock_ctx.args += ['--no-server-launcher']
                        else:
                            mock_ctx.args += ['--server-launcher']

                    check_headless_server_build  = dum_build_headless_server
                    if use_headless_server_build:
                        check_headless_server_build = not check_headless_server_build
                        if dum_build_headless_server:
                            mock_ctx.args += ['--no-headless-server-launcher']
                        else:
                            mock_ctx.args += ['--headless-server-launcher']
                    
                    check_unified_build  = dum_build_unified
                    if use_unified_build:
                        check_unified_build = not check_unified_build
                        if dum_build_unified:
                            mock_ctx.args += ['--no-unified-launcher']
                        else:
                            mock_ctx.args += ['--unified-launcher']
                        
                    check_engine_centric  = dum_engine_centric
                    if use_engine_centric:
                        check_engine_centric = not check_engine_centric
                        if dum_engine_centric:
                            mock_ctx.args += ['--project-centric']
                        else:
                            mock_ctx.args += ['--engine-centric']
                        
                    check_reg_override  = dum_reg_override
                    if use_reg_override:
                        check_reg_override = not check_reg_override
                        if dum_reg_override:
                            mock_ctx.args += ['--disallow-registry-overrides']
                        else:
                            mock_ctx.args += ['--allow-registry-overrides']
                    
                    check_mono = dum_monolithic
                    if use_mono:
                        check_mono = not check_mono
                        if dum_monolithic:
                            mock_ctx.args += ['--non-monolithic']
                        else:
                            mock_ctx.args += ['--monolithic']
                    

                    args = export_standalone_parse_args(mock_ctx, test_export_config)

                    assert getattr(args, 'output_path') == tmpdir/'output'
                    assert getattr(args, 'config') == check_build_config
                    assert getattr(args, 'tool_config') == check_tool_config
                    assert getattr(args, 'archive_output') == check_archive_format
                    assert getattr(args, 'platform') == check_platform

                    if use_asset_arg:
                        if dum_build_assets:
                            assert getattr(args, 'skip_build_assets')
                        else:
                            assert getattr(args, 'build_assets')
                    
                    if use_asset_fail_arg:
                        if dum_fail_asset_err:
                            assert getattr(args, 'continue_on_asset_errors')
                        else:
                            assert getattr(args, 'fail_on_asset_errors')
                    
                    if use_build_tool_arg:
                        if dum_build_tools:
                            assert getattr(args, 'skip_build_tools')
                        else:
                            assert getattr(args, 'build_tools')
                    
                    if use_game_build:
                        if dum_build_game:
                            assert getattr(args, 'no_game_launcher')
                        else:
                            assert getattr(args, 'game_launcher')
                    
                    if use_server_build:
                        if dum_build_server:
                            assert getattr(args, 'no_server_launcher')
                        else:
                            assert getattr(args, 'server_launcher')
                    
                    if use_unified_build:
                        if dum_build_unified:
                            assert getattr(args, 'no_unified_launcher')
                        else:
                            assert getattr(args, 'unified_launcher')
                    
                    if use_engine_centric:
                        if dum_engine_centric:
                            assert getattr(args, 'project_centric')
                        else:
                            assert getattr(args, 'engine_centric')
                    
                    if use_reg_override:
                        if dum_reg_override:
                            assert getattr(args, 'disallow_registry_overrides')
                        else:
                            assert getattr(args, 'allow_registry_overrides')
                    
                    if use_mono:
                        if dum_monolithic:
                            assert getattr(args, 'non_monolithic')
                        else:
                            assert getattr(args, 'monolithic')

                    export_standalone_run_command(mock_ctx, args, test_export_config, mock_logger)

                    mock_export_func.assert_called_once_with(ctx=mock_ctx,
                                    selected_platform=check_platform,
                                    output_path=tmpdir/'output',
                                    should_build_tools=check_tool_build,
                                    build_config=check_build_config,
                                    tool_config=check_tool_config,
                                    seedlist_paths=[],
                                    seedfile_paths=[],
                                    level_names=[],
                                    game_project_file_patterns_to_copy=[],
                                    server_project_file_patterns_to_copy=[],
                                    project_file_patterns_to_copy=[],
                                    asset_bundling_path=args.asset_bundling_path,
                                    max_bundle_size=2048,
                                    should_build_all_assets=check_asset_build,
                                    fail_on_asset_errors=check_fail_asset_err,
                                    should_build_game_launcher=check_game_build,
                                    should_build_server_launcher=check_server_build,
                                    should_build_headless_server_launcher=check_headless_server_build,
                                    should_build_unified_launcher=check_unified_build,
                                    engine_centric=check_engine_centric,
                                    allow_registry_overrides=check_reg_override,
                                    tools_build_path=args.tools_build_path,
                                    launcher_build_path=args.launcher_build_path,
                                    archive_output_format=check_archive_format,
                                    monolithic_build=check_mono,
                                    kill_o3de_processes_before_running=False,
                                    logger=mock_logger)
                    mock_export_func.reset_mock()


@pytest.mark.parametrize("seedlists, seedfiles, levelnames, gamefile_patterns, serverfile_patterns, project_patterns",[
    pytest.param([],[],[], [], [], []),
    pytest.param(["C:\\test\\test.seedlist"],["C:\\test2\\seed.seed"],['main'], ['game.cfg'], ['server.cfg'], ['licensing/test*.txt']),
    pytest.param(["C:\\test\\test.seedlist", "C:\\test\\test2.seedlist"],["C:\\test2\\seed.seed","C:\\test2\\seed2.seed"],['main', 'second', 'THIRD'], ['game.cfg', 'music/test*.mp3'], ['access_ctrls.xml','server.cfg'], ['licensing/test*.txt', 'dependency.graph']),
])
def test_export_standalone_multipart_args(tmpdir, seedlists, seedfiles, levelnames, gamefile_patterns, serverfile_patterns, project_patterns):

    test_project_name, test_project_path, test_engine_path = setup_local_export_config_test(tmpdir)
    
    with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder,\
         patch('o3de.export_project.get_default_asset_platform', return_value='pc') as mock_get_asset_platform,\
         patch('export_source_built_project.export_standalone_project') as mock_export_func:
        
        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        
        mock_logger = create_autospec(logging.Logger)
        test_export_config = exp.get_export_project_config(project_path=None)

        mock_platform='mac'

        mock_bundle_size = 777

        mock_ctx.args = ['--output-path', str(tmpdir / 'output'), "--max-bundle-size", str(mock_bundle_size),
                         '-lbp', str(pathlib.PurePath('LauncherBuilds/Packages')),
                         '-tbp', str(tmpdir / 'tool_builds'),
                         '-abp', str(tmpdir / 'assets/bundling/paks'),
                         '-pl', mock_platform]
        
        for ln in levelnames:
            mock_ctx.args += ['-lvl', ln]
        
        for gp in gamefile_patterns:
            mock_ctx.args += ['-gpfp', gp]
        
        for sp in serverfile_patterns:
            mock_ctx.args += ['-spfp', sp]
        
        for pp in project_patterns:
            mock_ctx.args += ['-pfp', pp]
        
        verify_seedlists = []
        for sl in seedlists:
            mock_ctx.args += ['-sl',  sl]
            verify_seedlists += [pathlib.Path(sl)]
        
        verify_seedfiles = []
        for sf in seedfiles:
            mock_ctx.args += ['-sf', sf]
            verify_seedfiles += [pathlib.Path(sf)]

        args = export_standalone_parse_args(mock_ctx, test_export_config)

        assert 'level_names' in args and getattr(args, 'level_names')  == levelnames

        assert 'game_project_file_patterns_to_copy' in args and getattr(args, 'game_project_file_patterns_to_copy') == gamefile_patterns

        assert 'server_project_file_patterns_to_copy' in args and getattr(args, 'server_project_file_patterns_to_copy') == serverfile_patterns

        assert 'project_file_patterns_to_copy' in args and getattr(args, 'project_file_patterns_to_copy') == project_patterns
    
        assert 'seedlist_paths' in args and getattr(args, 'seedlist_paths') == verify_seedlists
        assert 'seedfile_paths' in args and getattr(args, 'seedfile_paths') == verify_seedfiles

        export_standalone_run_command(mock_ctx, args, test_export_config, mock_logger)

        mock_export_func.assert_called_once_with(ctx=mock_ctx,
                        selected_platform='mac',
                        output_path=tmpdir/'output',
                        should_build_tools=False,
                        build_config='profile',
                        tool_config='profile',
                        seedlist_paths=verify_seedlists,
                        seedfile_paths=verify_seedfiles,
                        level_names=levelnames,
                        game_project_file_patterns_to_copy=gamefile_patterns,
                        server_project_file_patterns_to_copy=serverfile_patterns,
                        project_file_patterns_to_copy=project_patterns,
                        asset_bundling_path=tmpdir / 'assets/bundling/paks',
                        max_bundle_size=mock_bundle_size,
                        should_build_all_assets=False,
                        fail_on_asset_errors=False,
                        should_build_game_launcher=True,
                        should_build_server_launcher=True,
                        should_build_headless_server_launcher=True,
                        should_build_unified_launcher=True,
                        engine_centric=False,
                        allow_registry_overrides=False,
                        tools_build_path=tmpdir / 'tool_builds',
                        launcher_build_path=pathlib.PurePath('LauncherBuilds/Packages'),
                        archive_output_format='none',
                        monolithic_build=False,
                        kill_o3de_processes_before_running=False,
                        logger=mock_logger)
