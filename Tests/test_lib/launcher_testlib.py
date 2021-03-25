"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

This launcher_testlib file is used for a collection of reusable functionality that QA will use in their scripts.
"""

import os

import test_tools.shared.asset_processor_utils as asset_processor_utils
import test_tools.shared.file_utils as file_utils
import shared.shader_compile_server_utils as compile_server
import test_tools.shared.file_system as file_system


def setup_win_launcher_test(launcher):
    """
    Assert that the launcher was able to build and that assets were processed successfully.

    :param launcher: The test-tools Launcher to be built.
    """
    assert_build_success(launcher)
    assert_process_assets(launcher)


def setup_mac_launcher_test(launcher, configuration):
    """
    Assert that the launcher was able to build and that assets were processed successfully. Also starts the Mac
    shader compile server.

    :param launcher: The test-tools Launcher to be built.
    :param configuration: The shader compile server configuration to be launched.
    """
    assert_build_success(launcher)
    assert_process_assets(launcher)
    start_mac_shader_compile_server(launcher, configuration, '127.0.0.1')


def setup_android_launcher_test(launcher, configuration, win_x64_compiler_version):
    """
    Builds the Windows asset processor, asserts that the launcher was able to build and that assets were processed
    successfully, and starts the Windows shader compile server.

    :param launcher: The test-tools Launcher to be built.
    :param configuration: The shader compile server configuration to be launched.
    :param win_x64_compiler_version: The msvc compiler version used to build the shader compile server executable.
    """
    assert_asset_processor_build_success(launcher.workspace.release, win_x64_compiler_version)
    assert_build_success(launcher)
    assert_process_assets(launcher)

    start_shader_compile_server(launcher, configuration, '127.0.0.1', win_x64_compiler_version)


def assert_asset_processor_build_success(release, win_x64_compiler_version):
    """
    Builds the win_x64 AssetProcessor and asserts on build failure.

    For use with platforms that require Windows tools but not the Windows launcher.

    :param release: The test-tools Release which holds path info for the test environment in use.
    :param win_x64_compiler_version: The msvc compiler version used to build the AssetProcessor.
    """
    asset_proc_build_success = asset_processor_utils.build_win_x64(release.paths.dev(), release.configuration,
                                                                   win_x64_compiler_version)
    assert asset_proc_build_success, "AssetProcessor did not build properly"


def assert_process_assets(launcher):
    """
    Runs the Asset Processor Batch and asserts if any assets fail to process.

    :param launcher: The test-tools Launcher to be built.
    """
    process_assets_success = launcher.workspace.release.process_assets()
    assert process_assets_success, 'Assets did not process correctly'


def assert_build_success(launcher):
    """
    Runs the build command for specified launcher configuration and asserts if the build fails.

    :param launcher: The test-tools Launcher to be built.
    """
    build_success = launcher.workspace.release.build()
    assert build_success, "{} - Build Failed! - {} {} {}".format(launcher.workspace.release.project,
                                                                 launcher.workspace.release.platform,
                                                                 launcher.workspace.release.configuration,
                                                                 launcher.workspace.release.spec)

def start_shader_compile_server(launcher, configuration, ip_addr, win_x64_compiler_version):
    """
    Deals with the extra setup to start the Windows shader compiler.

    :param launcher: The test-tools Launcher to be built.
    :param configuration: The shader compile server configuration to be launched.
    :param ip_addr: The IP address of the remote shader compile server and AssetProcessor.
    :param win_x64_compiler_version: The msvc compiler version used to build the shader compile server executable.
    """
    launcher.workspace.release.modify_bootstrap_setting('remote_ip', ip_addr)
    launcher.workspace.release.modify_platform_setting('r_ShaderCompilerServer', ip_addr)
    launcher.workspace.release.modify_platform_setting('log_RemoteConsoleAllowedAddresses', ip_addr)
    compile_server.start_shader_compile_server(launcher.workspace.release.paths.tools(), configuration,
                                               win_x64_compiler_version)


def start_mac_shader_compile_server(launcher, configuration, ip_addr='127.0.0.1'):
    """
    Deals with the extra setup to start the Mac shader compiler.

    :param launcher: The test-tools Launcher to be built.
    :param configuration: The shader compile server configuration to be launched.
    :param ip_addr: The IP address of the remote shader compile server and AssetProcessor.
    """
    launcher.workspace.release.modify_bootstrap_setting('remote_ip', ip_addr)
    launcher.workspace.release.modify_platform_setting('r_ShaderCompilerServer', ip_addr)
    launcher.workspace.release.modify_platform_setting('log_RemoteConsoleAllowedAddresses', ip_addr)
    compile_server.start_mac_shader_compile_server(launcher.workspace.release.paths.tools(), configuration)


def set_launcher_startup_config_file(launcher, project_name, config_name, commands):
    """
    Clears out the specified config_name file and adds the commands specified to the cfg file.

    :param launcher: The test-tools Launcher to be built.
    :param project_name: Name of the game project in test.
    :param config_name: Name of the config file holding the launcher's startup commands.
    :param commands: List of commands to append to the specified config file.
    """
    file_utils.clear_out_config_file(launcher.workspace.release.paths.project(), config_name)
    config_path = os.path.join(launcher.workspace.release.paths.platform_cache(), project_name)
    file_utils.add_commands_to_config_file(config_path, '{}.cfg'.format(config_name), commands)


def configure_setup(launcher, delete_logs=True, delete_shaders=True):
    """
    Deletes old artifact folders and clears out any config files.

    :param launcher:
    :param delete_logs: Delete the game project's logs directory if True.
    :param delete_shaders: Delete the project's cache directory if True.
    """
    if delete_logs:
        delete_project_logs(launcher)

    if delete_shaders:
        delete_shader_cache(launcher)

    file_utils.delete_screenshot_folder(launcher.workspace.release.paths.platform_cache())

    file_utils.clear_out_config_file(launcher.workspace.release.paths.project(), 'initialmap')
    file_utils.clear_out_config_file(launcher.workspace.release.paths.project(), 'autoexec')


def delete_project_logs(launcher):
    """
    Deletes project logs in the launcher's project folder.
    """
    if os.path.exists(launcher.workspace.release.paths.project_log()):
        file_system.delete([launcher.workspace.release.paths.project_log()], True, True)


def delete_shader_cache(launcher, asset_type="pc"):
    """
    Deletes shader cache in the launcher's project folder.
    """
    user_folder = os.path.join(launcher.workspace.release.paths.project_cache(), asset_type, "user", "cache")
    if os.path.exists(user_folder):
        file_system.delete([user_folder], True, True)


def retry_console_command(remote_console, command, output, tries=10, timeout=10):
    """
    Retries specified console command multiple times and asserts if it still can not send.
    :param remote_console: the remote console connected to the launcher.
    :param command: the command to send to the console.
    :param output: The expected output to check if the command was sent successfully.
    :param tries: The amount of times to try before asserting.
    :param timeout: The amount of time in seconds to wait for each retry send.
    :return: True if succeeded, will assert otherwise.
    """
    while tries > 0:
        check_command = remote_console.expect_log_line(output, timeout)
        remote_console.send_command(command)
        if check_command():
            return True
        tries -= 1
    assert False, "Command \"{}\" failed to run in remote console.".format(command)
