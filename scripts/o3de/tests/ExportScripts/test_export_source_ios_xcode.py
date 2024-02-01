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
import o3de.export_project as exp
import shutil
import logging

# Because the export scripts are standalone, we have to manually import them to test
import o3de.utils as utils
utils.prepend_to_system_path(pathlib.Path(__file__).parent.parent.parent / 'ExportScripts')
import export_source_ios_xcode as exp_ios
from export_source_ios_xcode import export_ios_xcode_project
import export_utility as eutil

from export_test_utility import setup_local_export_config_test

from itertools import product


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


@pytest.mark.parametrize("seedlists, seedfiles, levelnames, gamefile_patterns, serverfile_patterns, project_patterns",[
    pytest.param([],[],[], [], [], []),
    pytest.param(["/test/test.seedlist"],["/test2/seed.seed"],['main'], ['game.cfg'], ['server.cfg'], ['licensing/test*.txt']),
    pytest.param(["/test/test.seedlist", "/test/test2.seedlist"],["/test2/seed.seed","/test2/seed2.seed"],['main', 'second', 'THIRD'], ['game.cfg', 'music/test*.mp3'], ['access_ctrls.xml','server.cfg'], ['licensing/test*.txt', 'dependency.graph']),
])
def test_export_standalone_multipart_args(tmpdir, seedlists, seedfiles, levelnames, gamefile_patterns, serverfile_patterns, project_patterns):

    test_project_name, test_project_path, test_engine_path = setup_local_export_config_test(tmpdir)
    
    with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder,\
         patch('o3de.export_project.get_default_asset_platform', return_value='mac') as mock_get_asset_platform,\
         patch('export_source_ios_xcode.export_ios_xcode_project') as mock_export_func:
        
        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        
        mock_logger = create_autospec(logging.Logger)
        test_export_config = exp.get_export_project_config(project_path=None)

        mock_bundle_size = 777

        mock_ctx.args = [ "--max-bundle-size", str(mock_bundle_size),
                         '-bt', '-assets', '-cfg','release', '-tcfg','profile',
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

        args = exp_ios.export_source_ios_parse_args(mock_ctx, test_export_config)

        assert 'level_names' in args and getattr(args, 'level_names')  == levelnames

        assert 'seedlist_paths' in args and getattr(args, 'seedlist_paths') == verify_seedlists
        assert 'seedfile_paths' in args and getattr(args, 'seedfile_paths') == verify_seedfiles

        exp_ios.export_source_ios_run_command(mock_ctx, args, test_export_config, mock_logger)

        mock_export_func.assert_called_once_with(ctx=mock_ctx,
                                    target_ios_project_path=args.ios_build_path,
                                    seedlist_paths=args.seedlist_paths,
                                    seedfile_paths=args.seedfile_paths,
                                    level_names=args.level_names,
                                    should_build_tools=True,
                                    should_build_all_assets=True,
                                    build_config='release',
                                    tool_config='profile',
                                    tools_build_path=args.tools_build_path,
                                    max_bundle_size=777,
                                    fail_on_asset_errors=False,
                                    logger=mock_logger)

@pytest.mark.parametrize('dum_fail_asset_err', [True, False])
@pytest.mark.parametrize('dum_build_tools', [True, False])
@pytest.mark.parametrize('dum_build_assets', [True, False])
def test_export_standalone_exhaustive(tmpdir, dum_fail_asset_err, dum_build_tools, dum_build_assets):
    test_export_standalone_single(tmpdir, dum_fail_asset_err, dum_build_tools, dum_build_assets)


@pytest.mark.parametrize("dum_fail_asset_err, dum_build_tools, dum_build_assets",[])
def test_export_standalone_single(tmpdir, dum_fail_asset_err, dum_build_tools, dum_build_assets):
    dummy_build_config = 'release'
    dummy_tool_config = 'profile'
    dummy_archive_format = 'none'

    test_project_name, test_project_path, test_engine_path = \
        setup_local_export_config_test(tmpdir, build_config=dummy_build_config, tool_config=dummy_tool_config, archive_format=dummy_archive_format,
            fail_asset_errors=dum_fail_asset_err, build_assets=dum_build_assets, build_tools=dum_build_tools)
    
    with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder,\
         patch('o3de.export_project.get_default_asset_platform', return_value='pc') as mock_get_asset_platform,\
         patch('o3de.export_project.get_asset_processor_batch_path', return_value=tmpdir/'assetproc'),\
         patch('o3de.export_project.get_asset_bundler_batch_path', return_value=tmpdir/'assetbundles'),\
         patch('o3de.export_project.process_command', return_value=0) as mock_process_command,\
         patch('export_utility.process_level_names'),\
         patch('export_utility.build_and_bundle_assets'),\
         patch('o3de.export_project.validate_project_artifact_paths'),\
         patch('o3de.export_project.preprocess_seed_path_list'),\
         patch('shutil.make_archive'),\
         patch('export_source_ios_xcode.export_ios_xcode_project') as mock_export_func:
        
        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        
        mock_logger = create_autospec(logging.Logger)
        test_export_config = exp.get_export_project_config(project_path=None)
        
        for use_asset_arg, use_build_tool_arg, use_asset_fail_arg, build_config in product(
                    [True, False],
                    [True, False],
                    [True, False],
                    ['release', 'profile']):
                
                tool_config = 'profile'

                mock_ctx.args = ['-ibp', str(tmpdir/'output')]

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

                args = exp_ios.export_source_ios_parse_args(mock_ctx, test_export_config)

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
                
                exp_ios.export_source_ios_run_command(mock_ctx, args, test_export_config, mock_logger)

                mock_export_func.assert_called_once_with(ctx=mock_ctx,
                                  target_ios_project_path=tmpdir/'output',
                                  seedlist_paths=[],
                                  seedfile_paths=[],
                                  level_names=[],
                                  should_build_all_assets=check_asset_build,
                                  should_build_tools=check_tool_build,
                                  build_config=check_build_config,
                                  tool_config=check_tool_config,
                                  tools_build_path=args.tools_build_path,
                                  max_bundle_size=2048,
                                  fail_on_asset_errors=check_fail_asset_err,
                                  logger=mock_logger)
                mock_export_func.reset_mock()

