#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import logging
import pytest
import pathlib
from unittest.mock import patch, create_autospec, MagicMock, Mock, PropertyMock
from o3de.export_project import O3DEScriptExportContext, ExportProjectError
from o3de.command_utils import O3DEConfig

# Because the export scripts are standalone, we have to manually import them to test
import o3de.utils as utils
from o3de import android_support
import platform
utils.prepend_to_system_path(pathlib.Path(__file__).parent.parent.parent / 'ExportScripts')
import export_source_android as expa
import export_utility as eutil

from export_test_utility import setup_local_export_config_test
import o3de.export_project as exp
from itertools import product

def test_export_android_parse_args_should_run_with_output(tmpdir):
    
    test_project_name, test_project_path, test_engine_path = setup_local_export_config_test(tmpdir)
    #this should run fine however
    with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder,\
         patch('o3de.export_project.get_default_asset_platform', return_value='pc') as mock_get_asset_platform:
        
        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        mock_ctx.args = ['--android-build-path', str(tmpdir/'output')]

        test_export_config = exp.get_export_project_config(project_path=None)
        test_android_config = android_support.get_android_config(project_path=None)

        expa.export_source_android_parse_args(mock_ctx, test_export_config)


@pytest.mark.parametrize("seedlists, seedfiles, levelnames, gamefile_patterns, serverfile_patterns, project_patterns",[
    pytest.param([],[],[], [], [], []),
    pytest.param(["C:\\test\\test.seedlist"],["C:\\test2\\seed.seed"],['main'], ['game.cfg'], ['server.cfg'], ['licensing/test*.txt']),
    pytest.param(["C:\\test\\test.seedlist", "C:\\test\\test2.seedlist"],["C:\\test2\\seed.seed","C:\\test2\\seed2.seed"],['main', 'second', 'THIRD'], ['game.cfg', 'music/test*.mp3'], ['access_ctrls.xml','server.cfg'], ['licensing/test*.txt', 'dependency.graph']),
])
def test_export_standalone_multipart_args(tmpdir, seedlists, seedfiles, levelnames, gamefile_patterns, serverfile_patterns, project_patterns):

    test_project_name, test_project_path, test_engine_path = setup_local_export_config_test(tmpdir)
    
    with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder,\
         patch('o3de.export_project.get_default_asset_platform', return_value='pc') as mock_get_asset_platform,\
         patch('o3de.android.configure_android_options'),\
         patch('o3de.android.generate_android_project'),\
         patch('export_source_android.export_source_android_project') as mock_export_func:
        
        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        
        mock_logger = create_autospec(logging.Logger)
        test_export_config = exp.get_export_project_config(project_path=None)
        test_android_config = android_support.get_android_config(project_path=None)

        mock_bundle_size = 777

        mock_ctx.args = ['--android-build-path', str(tmpdir / 'output'), "--max-bundle-size", str(mock_bundle_size),
                         '-tbp', str(tmpdir / 'tool_builds'),]
        
        for ln in levelnames:
            mock_ctx.args += ['-lvl', ln]
        
        verify_seedlists = []
        for sl in seedlists:
            mock_ctx.args += ['-sl',  sl]
            verify_seedlists += [pathlib.Path(sl)]
        
        verify_seedfiles = []
        for sf in seedfiles:
            mock_ctx.args += ['-sf', sf]
            verify_seedfiles += [pathlib.Path(sf)]

        args = expa.export_source_android_parse_args(mock_ctx, test_export_config)

        assert 'level_names' in args and getattr(args, 'level_names')  == levelnames

        assert 'seedlist_paths' in args and getattr(args, 'seedlist_paths') == verify_seedlists
        assert 'seedfile_paths' in args and getattr(args, 'seedfile_paths') == verify_seedfiles

        expa.export_source_android_run_command(mock_ctx, args, test_export_config, mock_logger)

        mock_export_func.assert_called_once_with(ctx=mock_ctx,
                                  target_android_project_path=pathlib.Path(args.android_build_path),
                                  seedlist_paths=verify_seedlists,
                                  seedfile_paths=verify_seedfiles,
                                  level_names=levelnames,
                                  asset_pack_mode='PAK',
                                  engine_centric=False,
                                  should_build_all_assets=False,
                                  should_build_tools=False,
                                  build_config='profile',
                                  tool_config='profile',
                                  tools_build_path=tmpdir / 'tool_builds',
                                  max_bundle_size=mock_bundle_size,
                                  fail_on_asset_errors=False,
                                  deploy_to_device=args.deploy_to_android,
                                  logger=mock_logger)


@pytest.mark.parametrize('dum_fail_asset_err', [True, False])
@pytest.mark.parametrize('dum_build_tools', [True, False])
@pytest.mark.parametrize('dum_build_assets', [True, False])
@pytest.mark.parametrize('dum_engine_centric', [True, False])
def test_export_standalone_exhaustive(tmpdir, dum_fail_asset_err, dum_build_tools, dum_build_assets, dum_engine_centric):
    test_export_standalone_single(tmpdir, dum_fail_asset_err, dum_build_tools, dum_build_assets, dum_engine_centric)


@pytest.mark.parametrize("dum_fail_asset_err, dum_build_tools, dum_build_assets, dum_engine_centric",[])
def test_export_standalone_single(tmpdir, dum_fail_asset_err, dum_build_tools, dum_build_assets, dum_engine_centric):
    dummy_build_config = 'release'
    dummy_tool_config = 'profile'
    dummy_archive_format = 'none'

    test_project_name, test_project_path, test_engine_path = \
        setup_local_export_config_test(tmpdir, build_config=dummy_build_config, tool_config=dummy_tool_config, archive_format=dummy_archive_format,
            fail_asset_errors=dum_fail_asset_err, build_assets=dum_build_assets, build_tools=dum_build_tools,
            engine_centric=dum_engine_centric)
    
    with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder,\
         patch('o3de.export_project.get_default_asset_platform', return_value='pc') as mock_get_asset_platform,\
         patch('o3de.export_project.get_asset_processor_batch_path', return_value=tmpdir/'assetproc'),\
         patch('o3de.export_project.get_asset_bundler_batch_path', return_value=tmpdir/'assetbundles'),\
         patch('o3de.export_project.process_command', return_value=0) as mock_process_command,\
         patch('o3de.android.configure_android_options'),\
         patch('o3de.android.generate_android_project'),\
         patch('export_source_android.export_source_android_project') as mock_export_func:
        
        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        
        mock_logger = create_autospec(logging.Logger)
        test_export_config = exp.get_export_project_config(project_path=None)
        test_android_config = android_support.get_android_config(project_path=None)
        
        for use_asset_arg, use_build_tool_arg, use_asset_fail_arg, use_engine_centric, build_config in product(
                    [True, False],
                    [True, False],
                    [True, False],
                    [True, False],
                    ['release', 'profile']):
                
                tool_config = 'profile'

                mock_ctx.args = ['--android-build-path', str(tmpdir/'output')]

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
                
                check_engine_centric  = dum_engine_centric
                if use_engine_centric:
                    check_engine_centric = not check_engine_centric
                    if dum_engine_centric:
                        mock_ctx.args += ['--project-centric']
                    else:
                        mock_ctx.args += ['--engine-centric']

                args = expa.export_source_android_parse_args(mock_ctx, test_export_config)

                assert getattr(args, 'android_build_path') == tmpdir/'output'
                assert getattr(args, 'config') == check_build_config
                assert getattr(args, 'tool_config') == check_tool_config

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
                
                if use_engine_centric:
                    if dum_engine_centric:
                        assert getattr(args, 'project_centric')
                    else:
                        assert getattr(args, 'engine_centric')
                
                
                expa.export_source_android_run_command(mock_ctx, args, test_export_config, mock_logger)

                mock_export_func.assert_called_once_with(ctx=mock_ctx,
                                  target_android_project_path=tmpdir/'output',
                                  seedlist_paths=[],
                                  seedfile_paths=[],
                                  level_names=[],
                                  asset_pack_mode='PAK',
                                  engine_centric=check_engine_centric,
                                  should_build_all_assets=check_asset_build,
                                  should_build_tools=check_tool_build,
                                  build_config=check_build_config,
                                  tool_config=check_tool_config,
                                  tools_build_path=args.tools_build_path,
                                  max_bundle_size=2048,
                                  fail_on_asset_errors=check_fail_asset_err,
                                  deploy_to_device=args.deploy_to_android,
                                  logger=mock_logger)
                mock_export_func.reset_mock()


@pytest.mark.parametrize("use_sdk", [True,False])
@pytest.mark.parametrize("should_build_tools_flag", [True,False])
@pytest.mark.parametrize("has_monolithic", [True,False])
def test_build_tool_combinations(tmp_path, use_sdk, should_build_tools_flag, has_monolithic):
    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"

    is_engine_centric=False

    test_o3de_base_path = test_project_path if not is_engine_centric else test_engine_path
    test_relative_base_path = pathlib.PurePath('this/is/relative')
    test_absolute_base_path  = tmp_path / "other"

    assert test_absolute_base_path.is_absolute()

    mock_logger = create_autospec(logging.Logger)
    test_output_path = tmp_path / "output"

    mock_config = create_autospec(O3DEConfig)
    mock_config.set_config_value(key=android_support.SETTINGS_SDK_ROOT.key, value=str(tmp_path/'android-sdk').replace('\\', '/'), validate_value=False)

    expect_toolchain_build_called = ((not use_sdk) and should_build_tools_flag)

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
         patch('o3de.export_project.process_command', return_value=0) as mock_process_command,\
         patch('o3de.android_support.get_android_config', return_value=mock_config),\
         patch('o3de.android.configure_android_options'),\
         patch('o3de.android.generate_android_project'),\
         patch('pathlib.Path.is_dir', return_value=True),\
         patch('logging.getLogger', return_value=mock_logger) as mock_get_logger:
        
        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name

        try:
            for base_path in [None, test_o3de_base_path, test_absolute_base_path, test_relative_base_path]:

                if base_path:
                    test_tools_build_path = (base_path / "build" / 'tools')

                    if test_tools_build_path.is_absolute():
                        test_tools_build_path.mkdir(parents=True)

                else:
                    test_tools_build_path = None

                buildconf = 'release'

                expa.export_source_android_project(mock_ctx,
                                test_output_path,
                                [],
                                [],
                                [],
                                asset_pack_mode = 'PAK',
                                engine_centric = is_engine_centric,
                                should_build_tools = should_build_tools_flag,
                                should_build_all_assets = True,
                                build_config = buildconf,
                                tool_config = 'profile',
                                tools_build_path = test_tools_build_path,
                                max_bundle_size = 2048,
                                fail_on_asset_errors = False,
                                deploy_to_device = False,
                                logger=mock_logger)

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


                mock_build_export_toolchain.reset_mock()
        except ExportProjectError:
            assert not has_monolithic, "Error expected if we do not have monolithic binaries what requesting monolithic builds."


@pytest.mark.parametrize("use_sdk", [True,False])
@pytest.mark.parametrize("should_build_tools_flag", [True,False])
@pytest.mark.parametrize("has_monolithic", [True,False])
def test_asset_bundler_combinations(tmp_path, use_sdk, should_build_tools_flag, has_monolithic):
    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"

    is_engine_centric=False

    test_o3de_base_path = test_project_path if not is_engine_centric else test_engine_path
    test_relative_base_path = pathlib.PurePath('this/is/relative')
    test_absolute_base_path  = tmp_path / "other"

    assert test_absolute_base_path.is_absolute()

    mock_logger = create_autospec(logging.Logger)
    test_output_path = tmp_path / "output"

    mock_config = create_autospec(O3DEConfig)
    mock_config.set_config_value(key=android_support.SETTINGS_SDK_ROOT.key, value=str(tmp_path/'android-sdk').replace('\\', '/'), validate_value=False)   

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
         patch('o3de.export_project.process_command', return_value=0) as mock_process_command,\
         patch('o3de.android_support.get_android_config', return_value=mock_config),\
         patch('o3de.android.configure_android_options'),\
         patch('o3de.android.generate_android_project'),\
         patch('pathlib.Path.is_dir', return_value=True),\
         patch('logging.getLogger', return_value=mock_logger) as mock_get_logger:
        
        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name

        try:

            for base_path in [None, test_o3de_base_path, test_absolute_base_path, test_relative_base_path]:

                test_tools_build_path = None if not base_path else (base_path / "build" / "tools")

                test_tools_sdk_path = (test_engine_path / 'bin/Windows/profile/Default')


                if test_tools_build_path and test_tools_build_path.is_absolute():
                    test_tools_build_path.mkdir(exist_ok=True, parents=True)

                buildconf = 'release'

                expa.export_source_android_project(mock_ctx,
                                test_output_path,
                                [],
                                [],
                                [],
                                asset_pack_mode = 'PAK',
                                engine_centric = is_engine_centric,
                                should_build_tools = should_build_tools_flag,
                                should_build_all_assets = True,
                                build_config = buildconf,
                                tool_config = 'profile',
                                tools_build_path = test_tools_build_path,
                                max_bundle_size = 2048,
                                fail_on_asset_errors = False,
                                deploy_to_device = False,
                                logger=mock_logger)

                selected_tools_build_path = test_tools_build_path if not use_sdk else test_tools_sdk_path

                if not selected_tools_build_path:
                    selected_tools_build_path = test_o3de_base_path / 'build/tools'

                if not selected_tools_build_path.is_absolute():
                    selected_tools_build_path = test_o3de_base_path / selected_tools_build_path

                mock_bundle_assets.assert_called_once_with(ctx=mock_ctx,
                                                        selected_platforms=['android'],
                                                        seedlist_paths=[],
                                                        seedfile_paths=[],
                                                        tools_build_path=selected_tools_build_path,
                                                        engine_centric=is_engine_centric,
                                                        asset_bundling_path=test_project_path/'AssetBundling',
                                                        using_installer_sdk=use_sdk,
                                                        tool_config='profile',
                                                        max_bundle_size=2048)

                mock_get_asset_bundler_path.reset_mock()
                mock_bundle_assets.reset_mock()
                mock_build_export_toolchain.reset_mock()
        except ExportProjectError:
            assert not has_monolithic, "Error expected if we do not have monolithic binaries what requesting monolithic builds."


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
    
    mock_logger = create_autospec(logging.Logger)

    mock_config = create_autospec(O3DEConfig)
    mock_config.set_config_value(key=android_support.SETTINGS_SDK_ROOT.key, value=str(tmp_path/'android-sdk').replace('\\', '/'), validate_value=False)

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
         patch('o3de.export_project.process_command', return_value=0) as mock_process_command,\
         patch('o3de.android_support.get_android_config', return_value=mock_config),\
         patch('o3de.android.configure_android_options'),\
         patch('o3de.android.generate_android_project'),\
         patch('o3de.android.add_args'),\
         patch('argparse.ArgumentParser.parse_args'),\
         patch('pathlib.Path.is_file'),\
         patch('o3de.export_project.setup_launcher_layout_directory') as mock_setup_launcher_layout_directory:
        
        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name

        expa.export_source_android_project(mock_ctx,
                                test_output_path,
                                test_seedlists,
                                test_seedfiles,
                                test_levelnames,
                                'PAK',
                                False,
                                True,
                                True,
                                'profile',
                                'profile',
                                None,
                                2048,
                                False,
                                False,
                                logger=mock_logger)
        
        combined_seedfiles = test_seedfiles 
        for ln in test_levelnames:
            combined_seedfiles.append(test_project_path / f'Cache/android/levels' / ln.lower() / (ln.lower() + ".spawnable"))
        

        # Because this script uses set for the level names, we have to manually inspect the arguments
        # Otherwise lists are incorrectly labeled as mismatching
        _, kwargs = mock_bundle_assets.call_args
        
        assert kwargs['ctx'] == mock_ctx
        assert kwargs['selected_platforms'] == ['android']
        assert sorted(kwargs['seedlist_paths']) == sorted(test_seedlists)
        assert sorted(kwargs['seedfile_paths']) == sorted(combined_seedfiles)
        assert kwargs['tools_build_path'] == test_tools_sdk_path
        assert kwargs['engine_centric'] == False
        assert kwargs['asset_bundling_path'] == test_project_path / 'AssetBundling'
        assert kwargs['using_installer_sdk'] == True
        assert kwargs['tool_config'] == 'profile'
        assert kwargs['max_bundle_size'] == 2048


@pytest.mark.parametrize("use_sdk", [True,False])
@pytest.mark.parametrize("should_build_tools_flag", [True,False])
@pytest.mark.parametrize("has_monolithic", [True,False])
def test_asset_processor_combinations(tmp_path, use_sdk, should_build_tools_flag, has_monolithic):
    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"

    is_engine_centric=False

    test_o3de_base_path = test_project_path if not is_engine_centric else test_engine_path
    test_relative_base_path = pathlib.PurePath('this/is/relative')
    test_absolute_base_path  = tmp_path / "other"

    assert test_absolute_base_path.is_absolute()

    mock_logger = create_autospec(logging.Logger)
    test_output_path = tmp_path / "output"

    mock_config = create_autospec(O3DEConfig)
    mock_config.set_config_value(key=android_support.SETTINGS_SDK_ROOT.key, value=str(tmp_path/'android-sdk').replace('\\', '/'), validate_value=False)

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
         patch('o3de.export_project.process_command', return_value=0) as mock_process_command,\
         patch('o3de.android_support.get_android_config', return_value=mock_config),\
         patch('o3de.android.configure_android_options'),\
         patch('o3de.android.generate_android_project'),\
         patch('pathlib.Path.is_dir', return_value=True),\
         patch('logging.getLogger', return_value=mock_logger) as mock_get_logger:
        
        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        try:
            for base_path in [None, test_o3de_base_path, test_absolute_base_path, test_relative_base_path]:

                test_tools_build_path = None if not base_path else (base_path / "build" / "tools")

                test_tools_sdk_path = (test_engine_path / 'bin/Windows/profile/Default')

                if test_tools_build_path and test_tools_build_path.is_absolute():
                    test_tools_build_path.mkdir(exist_ok=True, parents=True)

                buildconf = 'release'


                expa.export_source_android_project(mock_ctx,
                                test_output_path,
                                [],
                                [],
                                [],
                                asset_pack_mode = 'PAK',
                                engine_centric = is_engine_centric,
                                should_build_tools = should_build_tools_flag,
                                should_build_all_assets = True,
                                build_config = buildconf,
                                tool_config = 'profile',
                                tools_build_path = test_tools_build_path,
                                max_bundle_size = 2048,
                                fail_on_asset_errors = False,
                                deploy_to_device = False,
                                logger=mock_logger)

                selected_tools_build_path = test_tools_build_path if not use_sdk else test_tools_sdk_path

                if not selected_tools_build_path:
                    selected_tools_build_path = test_o3de_base_path / 'build/tools'

                if not selected_tools_build_path.is_absolute():
                    selected_tools_build_path = test_o3de_base_path / selected_tools_build_path

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
                                                        selected_platforms=['android'],
                                                        logger=mock_logger)

                mock_get_asset_processor_path.reset_mock()
                mock_build_assets.reset_mock()

                # now test when we skip asset building

                expa.export_source_android_project(mock_ctx,
                                test_output_path,
                                [],
                                [],
                                [],
                                asset_pack_mode = 'PAK',
                                engine_centric = is_engine_centric,
                                should_build_tools = should_build_tools_flag,
                                should_build_all_assets = False,
                                build_config = buildconf,
                                tool_config = 'profile',
                                tools_build_path = test_tools_build_path,
                                max_bundle_size = 2048,
                                fail_on_asset_errors = False,
                                deploy_to_device = False,
                                logger=mock_logger)

                selected_tools_build_path = test_tools_build_path if not use_sdk else test_tools_sdk_path

                if not selected_tools_build_path:
                    selected_tools_build_path = test_o3de_base_path / 'build/tools'

                if not selected_tools_build_path.is_absolute():
                    selected_tools_build_path = test_o3de_base_path / selected_tools_build_path

                mock_get_asset_processor_path.assert_not_called()
                mock_build_assets.assert_not_called()

                mock_get_asset_processor_path.reset_mock()
                mock_build_assets.reset_mock()
        except ExportProjectError:
            assert not has_monolithic, "Error expected if we do not have monolithic binaries what requesting monolithic builds."
