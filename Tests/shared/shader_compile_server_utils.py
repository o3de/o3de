"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import logging
import os
import psutil
import subprocess

import test_tools.shared.file_system as file_system
from test_tools.shared.process_utils import kill_processes_named
from test_tools import HOST_PLATFORM

logger = logging.getLogger(__name__)


def start_shader_compile_server(tools_dir, build_config, win_x64_compiler, clean=True):
    """
    Starts the shader compile server for the given compiler version. Failure is currently not checked due to issues
    with detecting if the process is alive immediately after requesting it to be spun up.
    :param tools_dir: Tools directory inside of the dev root.
    :param build_config: Build configuration name (profile, debug, etc.).
    :param win_x64_compiler: Windows compiler version used to build the project.
    :param clean: Removes Cache, Error, Shaders, and Temp directories if set to True.
    """
    shader_compile_server_dir = os.path.join(tools_dir, 'CrySCompileServer', 'x64', build_config)
    os.chdir(shader_compile_server_dir)

    shader_compile_server_exe = build_shader_compile_server_file_name(win_x64_compiler)

    # Only one shader compile server can run at a time, so kill the currently running process if any
    kill_processes_named(shader_compile_server_exe)

    if clean:
        clean_shader_compile_server_files(tools_dir, build_config)

    logger.info("Attempting to start shader compile server")
    # Running with basic user permissions since the shader compile server warns against running as admin
    subprocess.Popen(['RunAs', '/trustlevel:0x20000', shader_compile_server_exe])


def start_mac_shader_compile_server(tools_dir, build_config, clean=True):
    """
    Mac version of starting shader compiler given build configuration. Failure is currently not checked due to issues
    with detecting if the process is alive immediately after requesting it to be spun up.
    :param tools_dir: Tools directory inside of the dev root.
    :param build_config: Build configuration name (profile, debug, etc.).
    :param clean: Removes Cache, Error, Shaders, and Temp directories if set to True.
    :return: Returns the actual process.
    """
    shader_compile_server_dir = os.path.join(tools_dir, 'CrySCompileServer', 'osx', build_config)
    os.chdir(shader_compile_server_dir)

    kill_processes_named('CrySCompileServer')

    if clean:
        clean_shader_compile_server_files(tools_dir, build_config)

    logger.info("Attempting to start shader compile server")
    subprocess.Popen('./CrySCompileServer', shell=True)


def build_shader_compile_server_file_name(win_x64_compiler):
    """
    Puts together the shader compile server file name based on the specified VC compiler version.
    :param win_x64_compiler: The VC compiler version specified in vsyyyy format, where yyyy is a year (ex: vs2017)
    :return: The shader compile server file name complete with extension.
    """
    shader_compile_server_exe = 'CrySCompileServer'

    return '{}.exe'.format(shader_compile_server_exe)


def clean_shader_compile_server_files(tools_dir, build_config):
    """
    Removes the shader compile server generated Cache, Error, Shaders, and Temp directories.
    :param tools_dir: Tools directory inside of the dev root.
    :param build_config: Build configuration name (profile, debug, etc.).
    """
    if HOST_PLATFORM == 'win_x64':
        shader_compile_server_dir = os.path.join(tools_dir, 'CrySCompileServer', 'x64', build_config)
    else:
        shader_compile_server_dir = os.path.join(tools_dir, 'CrySCompileServer', 'osx', build_config)
    os.chdir(shader_compile_server_dir)

    shader_compile_server_dirs = [os.path.join(shader_compile_server_dir, 'Cache'),
                                  os.path.join(shader_compile_server_dir, 'Error'),
                                  os.path.join(shader_compile_server_dir, 'Shaders'),
                                  os.path.join(shader_compile_server_dir, 'Temp')]
    if not file_system.delete(shader_compile_server_dirs, True, True):
        directories_still_present = []
        for directory in shader_compile_server_dirs:
            if os.path.exists(directory):
                directories_still_present.append(directory)
        raise RuntimeError("Failed to clean folders {} from directory {}".format(directories_still_present,
                                                                                 shader_compile_server_dir))


def stop_shader_compile_server():
    """
    Finds any process with CrySCompileServer in its name and kills it.
    """
    # This is necessary because the shader compile server is spawned from another process which
    # immediately terminates after spawning its child
    for process in psutil.process_iter():
        try:
            if 'CrySCompileServer' in process.name():
                success_code = process.kill()
                if success_code == 0:
                    logger.error("Failed to terminate CrySCompileServer process")
        except psutil.NoSuchProcess:
            # Process was already killed but caught as a zombie process, so pass as normal.
            pass
