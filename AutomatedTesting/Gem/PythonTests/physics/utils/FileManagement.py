"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

"""

# Imports
import os
import shutil
import json
import logging
import ly_test_tools.environment.file_system as fs


class FileManagement:
    """
    File Management static class: Handles file backup and restoration.
    Organizes backing up files into a single directory and
    mapping the back up files to their original files.

    Features accessible via the exposed decorator functions.
    """

    # Static variables
    MAX_BACKUPS = 10000  # Arbitrary number to cap same-file-name name generating
    backup_folder_name = ".backup"
    backup_folder_path = os.path.join(os.path.split(__file__)[0], backup_folder_name)  # CWD plus backup folder name
    file_map_name = "_filebackup_map.json"  # JSON file to store original to back up file mappings

    @staticmethod
    def _load_file_map():
        # type: () -> {str: str}
        """
        Loads the FileManagement's json file map.
        The returned dictionary has key: value sets such that:
                keys:   full path to an original file currently being backed up
                values: name of the original file's backup file. Uses a numbering system for
                    storing multiple file names that are identical but located in different directories.
        If there is no current json file or the file cannot be parsed, an empty dictionary is returned.
        :return: Dictionary to map original file's to their back up file names.
        """
        json_file_path = os.path.join(FileManagement.backup_folder_path, FileManagement.file_map_name)
        if os.path.exists(json_file_path):
            try:
                with open(json_file_path, "r") as f:
                    return json.load(f)
            except ValueError:
                logging.info("Decoding JSON file {} failed. Empty dictionary used".format(json_file_path))
        return {}

    @staticmethod
    def _save_file_map(file_map):
        # type: ({str: str}) -> None
        """
        Saves the [file_map] dictionary as a json file.
        If [file_map] is empty, deletes the json file.
        :param file_map: A dictionary mapping original file paths to their back up file names.
        :return: None
        """
        file_path = os.path.join(FileManagement.backup_folder_path, FileManagement.file_map_name)
        if os.path.exists(file_path):
            fs.unlock_file(file_path)
        if len(file_map) > 0:
            with open(file_path, "w") as f:
                json.dump(file_map, f)
                fs.lock_file(file_path)
        else:
            fs.delete([file_path], True, False)

    @staticmethod
    def _next_available_name(file_name, file_map):
        # type: (str, {str: str}) -> str or None
        """
        Generates the next available backup file name using a FILE_NAME_x.ext naming convention
        where x is a number. Returns None if we've maxed out the numbering system.
        :param file_name: The base file to generate a back up file name for.
        :param file_map: The file_map for the FileManagement system
        :return: str
        """
        suffix, extension = file_name.split(".", 1)
        for i in range(FileManagement.MAX_BACKUPS):
            potential_name = suffix + "_" + str(i) + "." + extension
            if potential_name not in file_map.values():
                return potential_name
        return None

    @staticmethod
    def _backup_file(file_name, file_path):
        # type: (str, str) -> None
        """
        Backs up the [file_name] located at [file_path] into the FileManagement's backup folder.
        Leaves the backup file locked from writing privileges.
        :param file_name: The file's name that will be backed up
        :param file_path: The path to the file to back up
        :return: None
        """
        file_map = FileManagement._load_file_map()
        backup_path = FileManagement.backup_folder_path
        backup_file_name = "{}.bak".format(file_name)
        backup_file = os.path.join(backup_path, backup_file_name)
        # If backup directory DNE, make one
        if not os.path.exists(backup_path):
            os.mkdir(backup_path)
        # If "traditional" backup file exists, delete it (myFile.txt.bak)
        if os.path.exists(backup_file):
            fs.delete([backup_file], True, False)
        # Find my next storage name (myFile_1.txt.bak)
        backup_storage_file_name = FileManagement._next_available_name(backup_file_name, file_map)
        if backup_storage_file_name is None:
            # If _next_available_name returns None, we have backed up MAX_BACKUPS of files name [file_name]
            raise Exception(
                "FileManagement class ran out of backups per name. Max: {}".format(FileManagement.MAX_BACKUPS)
            )
        backup_storage_file = os.path.join(backup_path, backup_storage_file_name)
        if os.path.exists(backup_storage_file):
            # This file should not exists, but if it does it's about to get clobbered!
            fs.unlock_file(backup_storage_file)
        # Create "traditional" backup file (myFile.txt.bak)
        fs.create_backup(os.path.join(file_path, file_name), backup_path)
        # Copy "traditional" backup file into storage backup (myFile_1.txt.bak)
        FileManagement._copy_file(backup_file_name, backup_path, backup_storage_file_name, backup_path)
        fs.lock_file(backup_storage_file)
        # Delete "traditional" back up file
        fs.unlock_file(backup_file)
        fs.delete([backup_file], True, False)
        # Update file map with new file
        file_map[os.path.join(file_path, file_name)] = backup_storage_file_name
        FileManagement._save_file_map(file_map)
        # Unlock original file to get it ready to be edited by the test
        fs.unlock_file(os.path.join(file_path, file_name))

    @staticmethod
    def _restore_file(file_name, file_path):
        # type: (str, str) -> None
        """
        Restores the [file_name] located at [file_path] which is backed up in FileManagement's backup folder.
        Locks [file_name] from writing privileges when done restoring.
        :param file_name: The Original file that was backed up, and now restored
        :param file_path: The location of the original file that was backed up
        :return: None
        """
        file_map = FileManagement._load_file_map()
        backup_path = FileManagement.backup_folder_path
        src_file = os.path.join(file_path, file_name)
        if src_file in file_map:
            backup_file = os.path.join(backup_path, file_map[src_file])
            if os.path.exists(backup_file):
                fs.unlock_file(backup_file)
                fs.unlock_file(src_file)
                # Make temporary copy of backed up file to restore from
                temp_file = "{}.bak".format(file_name)
                FileManagement._copy_file(file_map[src_file], backup_path, temp_file, backup_path)
                fs.restore_backup(src_file, backup_path)
                fs.lock_file(src_file)
                # Delete backup file
                fs.delete([os.path.join(backup_path, temp_file)], True, False)
                fs.delete([backup_file], True, False)
                # Remove from file map
                del file_map[src_file]
                FileManagement._save_file_map(file_map)
                if not os.listdir(backup_path):
                    # Delete backup folder if it's empty now
                    os.rmdir(backup_path)

    @staticmethod
    def _find_files(file_names, root_dir, search_subdirs=False):
        # type: ([str], str, bool) -> {str:str}
        """
        Searches the [root_dir] directory tree for each of the file names in [file_names]. Returns a dictionary
        where keys = the entries in file_names, and their values are the paths they were found at.
        Raises a runtime warning if not exactly one copy of each file is found.
        :param file_names: A list of file_names to look for
        :param root_dir: The root directory to begin the search
        collect all file paths matching that file_name
        :param search_subdirs: Optional boolean flag for searching sub-directories of [parent_path]
        :return: A dictionary where keys are file names, and values are the file's path.
        """
        file_list = {}
        found_count = 0
        for file_name in file_names:
            file_list[file_name] = None
        if search_subdirs:
            # Search parent path for all file name arguments
            for dir_path, _, dir_files in os.walk(root_dir):
                for dir_file in dir_files:
                    if dir_file in file_list.keys():
                        if file_list[dir_file] is None:
                            file_list[dir_file] = dir_path
                            found_count += 1
                        else:
                            # Found multiple files with same name. Raise warning.
                            raise RuntimeWarning("Found multiple files with the name {}.".format(dir_file))
        else:
            for dir_file in os.listdir(root_dir):
                if os.path.isfile(os.path.join(root_dir, dir_file)) and dir_file in file_names:
                    file_list[dir_file] = root_dir
        not_found = [file_name for file_name in file_names if file_list[file_name] is None]
        if not_found:
            raise RuntimeWarning("Could not find the following files: {}".format(not_found))

        return file_list

    @staticmethod
    def _copy_file(src_file, src_path, target_file, target_path):
        # type: (str, str, str, str) -> None
        """
        Copies the [src_file] at located in [src_path] to the [target_file] located at [target_path].
        Leaves the [target_file] unlocked for reading and writing privileges
        :param src_file: The source file to copy (file name)
        :param src_path: The source file's path
        :param target_file: The target file to copy into (file name)
        :param target_path: The target file's path
        :return: None
        """
        target_file_path = os.path.join(target_path, target_file)
        src_file_path = os.path.join(src_path, src_file)
        if os.path.exists(target_file_path):
            fs.unlock_file(target_file_path)
        shutil.copyfile(src_file_path, target_file_path)

    @staticmethod
    def file_revert(file_name, parent_path=None, search_subdirs=False):
        # type: (str, str, bool) -> function
        """
        Function decorator:
        Convenience version of file_revert_list for passing a single file
        Calls file_revert_list on a list one [file_name]
        :param file_name: The name of a file to backup (not path)
        :param parent_path: The root directory to search for the [file_names] (relative to the dev folder).
                    Defaults to the project folder described by the inner function's workspace fixture
        :param search_subdirs: A boolean option for searching sub-directories for the files in [file_names]
        :return: function as per the function decorator pattern
        """
        return FileManagement.file_revert_list([file_name], parent_path, search_subdirs)

    @staticmethod
    def file_revert_list(file_names, parent_path=None, search_subdirs=False):
        # type: ([str], str, bool) -> function
        """
        Function decorator:
        Collects file names specified in [file_names] in the [parent_path] directory.
        Backs up these files before executing the parameter to the "wrap" function.
        If the search_subdirs flag is True, searches all subdirectories of [parent_path] for files.
        :param file_names: A list of file names (not paths)
        :param parent_path: The root directory to search for the [file_names] (relative to the dev folder).
                    Defaults to the project folder described by the inner function's workspace fixture
        :param search_subdirs: A boolean option for searching sub-directories for the files in [file_names]
        :return: function as per the function decorator pattern
        """

        def wrap(func):
            # type: (function) -> function
            def inner(self, request, workspace, editor, launcher_platform):
                # type: (...) -> None
                """
                Inner function to wrap around decorated function. Function being decorated must match this
                functions parameter order.
                """
                root_path = parent_path
                if root_path is not None:
                    root_path = os.path.join(workspace.paths.engine_root(), root_path)
                else:
                    # Default to project folder (AutomatedTesting)
                    root_path = workspace.paths.project()

                # Try to locate files
                try:
                    file_list = FileManagement._find_files(file_names, root_path, search_subdirs)
                except RuntimeWarning as w:
                    assert False, (
                        w.args[0]
                        + " Please check use of search_subdirs; make sure you are using the correct parent directory."
                    )

                # Restore from backups
                for file_name, file_path in file_list.items():
                    FileManagement._restore_file(file_name, file_path)

                # Create backups for each discovered path for each specified filename
                for file_name, file_path in file_list.items():
                    FileManagement._backup_file(file_name, file_path)

                try:
                    # Run the test
                    func(self, request, workspace, editor, launcher_platform)
                finally:
                    # Restore backups for each discovered path for each specified filename if they exist
                    for file_name, file_path in file_list.items():
                        FileManagement._restore_file(file_name, file_path)

            return inner

        return wrap

    @staticmethod
    def file_override(target_file, src_file, parent_path=None, search_subdirs=False):
        # type: (str, str, str, bool) -> function
        """
        Function decorator:
        Searches for [target_file] and [src_file] in [parent_path](relative to the project dev folder)
        and performs backup and file swapping. [target_file] is backed up and replaced with [src_file] before the
        decorated function executes.
        After the decorated function is executed [target_file] is restored to the state before being swapped.
        If [parent_path] is not specified, the project folder described by the current workspace will be used.
        If the search_subdirs flag is True, searches all subdirectories of [parent_path] for files.
        :param target_file: The name of the file being backed up and swapped into
        :param src_file: The name of the file to swap into [target_file]
        :param parent_path: The root directory to search for the [file_names] (relative to the dev folder).
                    Defaults to the project folder described by the inner function's workspace fixture
        :param search_subdirs: A boolean option for searching sub-directories for the files in [file_names]
        :return: The decorated function
        """

        def wrap(func):
            def inner(self, request, workspace, editor, launcher_platform):
                """
                Inner function to wrap around decorated function. Function being decorated must match this
                functions parameter order.
                """
                root_path = parent_path
                if root_path is not None:
                    root_path = os.path.join(workspace.paths.engine_root(), root_path)
                else:
                    # Default to project folder (AutomatedTesting)
                    root_path = workspace.paths.project()

                # Try to locate both target and source files
                try:
                    file_list = FileManagement._find_files([target_file, src_file], root_path, search_subdirs)
                except RuntimeWarning as w:
                    assert False, (
                        w.args[0]
                        + " Please check use of search_subdirs; make sure you are using the correct parent directory."
                    )

                FileManagement._restore_file(target_file, file_list[target_file])
                FileManagement._backup_file(target_file, file_list[target_file])
                FileManagement._copy_file(src_file, file_list[src_file], target_file, file_list[target_file])

                try:
                    # Run Test
                    func(self, request, workspace, editor, launcher_platform)
                finally:
                    FileManagement._restore_file(target_file, file_list[target_file])

            return inner

        return wrap
