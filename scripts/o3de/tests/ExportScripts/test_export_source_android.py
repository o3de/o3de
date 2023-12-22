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
from o3de.export_project import O3DEScriptExportContext

# Because the export scripts are standalone, we have to manually import them to test
import o3de.utils as utils
utils.prepend_to_system_path(pathlib.Path(__file__).parent.parent.parent / 'ExportScripts')
import export_source_android as expa



#helper to generate settings file
def create_dummy_commands_settings_file(build_config='profile', tool_config='profile', archive_format='none',
                                        build_assets=False, fail_asset_errors=False, build_tools=False,
                                        tools_path='build/tools', launcher_path='build/launcher', android_path='build/game_android',
                                        build_ios='build/game_ios', allow_reg_overrides=False,
                                        asset_bundle_path='build/asset_bundling', max_size=2048,
                                        build_game_launcher=True, build_server_launcher=True, build_headless_server_launcher=True, build_unified_launcher=True,
                                        engine_centric = False, monolithic=False):
    return  f"""
[export_project]
project.build.config = {build_config}
tool.build.config = {tool_config}
archive.output.format = {archive_format}
option.build.assets = {str(build_assets)}
option.fail.on.asset.errors = {str(fail_asset_errors)}
seedlist.paths = 
seedfile.paths = 
default.level.names = 
additional.game.project.file.pattern.to.copy = 
additional.server.project.file.pattern.to.copy = 
additional.project.file.pattern.to.copy = 
option.build.tools = {str(build_tools)}
default.build.tools.path = {tools_path}
default.launcher.build.path = {launcher_path}
default.android.build.path = {android_path}
default.ios.build.path = {build_ios}
option.allow.registry.overrides = {str(allow_reg_overrides)}
asset.bundling.path = {asset_bundle_path}
max.size = {max_size}
option.build.game.launcher = {str(build_game_launcher)}
option.build.server.launcher = {str(build_server_launcher)}
option.build.headless.server.launcher = {str(build_headless_server_launcher)}
option.build.unified.launcher = {str(build_unified_launcher)}
option.engine.centric = {str(engine_centric)}
option.build.monolithic = {str(monolithic)}

[android]
platform.sdk.api = 30
ndk.version = 25*
android.gradle.plugin = 8.1.0
gradle.jvmargs = 
sdk.root = C:\\Users\\o3deuser\\AppData\\Local\\Android\\Sdk
signconfig.store.file = C:\\Users\\o3deuser\\O3DE\\Projects\\DevTestProject\\o3de-android-key.keystore
signconfig.key.alias = o3dekey
signconfig.store.password = o3depass
signconfig.key.password = o3depass


"""
import o3de.export_project as exp
from itertools import product

def setup_local_export_config_test(tmpdir, **command_settings_kwargs):
    tmpdir.ensure('.o3de', dir=True)

    test_project_name = "TestProject"
    test_project_path = tmpdir/ 'O3DE' / "project"
    test_engine_path = tmpdir/ 'O3DE' / "engine"

    dummy_project_file = test_project_path / 'project.json'
    dummy_project_file.ensure()
    with dummy_project_file.open('w') as dpf:
        dpf.write("""
{
    "project_name": "TestProject",
    "project_id": "{11111111-1111-AAAA-AA11-111111111111}"
}
""")

    dummy_commands_file = tmpdir / '.o3de' / '.command_settings'
    with dummy_commands_file.open('w') as dcf:
        dcf.write(create_dummy_commands_settings_file(**command_settings_kwargs))
    
    return test_project_name, test_project_path, test_engine_path


def test_export_android_parse_args_should_require_output(tmpdir):
    
    test_project_name, test_project_path, test_engine_path = setup_local_export_config_test(tmpdir)
    with pytest.raises(SystemExit):
        with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder,\
             patch('o3de.export_project.get_default_asset_platform', return_value='pc') as mock_get_asset_platform:

            mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

            mock_ctx = create_autospec(O3DEScriptExportContext)
            mock_ctx.project_path = test_project_path
            mock_ctx.engine_path = test_engine_path
            mock_ctx.project_name = test_project_name

            test_export_config = exp.get_export_project_config(project_path=None)

            # Since we are not specifying arguments, output path is missing. This should cause an error
            expa.export_source_android_parse_args(mock_ctx, test_export_config)
    
    #this should run fine however
    with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder,\
         patch('o3de.export_project.get_default_asset_platform', return_value='pc') as mock_get_asset_platform:
        
        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        mock_ctx.args = ['--android-output-path', str(tmpdir/'output')]

        test_export_config = exp.get_export_project_config(project_path=None)

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
         patch('export_source_android.export_source_android_project') as mock_export_func:
        
        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        
        mock_logger = create_autospec(logging.Logger)
        test_export_config = exp.get_export_project_config(project_path=None)

        mock_bundle_size = 777

        mock_ctx.args = ['--android-output-path', str(tmpdir / 'output'), "--max-bundle-size", str(mock_bundle_size),
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
                                  target_android_project_path=args.android_output_path,
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
                                  activity_name=args.activity_name,
                                  org_name=args.org_name,
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
         patch('export_source_android.export_source_android_project') as mock_export_func:
        
        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.project_name = test_project_name
        
        mock_logger = create_autospec(logging.Logger)
        test_export_config = exp.get_export_project_config(project_path=None)


        
        for use_asset_arg, use_build_tool_arg, use_asset_fail_arg, use_engine_centric, build_config in product(
                    [True, False],
                    [True, False],
                    [True, False],
                    [True, False],
                    ['release', 'profile']):
                
                tool_config = 'profile'

                mock_ctx.args = ['--android-output-path', str(tmpdir/'output')]

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

                assert getattr(args, 'android_output_path') == tmpdir/'output'
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
                                  activity_name=args.activity_name,
                                  org_name=args.org_name,
                                  logger=mock_logger)
                mock_export_func.reset_mock()