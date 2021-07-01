"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import shutil
import logging
import stat

import ly_test_tools.environment.file_system as file_system
import ly_test_tools.environment.waiter as waiter

logger = logging.getLogger(__name__)


def clear_out_file(file_path):
    """
    Clears out the specified config file to be empty.
    :param file_path: The full path to the file.
    """
    if os.path.exists(file_path):
        file_system.unlock_file(file_path)
        with open(file_path, 'w') as file_to_write:
            file_to_write.write('')
    else:
        logger.debug(f'{file_path} not found while attempting to clear out file.')


def add_commands_to_config_file(config_file_dir, config_file_name, command_list):
    """
    From the command list, appends each command to the specified config file.
    :param config_file_dir: The directory the config file is contained in.
    :param config_file_name: The config file name.
    :param command_list: The commands to add to the file.
    :return:
    """
    config_file_path = os.path.join(config_file_dir, config_file_name)
    os.chmod(config_file_path, stat.S_IWRITE | stat.S_IREAD | stat.S_IEXEC)
    with open(config_file_path, 'w') as launch_config_file:
        for command in command_list:
            launch_config_file.write("{}\n".format(command))


def gather_error_logs(workspace):
    """
    Grabs all error logs (if there are any) and puts them into the specified logs path.
    :param workspace: The AbstractWorkspaceManager object that contains all of the paths
    """
    error_log_path = os.path.join(workspace.paths.project_log(), 'error.log')
    error_dump_path = os.path.join(workspace.paths.project_log(), 'error.dmp')
    if os.path.exists(error_dump_path):
        workspace.artifact_manager.save_artifact(error_dump_path)
    if os.path.exists(error_log_path):
        workspace.artifact_manager.save_artifact(error_log_path)


def delete_screenshot_folder(workspace):
    """
    Deletes screenshot folder from platform path
    :param workspace: The AbstractWorkspaceManager object that contains all of the paths
    """
    shutil.rmtree(workspace.paths.project_screenshots(), ignore_errors=True)


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
        waiter.wait_for(lambda: move_file_check(src_dir, dest_dir, file_name), timeout=timeout,
                        exc=IOError('Cannot move file {} while in use'.format(file_path)))


def move_file_check(src_dir, dest_dir, file_name):
    """
    Moves file and checks if the file has been moved from the source to the destination directory.
    :param src_dir: Source directory of the file to be moved
    :param dest_dir: Destination directory where the file should move to
    :param file_name: The name of the file to be moved
    :return:
    """
    try:
        shutil.move(os.path.join(src_dir, file_name), os.path.join(dest_dir, file_name))
    except OSError as e:
        logger.info(e)
        return False

    return True


def rename_file(file_path, dest_path, timeout=10):
    # type: (str, str, int) -> None
    """
    Renames a file by moving it. Waits for file to become available and raises and exception if timeout occurs.
    :param file_path: absolute path to the source file
    :param dest_path: absolute path to the new file
    :param timeout: timeout to wait for function to complete
    :return: None
    """
    def _rename_file_check():
        try:
            shutil.move(file_path, dest_path)
        except OSError as e:
            logger.debug(f'Attempted to rename file: {file_path} but an error occurred, retrying.'
                         f'\nError: {e}',
                         stackinfo=True)
            return False
        return True

    if os.path.exists(file_path):
        waiter.wait_for(lambda: _rename_file_check(), timeout=timeout,
                        exc=OSError('Cannot rename file {} while in use'.format(file_path)))


def delete_level(workspace, level_dir, timeout=120):
    """
    Attempts to delete an entire level folder from the project.
    :param workspace: The workspace instance to delete the level from.
    :param level_dir: The level folder to delete
    """

    if not level_dir:
        logger.warning("level_dir is empty, nothing to delete.")
        return

    full_level_dir = os.path.join(workspace.paths.project(), 'Levels', level_dir)
    if not os.path.isdir(full_level_dir):
        if os.path.exists(full_level_dir):
            logger.error("level '{}' isn't a directory, it won't be deleted.".format(full_level_dir))
        else:
            logger.info("level '{}' doesn't exist, nothing to delete.".format(full_level_dir))
        return

    waiter.wait_for(lambda: delete_check(full_level_dir),
                timeout=timeout,
                exc=IOError('Cannot delete directory {} while in use'.format(full_level_dir)))


def delete_check(src_dir):
    """
    Deletes directory and verifies that it's been deleted.
    :param src_dir: The directory to delete
    """
    try:
        def handle_delete_error(action, path, exception_info):
            # The deletion could fail if the file is read-only, so set the permissions to writeable and try again
            os.chmod(path, stat.S_IWRITE)
            # Try the passed-in action (delete) again
            action(path)

        shutil.rmtree(src_dir, onerror=handle_delete_error)
    except OSError as e:
        logger.debug("Delete for '{}' failed: {}".format(src_dir, e))
        return False

    return not os.path.exists(src_dir)

