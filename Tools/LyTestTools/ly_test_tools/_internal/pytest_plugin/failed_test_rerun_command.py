"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
import sys

from ly_test_tools import WINDOWS


def _get_test_launcher_cmd(build_dir=None):
    """
    Helper function to determine the test launcher command for the current platform
    :param build_dir: the --build-directory arg that is passed to determine which build dir to use. Can also be None
    :return: The test launcher command
    """
    build_arg = ""
    if build_dir:
        build_arg = f" --build-directory {build_dir} "

    python_runner = "python.cmd"
    if not WINDOWS:
        python_runner = "python.sh"
    current_dir = sys.executable

    # Look upward a handful of levels to check for the LY python entry point script
    # Assumes folder structure similar to: /Python/python.cmd
    for _ in range(10):
        python_wrapper = os.path.join(current_dir, python_runner)
        if os.path.exists(python_wrapper):
            return f"{python_wrapper} -m pytest{build_arg} "
        # Using an explicit else to avoid aberrant behavior from following filesystem links
        else:
            current_dir = os.path.abspath(os.path.join(current_dir, os.path.pardir))

    return f"{sys.executable} -m pytest{build_arg} "


def _format_cmd(launcher_cmd, test_path, nodeid):
    """
    Builds a command with required arguments to run a test

    :param launcher_cmd: The test launcher command
    :param test_path: File or directory that contains the test(s) that were run
    :param nodeid: A test node id, with parametrized values
    :return: Formatted command to re-run a test with parametrized values
    """

    # Assign the test id argument accordingly:
    # the node id (with parameters) is already part of the test path argument
    # when a single test case was invoked or
    # the node id (with parameters) includes the test filename when a whole
    # test module was invoked else
    # append the nodeId to the path when a whole test directory was invoked
    if nodeid == os.path.split(test_path)[-1]:
        test_id_argument = test_path
    elif os.path.split(test_path)[-1] in nodeid:
        test_id_argument = os.path.abspath(
            os.path.join(os.path.dirname(test_path), nodeid))
    else:
        test_id_argument = os.path.abspath(
            os.path.join(test_path, os.path.normpath(nodeid)))

    return f"{launcher_cmd}{test_id_argument}"


def build_rerun_commands(test_path, nodeids, build_dir=None):
    """
    Builds a list of commands to run tests

    :param test_path: File or directory that contains the test(s) that were run
    :param nodeids: List of test node ids, with parametrized values
    :param build_dir: the --build-directory arg that is passed to determine which build dir to use. Can also be None
    :return: A list of commands to re-run tests
    """

    commands = []
    test_launcher_cmd = _get_test_launcher_cmd(build_dir)

    for nodeid in nodeids:
        commands.append(_format_cmd(test_launcher_cmd, test_path, nodeid))

    return commands
