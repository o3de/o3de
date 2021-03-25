"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os
import shutil
import subprocess
import logging

import test_tools.shared.file_system as file_system
from test_tools.shared.waiter import wait_for

logger = logging.getLogger(__name__)


def clear_out_config_file(project_path, script_name):
    """
    Clears out the specified config file to be empty.
    :param project_path: The directory where the file is.
    :param script_name: The file name.
    """
    path = os.path.join(project_path, '{}.cfg'.format(script_name))

    if os.path.exists(path):
        # Clears read only flag
        file_system.unlock_file(path)

        with open(path, 'w') as initialmap_script:
            initialmap_script.write('')


def add_commands_to_config_file(config_file_dir, config_file_name, command_list):
    """
    From the command list, appends each command to the specified config file.
    :param config_file_dir: The directory the config file is contained in.
    :param config_file_name: The config file name.
    :param command_list: The commands to add to the file.
    :return:
    """
    config_file_path = os.path.join(config_file_dir, config_file_name)
    os.chmod(config_file_path, 0755)
    with open(config_file_path, 'w') as launch_config_file:
        for command in command_list:
            launch_config_file.write("{}\n".format(command))


def gather_error_logs(dev_path, logs_path):
    """
    Grabs all error logs (if there are any) and puts them into the specified logs path.
    :param dev_path: Path to the dev directory.
    :param logs_path: Path to the destination directory for the logs (the test results path).
    """
    error_logs_sent = False
    error_log_path = os.path.join(dev_path, 'error.log')
    artifact_log_path = os.path.join(logs_path, 'error.log')
    error_dump_path = os.path.join(dev_path, 'error.dmp')
    artifact_dump_path = os.path.join(logs_path, 'error.dmp')
    if os.path.exists(error_dump_path) and os.path.exists(error_log_path):
        shutil.move(error_dump_path, artifact_dump_path)
        shutil.move(error_log_path, artifact_log_path)
        error_logs_sent = True
    return error_logs_sent


def delete_screenshot_folder(platform_path):
    """
    Deletes screenshot folder from platform path
    :param platform_path: Platform Path
    :return:  None
    """
    platform_path = r'{}\user\screenshots'.format(platform_path)
    shutil.rmtree(platform_path, ignore_errors=True)


def move_file(src_dir, dest_dir, file_name, timeout=120):
    """
    Attempts to move a file from the source directory to the destination directory. Raises an IOError if
    the file is in use.
    :param src_dir: Directory of the file to be moved.
    :param dest_dir: Directory where the file will be moved to.
    :param file_name: Name of the file to be moved.
    :param timeout: Number of seconds to wait for the file to be released.
    """
    file_path = os.path.join(src_dir, file_name)
    if os.path.exists(file_path):
        wait_for(lambda: move_file_check(src_dir, dest_dir, file_name),
                 timeout=timeout,
                 exc=IOError('Cannot move file {} while in use'.format(file_path)))


def move_file_check(src_dir, dest_dir, file_name):
    """
    Moves file and checks if the file has been moved from the source to the destination directory.
    """
    try:
        shutil.move(os.path.join(src_dir, file_name), os.path.join(dest_dir, file_name))
    except OSError as e:
        print e
        return False

    return True


def revert_config_files(launcher):
    """
    Reverts modified config files from test. Runs the perforce revert command.
    :param launcher: The launcher instance to revert files from.
    """
    files_revert = [os.path.join(launcher.workspace.release.paths.dev(), 'bootstrap.cfg'),
                    os.path.join(launcher.workspace.release.paths.dev(), 'AssetProcessorPlatformConfig.ini'),
                    launcher.workspace.release.paths.platform_config_file()]

    for file_path in files_revert:
        subprocess.check_call(['p4', 'revert', file_path])
        subprocess.check_call(['p4', 'sync', '-f', file_path])


def create_file(path, file_name):
    """
    Creates a file with specified file name
    :param path: The path of where the file needs to be created
    :param file_name: The file name
    :return:
    """
    file_path = os.path.join(path, file_name)

    if not os.path.exists(file_path):
        new_file = open(file_path, "w")
        new_file.close()
    else:
        print "{} already exists".format(file_name)

def delete_level(launcher, level_dir, timeout=120):
    """
    Attempts to delete an entire level folder from the project.
    :param launcher: The launcher instance to delete the level from.
    :param level_dir: The level folder to delete
    """

    if (not level_dir):
        logger.warning("level_dir is empty, nothing to delete.")
        return

    full_level_dir = os.path.join(launcher.workspace.release.paths.project(), 'Levels', level_dir)
    if (not os.path.isdir(full_level_dir)):
        if (os.path.exists(full_level_dir)):
            logger.error("level '{}' isn't a directory, it won't be deleted.".format(full_level_dir))
        else:
            logger.info("level '{}' doesn't exist, nothing to delete.".format(full_level_dir))
        return

    wait_for(lambda: delete_check(full_level_dir),
                timeout=timeout,
                exc=IOError('Cannot delete directory {} while in use'.format(full_level_dir)))

def delete_check(src_dir):
    """
    Deletes directory and verifies that it's been deleted.
    :param src_dir: The directory to delete
    """
    try:
        shutil.rmtree(src_dir)
    except OSError as e:
        logger.debug("Delete for '{}' failed: {}".format(src_dir, e))
        return False

    return (not os.path.exists(src_dir))

def get_log_file_path(launcher, project_name):
    """
    Creates a log file path.
    :param launcher: The launcher instance to get a path from.
    :param project_name: The name of the project.
    """
    return os.path.join(launcher.workspace.release.paths.dev(), 'Cache', project_name, 'pc', 'user', 'log', 'Editor.log')
