"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os
import pytest
import sys

import ly_test_tools.environment.file_system as fs
from .utils.FileManagement import FileManagement as fm

sys.path.append(os.path.dirname(os.path.abspath(__file__)) + '/../automatedtesting_shared')

from base import TestAutomationBase

@pytest.mark.parametrize("launcher_platform", ['windows_editor'])
@pytest.mark.parametrize("project", ["AutomatedTesting"])
class TestUtils(TestAutomationBase):
    @fm.file_revert("UtilTest_Physmaterial_Editor_TestLibrary.physmaterial", r"AutomatedTesting\Levels\Physics\Physmaterial_Editor_Test")
    def test_physmaterial_editor(self, request, workspace, launcher_platform, editor):
        """
        Tests functionality of physmaterial editing utility
        :param workspace: Fixture containing platform and project detail
        :param request: Built in pytest object, and is needed to call the pytest "addfinalizer" teardown command.
        :editor: Fixture containing editor details
        """
        from .utils import UtilTest_Physmaterial_Editor as physmaterial_editor_test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, physmaterial_editor_test_module, expected_lines, unexpected_lines, enable_prefab_system=False)
    
    def test_UtilTest_Tracer_PicksErrorsAndWarnings(self, request, workspace, launcher_platform, editor):
        from .utils import UtilTest_Tracer_PicksErrorsAndWarnings as testcase_module
        self._run_test(request, workspace, editor, testcase_module, [], [], enable_prefab_system=False)

    def test_FileManagement_FindingFiles(self, workspace, launcher_platform):
        """
        Tests the functionality of "searching for files" with FileManagement._find_files()
        :param workspace: ly_test_tools workspace fixture
        :return: None
        """
        # Set up known files and paths
        search_dir = os.path.join(workspace.paths.project(), "Levels", "Utils", "Managed_Files")
        find_me_path = search_dir
        first_also_find_me_path = os.path.join(search_dir, "some_folder")
        second_also_find_me_path = os.path.join(search_dir, "some_folder", "another_folder", "one_more_folder")
        find_me_too_path = os.path.join(search_dir, "some_folder", "another_folder")

        # Test for expected failures
        call_failed = False
        try:
            # Should throw a warning from finding too many copies of "AlsoFindMe.txt"
            fm._find_files(["FindMe.txt", "AlsoFindMe.txt", "FindMeToo.txt"], search_dir, search_subdirs=True)
        except RuntimeWarning:
            call_failed = True
        assert call_failed, "_find_files() DID NOT fail when finding multiple files"

        call_failed = False
        try:
            # Should throw a warning from not finding "DNE.txt"
            fm._find_files(["FindMe.txt", "DNE.txt"], search_dir, search_subdirs=True)
        except RuntimeWarning:
            call_failed = True
        assert call_failed, "_find_files() DID NOT fail when looking for a non-existent file"

        # Expected successful usages
        found_me = fm._find_files(["FindMe.txt"], search_dir)
        also_found_me_1 = fm._find_files(["AlsoFindMe.txt"], os.path.join(search_dir, "some_folder"))
        also_found_me_2 = fm._find_files(
            ["AlsoFindMe.txt"], os.path.join(search_dir, "some_folder", "another_folder", "one_more_folder")
        )
        find_a_few = fm._find_files(["FindMe.txt", "FindMeToo.txt"], search_dir, search_subdirs=True)

        # Test results
        assert (
            found_me["FindMe.txt"] == find_me_path
        ), "FindMe.txt path was not as expected. Expected: {} Found: {} ".format(find_me_path, found_me["FindMe.txt"])
        assert (
            also_found_me_1["AlsoFindMe.txt"] == first_also_find_me_path
        ), "First AlsoFindMe.txt path was not as expected. Expected: {} Found: {} ".format(
            first_also_find_me_path, also_found_me_1["AlsoFindMe.txt"]
        )
        assert (
            also_found_me_2["AlsoFindMe.txt"] == second_also_find_me_path
        ), "Second AlsoFindMe.txt path was not as expected. Expected: {} Found: {} ".format(
            second_also_find_me_path, also_found_me_2["AlsoFindMe.txt"]
        )
        for file_name, file_path in find_a_few.items():
            assert file_name == "FindMe.txt" or file_name == "FindMeToo.txt", "{} was found on accident".format(
                file_name
            )
        assert (
            "FindMe.txt" in find_a_few and find_a_few["FindMe.txt"] is not None
        ), "Finding multiple files did not find 'FindMe.txt'"
        assert (
            find_a_few["FindMe.txt"] == find_me_path
        ), "FindMe.txt path was not as expected. Expected: {} Found: {} ".format(find_me_path, found_me["FindMe.txt"])
        assert (
            "FindMeToo.txt" in find_a_few and find_a_few["FindMeToo.txt"] is not None
        ), "Finding multiple files did not find 'FindMeToo.txt'"
        assert (
            find_a_few["FindMeToo.txt"] == find_me_too_path
        ), "FindMeToo.txt path was not as expected. Expected: {} Found: {} ".format(
            find_me_too_path, found_me["FindMeToo.txt"]
        )

    def test_FileManagement_FileBackup(self, workspace, launcher_platform):
        """
        Tests the functionality of the file back up system via the FileManagement class
        :param workspace: ly_test_tools workspace fixture
        :return: None
        """

        def get_contents(file_path):
            # type: (str) -> [str]
            """
            Simply gets the contents of a file specified by the [file_path]
            :param file_path: file to get contents of
            :return: a list of strings
            """
            lines = []
            with open(file_path, "r") as file:
                for line in file:
                    lines.append(line)
            return lines

        # Set up known paths and file names
        target_folder = os.path.join(workspace.paths.project(), "Levels", "Utils", "Managed_Files")
        target_file_path = os.path.join(target_folder, "FindMe.txt")
        backup_folder = fm.backup_folder_path

        # Make sure the target file exists
        assert os.path.exists(target_file_path), "Target file: {} does not exist".format(target_file_path)

        original_contents = get_contents(target_file_path)
        fm._backup_file("FindMe.txt", target_folder)

        # Get newly-created backup file full path
        file_map = fm._load_file_map()
        assert target_file_path in file_map.keys(), "File path {} was not added to FileManagement file_map"
        backup_file_path = os.path.join(backup_folder, file_map[target_file_path])

        # Ensure back up worked as expected
        assert os.path.exists(target_file_path), "Original file got destroyed: {}".format(target_file_path)
        for i, line in enumerate(get_contents(target_file_path)):
            assert (
                original_contents[i] == line
            ), "Original file ({}) changed after being backed up. Line[{}]: expected: {}, found: {}".format(
                target_file_path, i, original_contents[i], line
            )

        for i, line in enumerate(get_contents(backup_file_path)):
            assert (
                original_contents[i] == line
            ), "Backed up file ({}) different from original. Line[{}]: expected: {}, found: {}".format(
                backup_file_path, i, original_contents[i], line
            )

        # Delete up back up file and clear entry in file map
        fs.delete([backup_file_path], True, False)
        del file_map[target_file_path]
        fm._save_file_map(file_map)

    def test_FileManagement_FileRestoration(self, workspace, launcher_platform):
        """
        Tests the restore file system via the FileManagement class
        :param workspace: ly_test_tools workspace fixture
        :return: None
        """

        def get_contents(file_path):
            # type: (str) -> [str]
            """
            Simply gets the contents of a file specified by the [file_path]
            :param file_path: file to get contents of
            :return: a list of strings
            """
            lines = []
            with open(file_path, "r") as file:
                for line in file:
                    lines.append(line)
            return lines

        # Set know files and paths
        target_folder = os.path.join(workspace.paths.project(), "Levels", "Utils", "Managed_Files")
        target_file_path = os.path.join(target_folder, "FindMe.txt")
        backup_folder = fm.backup_folder_path

        assert os.path.exists(target_file_path), "Target file: {} does not exist".format(target_file_path)

        # original file back up
        original_contents = get_contents(target_file_path)
        fm._backup_file("FindMe.txt", target_folder)

        # Get newly-created backup file full path
        file_map = fm._load_file_map()
        assert target_file_path in file_map.keys(), "File path {} was not added to FileManagement file_map".format(
            target_file_path
        )
        backup_file_path = os.path.join(backup_folder, file_map[target_file_path])

        assert os.path.exists(backup_file_path), "Back up file {} was not created".format(backup_file_path)

        # Makes sure back up is exactly the same as original
        for i, line in enumerate(get_contents(backup_file_path)):
            assert (
                original_contents[i] == line
            ), "Backed up file ({}) different than original. Line[{}]: expected: {}, found: {}".format(
                backup_file_path, i, original_contents[i], line
            )

        # Change original file
        fs.unlock_file(target_file_path)
        text_to_write = "Some new text to write into the target file!"
        with open(target_file_path, "w") as target_file:
            target_file.write(text_to_write)
        fs.lock_file(target_file_path)

        # Make sure original actually got changed
        changed_lines = get_contents(target_file_path)
        assert changed_lines[0] == text_to_write, "The file {} did not get changed while writing in test".format(
            target_file_path
        )
        assert changed_lines[0] != original_contents[0], "The file {} did not get changed while writing in test".format(
            target_file_path
        )

        # Restore the file!
        fm._restore_file("FindMe.txt", target_folder)

        # Ensure backup file was deleted after restoration
        assert not os.path.exists(backup_file_path), "Backup file {} WAS NOT deleted on restoration".format(
            backup_file_path
        )

        # Ensure saved file map was updated
        file_map = fm._load_file_map()
        assert (
            target_file_path not in file_map.keys()
        ), "File path {} was not cleared from file map after restoration".format(target_file_path)

        # Ensure restore brought original file back to it's default state
        restored_lines = get_contents(target_file_path)
        assert restored_lines[0] != text_to_write, "File restore for {} did not work".format(target_file_path)
        for restored, original, i in zip(restored_lines, original_contents, range(len(original_contents))):
            assert (
                restored == original
            ), "Restored file varies from original. Line [{}]: original: {}, restored: {}".format(i, original, restored)

        # Clean up back up folder and file
        fs.delete([backup_file_path], True, False)

    # Demo of how to use the managed_files system in your testcase
    @fm.file_revert_list(
        ["FindMe.txt", "FindMeToo.txt"], parent_path=r"AutomatedTesting\levels\Utils\Managed_files", search_subdirs=True
    )
    @fm.file_override("default.physxconfiguration", "UtilTest_PhysxConfig_Override.physxconfiguration")
    def test_UtilTest_Managed_Files(self, request, workspace, editor, launcher_platform):
        from .utils import UtilTest_Managed_Files as test_module

        expected_lines = []
        unexpected_lines = ["Assert"]
        self._run_test(request, workspace, editor, test_module, expected_lines, unexpected_lines, enable_prefab_system=False)
