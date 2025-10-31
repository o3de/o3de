#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import json
import pytest
import pathlib
import unittest.mock as mock
from unittest.mock import patch, create_autospec
from o3de.export_project import _export_script, process_command, setup_launcher_layout_directory, \
                                 O3DEScriptExportContext, ExportLayoutConfig, build_assets, ExportProjectError, \
                                 bundle_assets, build_export_toolchain, build_game_targets, \
                                 validate_project_artifact_paths, LauncherType, extract_cmake_custom_args, \
                                 get_default_asset_platform, get_platform_installer_folder_name, \
                                 preprocess_seed_path_list

TEST_PROJECT_JSON_PAYLOAD = '''
{
    "project_name": "TestProject",
    "project_id": "{24114e69-306d-4de6-b3b4-4cb1a3eca58e}",
    "version" : "0.0.0",
    "compatible_engines" : [
        "o3de-sdk==2205.01"
    ],
    "engine_api_dependencies" : [
        "framework==1.2.3"
    ],
    "origin": "The primary repo for TestProject goes here: i.e. http://www.mydomain.com",
    "license": "What license TestProject uses goes here: i.e. https://opensource.org/licenses/MIT",
    "display_name": "TestProject",
    "summary": "A short description of TestProject.",
    "canonical_tags": [
        "Project"
    ],
    "user_tags": [
        "TestProject"
    ],
    "icon_path": "preview.png",
    "engine": "o3de-install",
    "restricted_name": "projects",
    "external_subdirectories": [
        "D:/TestGem"
    ]
}
'''

# Note: the underlying command logic is found in CLICommand class object. That is tested in test_utils.py
@pytest.mark.parametrize("args, expected_result",[
    pytest.param(["cmake", "--version"], 0),
    pytest.param(["cmake"], 0),
    pytest.param(["cmake", "-B"], 1),
    pytest.param([], 1),
])
def test_process_command(args, expected_result):

    cli_command = mock.Mock()
    cli_command.run.return_value = expected_result

    with patch("o3de.utils.CLICommand", return_value=cli_command) as cli:
        result = process_command(args)
        assert result == expected_result



# The following functions will do integration tests of _export_script and execute_python_script, thereby testing all of script execution
TEST_PYTHON_SCRIPT = """
import pathlib
folder = pathlib.Path(__file__).parent
with open(folder / "test_output.txt", 'w') as test_file:
    test_file.write(f"This is a test for the following: {o3de_context.args[0]}")
    """

def check_for_o3de_context_arg(output_file_text, args):
    if len(args) > 0:
        assert output_file_text == f"This is a test for the following: {args[0]}"

TEST_ERR_PYTHON_SCRIPT = """
import pathlib
raise RuntimeError("Test export RuntimeError")
print("hi there")
    """

@pytest.mark.parametrize("input_script, args, should_pass_project_folder, project_folder_subpath, script_folder_subpath, output_filename, is_expecting_error, output_evaluation_func, expected_result", [
    # TEST_PYTHON_SCRIPT
    # successful cases
    pytest.param(TEST_PYTHON_SCRIPT, ['456'], False, pathlib.PurePath("test_project"), pathlib.PurePath("test_project/ExportScripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 0),
    pytest.param(TEST_PYTHON_SCRIPT, ['456'], True, pathlib.PurePath("test_project"), pathlib.PurePath("test_project/ExportScripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 0),
    pytest.param(TEST_PYTHON_SCRIPT, ['456'], True, pathlib.PurePath("test_project"), pathlib.PurePath("export_scripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 0),
    pytest.param(TEST_PYTHON_SCRIPT, [456], True, pathlib.PurePath("test_project"), pathlib.PurePath("export_scripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 0),
    # failure cases
    pytest.param(TEST_PYTHON_SCRIPT, [], True, pathlib.PurePath("test_project"), pathlib.PurePath("export_scripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 1),
    pytest.param(TEST_PYTHON_SCRIPT, [456], False, pathlib.PurePath("test_project"), pathlib.PurePath("export_scripts"), pathlib.PurePath("test_output.txt"), False, check_for_o3de_context_arg, 1),
    # TEST_ERR_PYTHON_SCRIPT
    pytest.param(TEST_ERR_PYTHON_SCRIPT, [], True, pathlib.PurePath("test_project"), pathlib.PurePath("export_scripts"), None, True, None, 1)
])
def test_export_script(tmp_path,
                       input_script, 
                       args, 
                       should_pass_project_folder, 
                       project_folder_subpath,
                       script_folder_subpath, 
                       output_filename, 
                       is_expecting_error, 
                       output_evaluation_func,
                       expected_result):
    import sys

    engine_folder = tmp_path / "test_engine"
    engine_folder.mkdir()

    with patch('o3de.manifest.get_project_engine_path', return_value=pathlib.Path) as mock_get_project_engine_path:
        mock_get_project_engine_path.return_value = engine_folder

        project_folder = tmp_path / project_folder_subpath
        project_folder.mkdir()

        script_folder = tmp_path / script_folder_subpath
        script_folder.mkdir()

        project_json = project_folder / "project.json"
        project_json.write_text(TEST_PROJECT_JSON_PAYLOAD)

        test_script = script_folder / "test.py"
        test_script.write_text(input_script)

        if output_filename:
            test_output = script_folder / output_filename

            assert not test_output.is_file()

        result = _export_script(test_script, project_folder if should_pass_project_folder else None, args)

        assert result == expected_result

    # only check for these if we're simulating a successful case
    if result == 0 and not is_expecting_error:
        assert str(script_folder) in sys.path

        if output_filename:
            assert test_output.is_file()

            with test_output.open('r') as t_out:
                output_text = t_out.read()
        
            if output_evaluation_func:
                output_evaluation_func(output_text, args)

        o3de_cli_folder = pathlib.Path(__file__).parent.parent / "o3de"

        assert o3de_cli_folder in [pathlib.Path(sysPath) for sysPath in sys.path]


@pytest.mark.parametrize("engine_centric, fail_on_ap_errors", [
    pytest.param(False, False),
    pytest.param(False, True)
])
def test_build_assets(tmp_path, engine_centric, fail_on_ap_errors):

    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"
    test_tools_build_path = (test_project_path / "build" / "test") if engine_centric else (test_engine_path / "build" / "test")
    test_tools_build_path.mkdir(parents=True)
    test_asset_processor_batch_path = test_tools_build_path / "AssetProcessorBatch"
    test_asset_processor_batch_path.write_bytes(b'fake processor')

    # Run through a mock success and mock failure run of AssetProcessorBatch
    for test_process_return_values in [0, 1]:

        with patch("o3de.export_project.process_command", return_value=test_process_return_values) as mock_process_command, \
             patch("o3de.export_project.get_asset_processor_batch_path", return_value=test_asset_processor_batch_path):

            mock_ctx = create_autospec(O3DEScriptExportContext)
            mock_ctx.project_path = test_project_path
            mock_ctx.engine_path = test_engine_path
            mock_ctx.project_name = test_project_name

            try:
                build_assets(ctx=mock_ctx,
                             tools_build_path=test_tools_build_path,
                             engine_centric=engine_centric,
                             fail_on_ap_errors=fail_on_ap_errors)
            except ExportProjectError:
                ap_error_raised = True
            else:
                ap_error_raised = False

            mock_build_assets_process_input = mock_process_command.call_args_list[0][0][0]

            # Expected process command to asset processor: <AssetProcessorBatchPath> --project-path <Project Path>
            expected_build_assets_args = [
                test_asset_processor_batch_path,
                "--project-path",
                test_project_path
            ]
            # Validate the expected arguments
            assert mock_build_assets_process_input == expected_build_assets_args

            # Validate the 'fail_on_ap_errors' option if applicable
            if test_process_return_values == 1:
                assert ap_error_raised == fail_on_ap_errors
            else:
                assert not ap_error_raised


@pytest.mark.parametrize("engine_centric, additional_cmake_configure_options, additional_build_args", [
    pytest.param(False, [], []),
    pytest.param(True, [], []),
    pytest.param(False, ['-DFOO=BAR', '--preset FOO'], ['-j', '24']),
])
def test_build_export_toolchain(tmp_path, engine_centric, additional_cmake_configure_options, additional_build_args):

    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_engine_path = tmp_path / "engine"
    test_tools_build_path = (test_project_path / "build" / "test") if engine_centric else (test_engine_path / "build" / "test")
    test_additional_args = additional_build_args
    test_generator = "Test Generator"
    test_generator_options = ["-DTEST_OPTION1", "-DTEST_OPTION2"]
    test_build_configuration = "profile"

    with patch("o3de.export_project.process_command", return_value=0) as mock_process_command, \
         patch("o3de.export_project.GENERATOR", test_generator), \
         patch("o3de.export_project.CMAKE_GENERATOR_OPTIONS", test_generator_options), \
         patch("o3de.export_project.PREREQUISITE_TOOL_BUILD_CONFIG", test_build_configuration):

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.cmake_additional_configure_args = additional_cmake_configure_options
        mock_ctx.cmake_additional_build_args = test_additional_args
        mock_ctx.is_engine_centric = engine_centric
        mock_ctx.project_name = test_project_name

        build_export_toolchain(ctx=mock_ctx,
                               tools_build_path=test_tools_build_path,
                               engine_centric=engine_centric)

        # Validate the cmake project generation calls
        mock_generate_process_input = mock_process_command.call_args_list[0][0][0]

        expected_project_path = test_engine_path if engine_centric else test_project_path

        expected_generate_args = [
            'cmake',
            '-B', test_tools_build_path,
            '-S', expected_project_path,
            '-G', test_generator
        ]
        expected_generate_args.extend(test_generator_options)
        if engine_centric:
            expected_generate_args.extend([
                f'-DLY_PROJECTS={test_project_path}'
            ])
        if additional_cmake_configure_options:
            expected_generate_args.extend(additional_cmake_configure_options)
        assert mock_generate_process_input == expected_generate_args

        # Validate the cmake project build calls
        mock_build_process_input = mock_process_command.call_args_list[1][0][0]
        expected_build_args = [
            'cmake',
            '--build', test_tools_build_path,
            '--config', test_build_configuration,
            '--target',
            'AssetProcessorBatch',
            'AssetBundlerBatch',
        ]
        if test_additional_args:
            expected_build_args.extend(test_additional_args)

        assert mock_build_process_input == expected_build_args

@pytest.mark.parametrize("focal_platform, focal_asset_platform, mock_platform",[
    pytest.param("Windows", 'pc', 'windows'),
    pytest.param("Linux", 'linux', 'linux'),
    pytest.param("Mac", 'mac', 'darwin')
])
def test_platform_helpers_with_no_params(focal_platform, focal_asset_platform, mock_platform):
    with patch('platform.system', return_value=mock_platform):
        assert get_default_asset_platform() == focal_asset_platform
        assert get_platform_installer_folder_name() == focal_platform


@pytest.mark.parametrize("build_config, build_game_launcher, build_server_launcher, build_unified_launcher, allow_registry_overrides, engine_centric, additional_build_args", [
    pytest.param("profile", True, True, True, True, False, []),
    pytest.param("release", True, True, True, True, False, []),
    pytest.param("release", True, False, False, True, False, []),
    pytest.param("release", True, True, False, True, False, []),
    pytest.param("release", False, False, False, True, False, []),
    pytest.param("release", False, True, False, True, False, []),
    pytest.param("profile", True, True, True, True, True, []),
    pytest.param("profile", True, True, True, True, True, ['-j', '16'])
])
def test_build_game_targets(tmp_path, build_config, build_game_launcher, build_server_launcher, build_unified_launcher, allow_registry_overrides, engine_centric, additional_build_args):

    test_project_name = "TestProject"
    test_project_path = tmp_path / "project"
    test_game_build_path = test_project_path / "build" / "test"
    test_engine_path = tmp_path / "engine"
    test_additional_args = additional_build_args
    test_generator = "Test Generator"
    test_generator_options = ["-DTEST_OPTION1", "-DTEST_OPTION2"]

    with patch("o3de.export_project.process_command", return_value=0) as mock_process_command, \
         patch("o3de.export_project.GENERATOR", test_generator), \
         patch("o3de.export_project.CMAKE_GENERATOR_OPTIONS", test_generator_options):

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path
        mock_ctx.engine_path = test_engine_path
        mock_ctx.cmake_additional_build_args = test_additional_args
        mock_ctx.is_engine_centric = engine_centric
        mock_ctx.project_name = test_project_name

        launcher_types = 0
        if build_game_launcher:
            launcher_types |= LauncherType.GAME
        if build_server_launcher:
            launcher_types |= LauncherType.SERVER
        if build_unified_launcher:
            launcher_types |= LauncherType.UNIFIED

        build_game_targets(ctx=mock_ctx,
                           build_config=build_config,
                           game_build_path=test_game_build_path,
                           engine_centric=engine_centric,
                           monolithic_build=True,
                           launcher_types=launcher_types,
                           allow_registry_overrides=allow_registry_overrides)

        if not build_game_launcher and not build_server_launcher and not build_unified_launcher:
            assert len(mock_process_command.call_args_list) == 0

        else:
            # Validate the cmake project generation calls
            mock_generate_process_input = mock_process_command.call_args_list[0][0][0]

            expected_project_path = test_engine_path if engine_centric else test_project_path

            expected_generate_args = [
                'cmake',
                '-B', test_game_build_path,
                '-S', expected_project_path,
                '-G', test_generator
            ]
            expected_generate_args.extend(test_generator_options)
            if engine_centric:
                expected_generate_args.extend([
                    f'-DLY_PROJECTS={test_project_path}'
                ])
            expected_generate_args.extend(
                [
                    '-DLY_MONOLITHIC_GAME=1',
                    '-DALLOW_SETTINGS_REGISTRY_DEVELOPMENT_OVERRIDES=1',
                ]
            )
            assert mock_generate_process_input == expected_generate_args

            # Validate the cmake project build calls
            mock_build_process_input = mock_process_command.call_args_list[1][0][0]
            expected_build_args = [
                'cmake',
                '--build', test_game_build_path,
                '--config', build_config,
                '--target'
            ]
            if build_server_launcher:
                expected_build_args.extend([f'{test_project_name}.ServerLauncher'])
            if build_game_launcher:
                expected_build_args.extend([f'{test_project_name}.GameLauncher'])
            if build_unified_launcher:
                expected_build_args.extend([f'{test_project_name}.UnifiedLauncher'])
            if test_additional_args:
                expected_build_args.extend(test_additional_args)

            assert mock_build_process_input == expected_build_args


@pytest.mark.parametrize("asset_platform", [
    pytest.param("linux")
])
def test_bundle_assets(tmp_path, asset_platform):

    test_project_path = tmp_path / "project"

    test_seedlist_file = "test.SeedList"
    test_seedlist_base_path = test_project_path / "AssetSeedLists"
    test_seedlist_base_path.mkdir(parents=True)
    test_seedlist_path = test_seedlist_base_path / test_seedlist_file
    test_seedlist_path.write_text("seedlist file")

    test_custom_asset_base_path = tmp_path / 'build' / "assets_custom"
    test_custom_asset_base_path.mkdir(parents=True)

    test_build_tool_path = tmp_path / "tools"
    test_build_tool_asset_bundler_path = test_build_tool_path / "bin" / "profile" / "AssetBundler"

    test_max_bundle_size = 2048

    with patch("o3de.export_project.process_command", return_value=0) as mock_process_command, \
         patch("o3de.export_project.get_asset_bundler_batch_path", return_value=test_build_tool_asset_bundler_path):

        mock_ctx = create_autospec(O3DEScriptExportContext)
        mock_ctx.project_path = test_project_path

        result = bundle_assets(ctx=mock_ctx,
                               selected_platforms=[asset_platform],
                               seedlist_paths=[test_seedlist_path],
                               seedfile_paths=[],
                               tools_build_path=test_build_tool_path,
                               engine_centric=False,
                               asset_bundling_path=test_custom_asset_base_path,
                               max_bundle_size=test_max_bundle_size)

        assert len(mock_process_command.call_args_list) == 4

        # Validate the expected call for generating the asset lists for the project
        for arg_index, assetlist_type in [(0, 'game'), (1, 'engine')]:
            mock_process_input = mock_process_command.call_args_list[arg_index][0]
            mock_process_input_arglist = mock_process_input[0]

            expected_assetlist_path = test_custom_asset_base_path / 'AssetLists' / f'{assetlist_type}_{asset_platform}.assetlist'

            expected_values = [
                test_build_tool_asset_bundler_path, # Test path to the fake AssetBundler
                'assetLists',                       # Main argument to the bundler
                '--assetListFile',                  # Argument parameter to the bundler
                expected_assetlist_path,
                '--platform',                       # Argument parameter '--platform' for the Asset Bundler
                asset_platform,                     # Argument value for '--platform'
                '--project-path',                   # Argument parameter '--project-path' for the Asset Bundler
                test_project_path,                  # Argument value for '--project-path'
                '--allowOverwrites'                # Argument parameter '--allowOverwrites' for the Asset Bundler
            ]
            if assetlist_type == 'engine':
                expected_values.extend(['--addDefaultSeedListFiles'])
            else:
                expected_values.extend([
                    '--seedListFile',                   # Argument parameter '--seedListFile' for the Asset Bundler
                    str(test_seedlist_path)           # Argument value for '--seedListFile'
                ])
            assert mock_process_input_arglist == expected_values

        # Validate the expected call for generating the asset bundles for the project
        for arg_index, assetlist_type in [(2, 'game'), (3, 'engine')]:
            mock_process_input = mock_process_command.call_args_list[arg_index][0]
            mock_process_input_arglist = mock_process_input[0]

            expected_assetlist_path = test_custom_asset_base_path / 'AssetLists' / f'{assetlist_type}_{asset_platform}.assetlist'
            expected_output_bundle_path = test_custom_asset_base_path / 'Bundles' / f'{assetlist_type}_{asset_platform}.pak'

            expected_values = [
                test_build_tool_asset_bundler_path, # Test path to the fake AssetBundler
                'bundles',                       # Main argument to the bundler
                '--maxSize',
                str(test_max_bundle_size),
                '--platform',                       # Argument parameter '--platform' for the Asset Bundler
                asset_platform,                     # Argument value for '--platform'
                '--project-path',                   # Argument parameter '--project-path' for the Asset Bundler
                test_project_path,                  # Argument value for '--project-path'
                '--allowOverwrites',               # Argument parameter '--allowOverwrites' for the Asset Bundler
                '--outputBundlePath',
                expected_output_bundle_path,
                '--assetListFile',
                expected_assetlist_path
            ]
            assert mock_process_input_arglist == expected_values


@pytest.mark.parametrize("build_config, asset_platform, project_files_to_copy, file_patterns_to_ignore, archive_format, project_name", [
    pytest.param("profile", "pc", ["include*.cfg"], ["ExcludeMe.ServerLauncher"], "zip", "testproject"),
    pytest.param("profile", "linux", ["include_me1.cfg", "include_me2.cfg"], ["*.ServerLauncher"], "zip", "testproject"),
    pytest.param("profile", "pc", ["include*.cfg"], ["ExcludeMe.ServerLauncher"], "none", "testproject"),
])
def test_setup_launcher_layout_directory(tmp_path, build_config, asset_platform, project_files_to_copy, file_patterns_to_ignore, archive_format, project_name):

    test_project_path = tmp_path / "project"
    test_project_path.mkdir()

    test_output_path = tmp_path / "output"
    test_output_path.mkdir()

    test_asset_platform = asset_platform

    test_build_config = build_config

    test_mono_build_path = tmp_path / "build"

    test_mono_build_path_bin_dir = test_mono_build_path / "bin" / test_build_config
    test_mono_build_path_bin_dir.mkdir(parents=True)

    test_include_mono_launcher_name = "IncludeMe.GameLauncher"
    test_include_mono_launcher_path = test_mono_build_path_bin_dir / test_include_mono_launcher_name
    test_include_mono_launcher_path.write_bytes(b'test_launcher bytes')

    test_exclude_mono_launcher_name = "ExcludeMe.ServerLauncher"
    test_exclude_mono_launcher_path = test_mono_build_path_bin_dir / test_exclude_mono_launcher_name
    test_exclude_mono_launcher_path.write_bytes(b'test_launcher bytes')

    test_bundle_file = tmp_path / "test_bundle.pak"
    test_bundle_file.write_bytes(b'test_pak bytes')

    test_project_files_to_copy = project_files_to_copy
    test_project_include_me1_cfg = test_project_path / "include_me1.cfg"
    test_project_include_me1_cfg.write_text("test include me1")
    test_project_include_me2_cfg = test_project_path / "include_me2.cfg"
    test_project_include_me2_cfg.write_text("test include me2")

    test_archive_output_format = archive_format

    export_layout = ExportLayoutConfig(
                output_path=test_output_path,
                project_file_patterns=test_project_files_to_copy,
                ignore_file_patterns=file_patterns_to_ignore)

    setup_launcher_layout_directory(project_path=test_project_path,
                                    project_name=project_name,
                                    asset_platform=test_asset_platform,
                                    launcher_build_path=test_mono_build_path,
                                    build_config=test_build_config,
                                    bundles_to_copy=[test_bundle_file],
                                    export_layout=export_layout,
                                    archive_output_format=test_archive_output_format,
                                    logger=None)

    result_bundle_file = test_output_path / "Cache" / test_asset_platform / "test_bundle.pak"
    assert result_bundle_file.is_file(), f"Missing <output>/Cache/{test_asset_platform}/test_bundle.pak"

    result_include_me1 = test_output_path / "include_me1.cfg"
    assert result_include_me1.is_file(), "Missing <output>/include_me1.cfg"

    result_include_me2 = test_output_path / "include_me2.cfg"
    assert result_include_me2.is_file(), "Missing <output>/include_me2.cfg"

    result_include_game_launcher = test_output_path / test_include_mono_launcher_name
    assert result_include_game_launcher.is_file(), f"Missing <output>/{test_include_mono_launcher_name}"

    result_exclude_server_launcher = test_output_path / test_exclude_mono_launcher_name
    assert not result_exclude_server_launcher.is_file(), f"<output>/{test_exclude_mono_launcher_name} not excluded"

    if test_archive_output_format != "none":
        result_archive_file = tmp_path / f"output.{test_archive_output_format}"
        assert result_archive_file.is_file(), f"Missing <output>.{test_archive_output_format}"

    setregpatch_file = test_output_path / 'Registry/IgnoreAssetProcessor.profile.setregpatch'
    if build_config == 'profile':
        assert (test_output_path / 'Registry').exists()
        assert setregpatch_file.is_file()
        with open(setregpatch_file,'r') as pf:
            assert 'bg_ConnectToAssetProcessor' in pf.read()
    else:
        assert not setregpatch_file.is_file()

@pytest.mark.parametrize("project_path, create_files, check_abs_files, check_rel_files, expect_error",[
    pytest.param("project", ["project/SeedLists/seed1", "project/SeedLists/seed2"], ["project/SeedLists/seed1", "project/SeedLists/seed2"], [], False),
    pytest.param("project", ["project/SeedLists/seed1", "project/SeedLists/seed2"], ["project/SeedLists/seed1"], ["SeedLists/seed2"], False),
    pytest.param("project", ["project/SeedLists/seed1", "project/SeedLists/seed2"], ["project/SeedLists/seed1", "project/SeedLists/seed3"], [], True)
])
def test_validate_project_artifact_paths(tmp_path, project_path, create_files, check_abs_files, check_rel_files, expect_error):

    test_project_path = tmp_path / project_path
    test_project_path.mkdir(parents=True, exist_ok=True)
    for create_file in create_files:
        create_file_path = tmp_path / create_file
        create_file_path.parent.mkdir(parents=True, exist_ok=True)
        create_file_path.write_text(create_file)
    artifact_list = []
    for check_abs_file in check_abs_files:
        check_abs_file_path = tmp_path / check_abs_file
        artifact_list.append(check_abs_file_path)
    for check_rel_file in check_rel_files:
        artifact_list.append(pathlib.Path(check_rel_file))

    try:
        validated_list = validate_project_artifact_paths(project_path=test_project_path,
                                                         artifact_paths=artifact_list)
    except ExportProjectError:
        assert expect_error, "Error condition expected"
    else:
        assert not expect_error
        assert len(artifact_list) == len(validated_list)
        for index in range(len(artifact_list)):
            original = artifact_list[index]
            validated = validated_list[index]
            if not original.is_absolute():
                original = test_project_path / original
            assert original == validated

@pytest.mark.parametrize("input_args, expected_export_args, expected_cca, expected_cba" ,[
    pytest.param(['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO', '-cca', '-DLY_STRIP_DEBUG_SYMBOLS=ON', '/', '-cba', '--', '/m:20'],
                 ['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO'],
                 ['-DLY_STRIP_DEBUG_SYMBOLS=ON'],
                 ['--', '/m:20'], id='case 1'),
    pytest.param(['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO', '--cmake-configure-arg', '-DLY_STRIP_DEBUG_SYMBOLS=ON', '/', '--cmake-build-arg', '--', '/m:20'],
                 ['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO'],
                 ['-DLY_STRIP_DEBUG_SYMBOLS=ON'],
                 ['--', '/m:20'], id='case 2'),
    pytest.param(['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-cca',
                  '-DLY_STRIP_DEBUG_SYMBOLS=ON', '/', '-nounified', '-gl', '-assets', '-ll', 'INFO', '-cba', '--', '/m:20'],
                 ['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO'],
                 ['-DLY_STRIP_DEBUG_SYMBOLS=ON'],
                 ['--', '/m:20'], id='case 3'),
    pytest.param(['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-cba',
                  '--', '/m:20', '-cca', '-DLY_STRIP_DEBUG_SYMBOLS=ON', '/', '-nounified', '-gl', '-assets', '-ll', 'INFO'],
                 ['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO'],
                 ['-DLY_STRIP_DEBUG_SYMBOLS=ON'],
                 ['--', '/m:20'], id='case 4'),
    pytest.param(['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO', '-cca', '-DLY_STRIP_DEBUG_SYMBOLS=ON', '/'],
                 ['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO'],
                 ['-DLY_STRIP_DEBUG_SYMBOLS=ON'],
                 [], id='case 5'),
    pytest.param(['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO', '-cba', '--', '/m:20'],
                 ['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO'],
                 [],
                 ['--', '/m:20'], id='case 6'),
    pytest.param(['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO', '-cca', '-cba'],
                 ['-pp', 'o3de', '-es', 'export_mps.py', '-out', 'export', '-cfg', 'profile', '-a', 'zip', '-nounified', '-gl',
                  '-assets', '-ll', 'INFO'],
                 [],
                 [], id='case 7')
  ])
def test_extract_cmake_custom_args(input_args, expected_export_args, expected_cca, expected_cba):
    result_export_args, result_cca_args, result_cba_args = extract_cmake_custom_args(input_args)
    assert result_export_args == expected_export_args
    assert result_cca_args == expected_cca
    assert result_cba_args == expected_cba


@pytest.mark.parametrize("project_path, create_files, check_files, expected_result",[
    pytest.param("project", ["project/SeedLists/seed1", "project/SeedLists/seed2"], ["SeedLists/seed1", "SeedLists/seed2"], ["SeedLists/seed1", "SeedLists/seed2"]),
    pytest.param("project", ["project/SeedLists/seed1", "project/SeedLists/seed2"], ["SeedLists/seed*"], ["SeedLists/seed1", "SeedLists/seed2"]),
    pytest.param("project", ["project/SeedLists/seed1", "project/SeedLists/seed2"], ["SeedLists/seed1", "SeedLists/seed*"], ["SeedLists/seed1", "SeedLists/seed2"]),
    pytest.param("project", ["project/SeedLists/seed1", "project/SeedLists/seed2", "project/SeedLists/seed3"], ["SeedLists/*1"], ["SeedLists/seed1"])
])
def test_preprocess_seed_path_list(tmp_path, project_path, create_files, check_files, expected_result):

    test_project_path = tmp_path / project_path
    test_project_path.mkdir(parents=True, exist_ok=True)
    for create_file in create_files:
        create_file_path = tmp_path / create_file
        create_file_path.parent.mkdir(parents=True, exist_ok=True)
        create_file_path.write_text(create_file)

    path_list = []
    for check_file in check_files:
        path_list.append(test_project_path / check_file)

    expected_path_list = [test_project_path / path for path in expected_result]
    result_path_list = preprocess_seed_path_list(project_path=test_project_path,
                                                 paths=path_list)
    assert expected_path_list == result_path_list

    abs_path_list = []
    for check_file in check_files:
        abs_path_list.append(test_project_path / check_file)
    result_path_list = preprocess_seed_path_list(project_path=test_project_path,
                                                 paths=path_list)
    assert expected_path_list == result_path_list


