"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

General Asset Relocator Tests
"""

# Import builtin libraries
import pytest
import logging
import os
import pathlib
import subprocess
import shutil
import sys
from dataclasses import dataclass
from dataclasses import field
from typing import List

# Import LyTestTools
import ly_test_tools
import ly_test_tools.builtin.helpers as helpers
import ly_test_tools.environment.file_system as fs
import ly_test_tools.environment.process_utils as process_utils
from ly_test_tools.o3de import asset_processor as asset_processor_utils
from ly_test_tools.o3de.asset_processor import ASSET_PROCESSOR_PLATFORM_MAP

# Import fixtures
from ..ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from ..ap_fixtures.ap_setup_fixture import ap_setup_fixture as ap_setup_fixture
from ..ap_fixtures.timeout_option_fixture import timeout_option_fixture as timeout
from ..ap_fixtures.clear_moveoutput_fixture import clear_moveoutput_fixture as clear_moveoutput
from ..ap_fixtures.clear_testingAssets_dir import clear_testingAssets_dir as clear_testingAssets_dir

# Import LyShared
import ly_test_tools.o3de.pipeline_utils as utils

# Use the following logging pattern to hook all test logging together:
logger = logging.getLogger(__name__)
# Configuring the logging is done in ly_test_tools at the following location:
# ~/dev/Tools/LyTestTools/ly_test_tools/log/py_logging_util.py

# Helper: variables we will use for parameter values in the test:
targetProjects = ["AutomatedTesting"]


@pytest.fixture
def local_resources(request, workspace, ap_setup_fixture):
    # Test-level asset folder. Directory contains a subfolder for each test (i.e. C1234567)
    ap_setup_fixture["tests_dir"] = os.path.dirname(os.path.realpath(__file__))



@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.usefixtures("clear_moveoutput")
@pytest.mark.usefixtures("clear_testingAssets_dir")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_periodic
class TestsAssetRelocator_WindowsAndMac(object):
    """
    Specific Tests for Asset Processor GUI To Only Run on Windows and Mac
    """

    @pytest.mark.test_case_id("C21968345")
    @pytest.mark.test_case_id("C21968346")
    @pytest.mark.test_case_id("C21968349")
    @pytest.mark.test_case_id("C21968350")
    @pytest.mark.assetpipeline
    @pytest.mark.parametrize(
        "testId,readonly,confirm,success",
        [
            ("C21968345", False, True, True),
            ("C21968346", False, False, False),
            ("C21968349", True, True, False),
            ("C21968350", False, True, True),
        ],
    )
    def test_WindowsMacPlatforms_RelocatorMoveFileWithConfirm_MoveSuccess(self, request, workspace, asset_processor,
                                                                          ap_setup_fixture, testId, readonly, confirm,
                                                                          success):
        """
        Tests whether tests with Move File Confirm are successful

        Test Steps:
        1. Create temporary testing environment
        2. Set move location
        3. Determine if confirm flag is set
        4. Attempt to move the files
        5. If confirm flag set:
           * Validate Move was successful
           * Else: Validate move was not successful
        """
        env = ap_setup_fixture
        copied_asset = ''

        def teardown():
            # Delete files created during test
            if not fs.delete([copied_asset], True, False):
                fs.unlock_file(copied_asset)
                fs.delete(copied_asset, True, False)

        request.addfinalizer(teardown)

        # testFile.txt is copied to the root of the dev/AutomatedTesting folder
        file_name = "testFile.txt"
        source_folder, _ = asset_processor.prepare_test_environment(env["tests_dir"], "C21968345")

        copied_asset = os.path.join(source_folder, file_name)
        assert os.path.exists(copied_asset), f"{copied_asset} does not exist"

        # Set the write-access for the file based on test case
        if readonly:
            fs.lock_file(copied_asset)

        else:
            fs.unlock_file(copied_asset)

        prefix = "C21968345"
        move_dir = os.path.join(f"MoveOutput_{testId}")
        moved_file = os.path.join(move_dir, file_name)

        # Command '--move' will create the MoveOutput directory for test when called
        relative_move_str = f"{os.path.join(prefix, file_name)},{os.path.join(prefix, moved_file)}"

        extraParams = [f"--move={relative_move_str}"]

        # Set the '--confirm' flag based on test case
        if confirm:
            extraParams = extraParams +["--confirm"]

        asset_processor.batch_process(extra_params=extraParams)

        # Assert success based on expected results from individual test cases
        if success:
            assert os.path.exists(os.path.join(source_folder, move_dir, file_name)), "The file was not moved when it was expected to"

        else:
            assert not os.path.exists(os.path.join(source_folder, move_dir, file_name)), "The file was expected to not move but was moved anyway"

    @pytest.mark.test_case_id("C21968385")
    @pytest.mark.assetpipeline
    def test_WindowsMacPlatforms_LeaveEmptyFoldersWithoutCommands_RelocatorWarnsCommandsNeeded(self, asset_processor,
                                                                                               ap_setup_fixture):
        """
        Run the Asset Processor Batch command with the '--LeaveEmptyFolders' flag
        User should be warned that LeaveEmptyFolders needs to be used with the move or delete command

        :return: None

        Test Steps:
        1. Create temporary testing environment
        2. Attempt to move with --LeaveEmptyFolders set
        3. Verify user is given a message that command requires to be used with --move or --delete
        """
        env = ap_setup_fixture
        expected_message = "Command --leaveEmptyFolders must be used with command --move or --delete"
        unexpected_message = "RELOCATION REPORT"

        asset_processor.prepare_test_environment(env["tests_dir"], "C21968385")

        # AssetProcessorBatch --LeaveEmptyFolders
        result, ap_batch_output = asset_processor.batch_process(capture_output=True, extra_params=[ "--LeaveEmptyFolders"])

        # Verify expected and unexpected messages in log
        utils.validate_log_output(ap_batch_output, [expected_message], [unexpected_message])

    @pytest.mark.test_case_id("C21968388")
    @pytest.mark.assetpipeline
    def test_WindowsMacPlatforms_MoveCorruptedPrefabFile_MoveSuccess(self, request, workspace, ap_setup_fixture,
                                                                    asset_processor):
        """
        Asset with UUID/AssetId reference in non-standard format is
        successfully scanned and relocated to the MoveOutput folder.
        This test uses a pre-corrupted .prefab file.

        Test Steps:
        1. Create temporary testing environment with a corrupted prefab
        2. Attempt to move the corrupted prefab
        3. Verify that corrupted prefab was moved successfully
        """

        env = ap_setup_fixture

        asset_folder = "C21968388"
        source_dir, _ = asset_processor.prepare_test_environment(env["tests_dir"], asset_folder)

        filename = "DependencyScannerAsset.prefab"
        file_path = os.path.join(source_dir, filename)
        dst_rel_path = os.path.join("MoveOutput", filename)
        dst_full_path = os.path.join(source_dir, dst_rel_path)

        def teardown():
            # Delete files created during test
            fs.delete([file_path], True, False)

        request.addfinalizer(teardown)

        # Ensure the file is unlocked
        fs.unlock_file(file_path)

        # Construct asset processor batch command
        extraParams = [f"--move={os.path.join(asset_folder, filename)},{os.path.join(asset_folder, dst_rel_path)}", "--confirm"]
        result, _ = asset_processor.batch_process(extra_params=extraParams)
        assert result, "AP Batch failed"
        assert os.path.exists(dst_full_path), f"{filename} was not found in MoveOutput. Expected {dst_full_path}"

    @pytest.mark.test_case_id("C21968365")
    @pytest.mark.assetpipeline
    def test_WindowsMacPlatforms_UpdateReferences_MoveCommandMessage(self, ap_setup_fixture, asset_processor):
        """
        UpdateReferences without move or delete

        Test Steps:
        1. Create temporary testing environment
        2. Attempt to move with UpdateReferences but without move or delete flags
        3. Verify that message is returned to the user that additional flags are required
        """
        env = ap_setup_fixture
        expected_message = "Command --updateReferences must be used with command --move"
        unexpected_message = "RELOCATION REPORT"

        asset_processor.prepare_test_environment(env["tests_dir"], "C21968365")

        # Build and run the AP Batch command: UpdateReferences without move or delete
        extraParams = ["--UpdateReferences", "--confirm"]
        result, ap_batch_output = asset_processor.batch_process(capture_output=True, extra_params=extraParams)

        # Verify expected and unexpected messages in log
        utils.validate_log_output(ap_batch_output, [expected_message], [unexpected_message])

    @pytest.mark.test_case_id("C21968374")
    @pytest.mark.assetpipeline
    def test_WindowsMacPlatforms_AllowBrokenDependenciesWithoutMoveDelete_RelocatorWarnsUser(self, asset_processor,
                                                                                             ap_setup_fixture):
        """
        When running the relocator command --AllowBrokenDependencies without the move or delete flags, the user should
        be warned that the flags are necessary for the functionality to be used

        Test Steps:
        1. Create temporary testing environment
        2. Attempt to move with AllowBrokenDependencies without the move or delete flag
        3. Verify that message is returned to the user that additional flags are required
        """

        env = ap_setup_fixture
        expected_message = "Command --allowBrokenDependencies must be used with command --move or --delete"
        unexpected_message = "RELOCATION REPORT"

        asset_processor.prepare_test_environment(env["tests_dir"], "C21968374")

        # AssetProcessorBatch --AllowBrokenDependencies --confirm
        extraParams = ["--AllowBrokenDependencies", "--confirm"]
        result, ap_batch_output = asset_processor.batch_process(capture_output=True, extra_params=extraParams)

        # Look for expected message inside the log and verify that no move or delete occurs in the log
        utils.validate_log_output(ap_batch_output, [expected_message], [unexpected_message])

    @pytest.mark.skipif(ly_test_tools.WINDOWS, reason="https://github.com/o3de/o3de/issues/14514")
    @pytest.mark.skipif(ly_test_tools.LINUX, reason="Python based file locking does not function on Linux")
    @pytest.mark.test_case_id("C21968355")
    @pytest.mark.test_case_id("C21968356")
    @pytest.mark.test_case_id("C21968359")
    @pytest.mark.test_case_id("C21968360")
    @pytest.mark.assetpipeline
    @pytest.mark.parametrize(
        "test_id, read_only, confirm, expect_success, expected_queries, unexpected_queries",
        # Comprehend a list of tuples for pytest parametrize while maintaining readability of arguments
        [
            (
                test["test_id"],
                test["read_only"],
                test["confirm"],
                test["expect_success"],
                test["expected_queries"],
                test["unexpected_queries"],
            )
            for test in (
                {
                    "test_id": "C21968355",
                    "read_only": False,
                    "confirm": True,
                    "expect_success": True,
                    "expected_queries": [],
                    "unexpected_queries": [],
                },
                {
                    "test_id": "C21968356",
                    "read_only": False,
                    "confirm": False,
                    "expect_success": False,
                    "expected_queries": ["Preview file delete.  Run again with --confirm to actually make changes"],
                    "unexpected_queries": [["SourceFileRelocator", "Error"]],
                },
                {
                    "test_id": "C21968359",
                    "read_only": True,
                    "confirm": True,
                    "expect_success": False,
                    "expected_queries": [
                        ["SourceFileRelocator", "Error: operation failed for file", "File is read-only."],
                        "SUCCESS COUNT: 0",
                        "FAILURE COUNT: 1",
                    ],
                    "unexpected_queries": [],
                },
                {
                    "test_id": "C21968360",
                    "read_only": False,
                    "confirm": True,
                    "expect_success": True,
                    "expected_queries": ["SUCCESS COUNT: 1", "FAILURE COUNT: 0"],
                    "unexpected_queries": [["SourceFileRelocator", "Error"]],
                },
            )
        ],
    )
    def test_WindowsMacPlatforms_DeleteWithParameters_DeleteSuccess(
        self,
        ap_setup_fixture,
        asset_processor,
        timeout,
        workspace,
        test_id,
        read_only,
        confirm,
        expect_success,
        expected_queries,
        unexpected_queries,
        project
    ):
        """
        Dynamic data test  for deleting a file with Asset Relocator:

        C21968355 Delete a file with confirm
        C21968356 Delete a file without confirm
        C21968359 Delete a file that is marked as ReadOnly
        C21968360 Delete a file that is not marked as ReadOnly

        Test Steps:
        1. Create temporary testing environment
        2. Set the read-only status of the file based on the test case
        3. Run asset relocator with --delete and the confirm status based on the test case
        4. Assert file existence or nonexistence based on the test case
        5. Validate the relocation report based on expected and unexpected messages
        """

        env = ap_setup_fixture
        test_file = "testFile.txt"

        # Copy test asset to project folder and verify the copy

        source_dir, _ = asset_processor.prepare_test_environment(env["tests_dir"], "C21968355")
        project_asset = os.path.join(source_dir, test_file)
        assert os.path.exists(project_asset), f"{project_asset} does not exist"

        # Set the read-only status of the file based on the test case
        if read_only:
            fs.lock_file(project_asset)
        else:
            fs.unlock_file(project_asset)

        # Build and run the AP Batch --delete command with parameters
        test_file_relative_path = os.path.join("C21968355", test_file)

        extraParams = [f"--delete={test_file_relative_path}", f"{'--confirm' if confirm else ''}"]
        result, ap_batch_output = asset_processor.batch_process(capture_output=True, extra_params=extraParams)

        # Assert file existence or nonexistence based on the test case
        if expect_success:
            assert not os.path.exists(project_asset), "Failed to delete the file"
        else:
            assert os.path.exists(project_asset), "File was deleted unexpectedly"

        # Validate the relocation report based on expected and unexpected messages
        if expected_queries or unexpected_queries:
            utils.validate_log_output(ap_batch_output, expected_queries, unexpected_queries)

    @pytest.mark.skip(reason="https://github.com/o3de/o3de/issues/14514")
    @pytest.mark.test_case_id("C21968381")
    @pytest.mark.test_case_id("C21968382")
    @pytest.mark.assetpipeline
    @pytest.mark.parametrize(
        "test_id, move, delete, confirm, leave_empty, folder_should_remain, expect_relocate_or_delete",
        [
            (
                p["test_id"],
                p["move"],
                p["delete"],
                p["confirm"],
                p["leave_empty"],
                p["folder_should_remain"],
                p["expect_relocate_or_delete"],
            )
            for p in (
                {
                    "test_id": "C21968381_a",
                    "move": True,
                    "delete": False,
                    "confirm": True,
                    "leave_empty": True,
                    "folder_should_remain": True,
                    "expect_relocate_or_delete": "relocate",
                },
                {
                    "test_id": "C21968381_b",
                    "move": True,
                    "delete": False,
                    "confirm": True,
                    "leave_empty": False,
                    "folder_should_remain": False,
                    "expect_relocate_or_delete": "relocate",
                },
                {
                    "test_id": "C21968381_c",
                    "move": False,
                    "delete": True,
                    "confirm": True,
                    "leave_empty": True,
                    "folder_should_remain": True,
                    "expect_relocate_or_delete": "delete",
                },
                {
                    "test_id": "C21968381_d",
                    "move": False,
                    "delete": True,
                    "confirm": True,
                    "leave_empty": False,
                    "folder_should_remain": False,
                    "expect_relocate_or_delete": "delete",
                },
                {
                    "test_id": "C21968382_a",
                    "move": True,
                    "delete": False,
                    "confirm": False,
                    "leave_empty": True,
                    "folder_should_remain": True,
                    "expect_relocate_or_delete": None,
                },
                {
                    "test_id": "C21968382_b",
                    "move": False,
                    "delete": True,
                    "confirm": False,
                    "leave_empty": True,
                    "folder_should_remain": True,
                    "expect_relocate_or_delete": None,
                },
            )
        ],
    )
    def test_WindowsMacPlatforms_RelocatorLeaveEmptyFolders_WithAndWithoutConfirm(
        self,
        asset_processor,
        workspace,
        timeout,
        ap_setup_fixture,
        test_id,
        move,
        delete,
        confirm,
        leave_empty,
        folder_should_remain,
        expect_relocate_or_delete,
        project
    ):
        """
        Test the LeaveEmptyFolders flag in various configurations

        :returns: None

        Test Steps:
        1. Create temporary testing environment
        2. Build the various move/delete commands here based on test data
        3. Run the move command with the various triggers based on test data
        4. Verify the original assets folder still exists based on test data
        5. Verify the files successfully moved to new location based on test data
        6. Verify that the files were removed from original location based on test data
        7. Verify the files have not been deleted or moved from original location based on test data
        """
        # # Start test setup # #
        env = ap_setup_fixture
        source_dir, _ = asset_processor.prepare_test_environment(env["tests_dir"], test_id)
        assets_name = "testingAssets"
        test_assets_zip = f"""{os.path.join(workspace.paths.project(), "TestAssets", assets_name)}.zip"""
        destination_path = source_dir
        destination_name = test_id
        unzipped_assets_path = os.path.join(destination_path, assets_name)

        # Extract test assets to the project folder and count files for later use
        fs.unzip(destination_path, test_assets_zip)
        original_files = utils.get_relative_file_paths(unzipped_assets_path)

        # # Build the various move/delete commands here based on test data # #
        move_dir = f"MoveOutput_{test_id}"
        move_dir_path = os.path.join(source_dir, move_dir)
        relative_assets_path = os.path.join(destination_name, assets_name, "*")
        relative_move_str = f" --move={relative_assets_path},{os.path.join(destination_name, move_dir, '*')}"
        relative_delete_str = f" --delete={relative_assets_path}"
        leave_empty_flag = " --leaveEmptyFolders"
        confirm_flag = " --confirm"

        extraParams = ["--zeroAnalysisMode"]

        if move:
            extraParams += [relative_move_str]

        if delete:
            extraParams += [relative_delete_str]

        if confirm:
            extraParams += [confirm_flag]

        if leave_empty:
            extraParams += [leave_empty_flag]


        # # Run the command # #
        asset_processor.batch_process(extra_params=extraParams)

        # # Check the results of the commands # #
        # Verify the original assets folder still exists
        if folder_should_remain:
            assert os.path.exists(unzipped_assets_path), "The directory was expected to still exist but was removed"
        else:
            assert not os.path.exists(unzipped_assets_path), "The directory was supposed to be removed but was not"

        # Verify the files successfully moved to new location
        if expect_relocate_or_delete == "relocate":
            # fmt:off
            assert os.path.exists(move_dir_path), \
                f"{move_dir} was expected to exist inside {os.path.dirname(move_dir_path)} but was not found"
            # fmt:on
            new_files_to_check = utils.get_relative_file_paths(move_dir_path)
            # fmt:off
            assert utils.compare_lists(new_files_to_check, original_files), \
                "Files did not match the original list after moving"
            # fmt:on

        # Verify that the files were removed from original location; original location may still exist or be deleted
        if expect_relocate_or_delete == "delete":
            if os.path.exists(unzipped_assets_path):
                # fmt:off
                assert os.listdir(unzipped_assets_path) == [], \
                    f"There are still assets inside {assets_name} when they should have been deleted"
                # fmt:on
            else:
                logger.info(f"{unzipped_assets_path} has been deleted as expected")

        # Verify the files have not been deleted or moved from original location
        if expect_relocate_or_delete is None:
            assert not os.path.exists(move_dir_path), "The move directory exists when it was not expected to"
            assert os.path.exists(unzipped_assets_path), "The original directory was unexpectedly deleted"

            new_files_to_check = utils.get_relative_file_paths(unzipped_assets_path)
            assert utils.compare_lists(new_files_to_check, original_files), "Files did not match the original list"

    @pytest.mark.test_case_id("C21968373")
    @pytest.mark.assetpipeline
    @pytest.mark.parametrize("test_file_name", ["ReadOnly.txt", "Writable.txt"])
    @pytest.mark.skipif(not utils.check_for_perforce(error_on_no_perforce=False), reason="Perforce not enabled")
    def test_WindowsMacPlatforms_EnableSCM_RelocatorFails(self, workspace, ap_setup_fixture, test_file_name,
                                                          asset_processor):
        """
        The test will attempt to move test assets that are not tracked under P4 source control using the EnableSCM flag
        Because the files are not tracked by source control, the relocation should fail

        Test Steps:
        1. Create temporary testing environment
        2. Set ReadOnly or Not-ReadOnly for the test files based on test data
        3. Generate and run the enableSCM command
        4. Verify the move failed and expected messages are present
        """
        # Move the test assets into the project folder
        env = ap_setup_fixture
        
        test_assets_dir = os.path.join(env["tests_dir"], "temp_work_dir_C21968373")

        # Remove previous temp dir if it exists
        if os.path.exists(test_assets_dir):
            for asset in os.listdir(test_assets_dir):
                fs.unlock_file(os.path.join(test_assets_dir, asset))

            shutil.rmtree(test_assets_dir)

        # Copy the test assets into a test folder and add that as a scan folder
        # Note that using asset_processor.prepare_test_environment for this test causes
        # the test to hang when run in pycharm due to an issue with p4.exe stdout pipe not closing
        # when p4.exe terminates.
        source_files = utils.prepare_test_assets(env["tests_dir"], "C21968373", test_assets_dir)
        asset_processor.add_scan_folder(test_assets_dir)

        # fmt:off
        assert utils.compare_lists(os.listdir(test_assets_dir), os.listdir(source_files)), \
            "There was an error copying the files from the source."
        # fmt:on
        # # Test Setup is Complete # #

        # # Set ReadOnly or Not-ReadOnly for the file # #
        file = os.path.join(test_assets_dir, test_file_name)
        if "ReadOnly" in test_file_name:
            fs.lock_file(file)
        else:
            fs.unlock_file(file)


        # # Generate and run the enableSCM command # #
        src_rel_path = os.path.join(test_assets_dir, test_file_name)
        dst_rel_path = os.path.join("MoveOutput", test_file_name)
        move_flag = f"--move={src_rel_path},{dst_rel_path}"

        extraParams = [f"{move_flag}", "--confirm", "--updateReferences", "--enableSCM"]
        result, ap_batch_output = asset_processor.batch_process(capture_output=True, extra_params=extraParams)

        # # Verify the move failed and expected messages are present # #
        # fmt:off
        assert not os.path.exists(os.path.join(workspace.paths.project(), dst_rel_path)), \
            f"Move command should have failed but {dst_rel_path} exists"
        # fmt:on

        expected_log_strings = ["Error: operation failed for file", "FAILURE COUNT: 1"]
        utils.validate_log_output(ap_batch_output, expected_log_strings, [],
                                  asset_processor_utils.print_tail_lines(ap_batch_output, 100))

    tests = [
        pytest.param(
            {
                # UpdateReferences with move and confirm - one file
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "Prefabs", "TestA.prefab"),
                "secondary": "UpdateReferences",
                "confirm": True,
                "expected_messages": ["SUCCESS COUNT: 1", "FAILURE COUNT: 0"],
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968362_1",
            marks=[pytest.mark.test_case_id("C21968362")],
        ),
        pytest.param(
            {
                # UpdateReferences with move and confirm - file group (file type provided)
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "TestA",  "*.prefab"),
                "secondary": "UpdateReferences",
                "confirm": True,
                "expected_messages": [
                    "SUCCESS COUNT: 1",
                    "FAILURE COUNT: 0",
                    "SUCCESSFULLY UPDATED: 0",
                    "FAILED TO UPDATE: 1",
                    "The following files have a product dependency on one or more of the products generated by this file"
                ],
                "unexpected_messages": [
                    ["SourceFileRelocator", "Error"],
                    ["Updated", "TestA.prefab", "SUCCESS"],
                ],
            },
            id="C21968362_2",
            marks=[pytest.mark.test_case_id("C21968362"), pytest.mark.skip(reason="Need a file type that supports sequence reference types.")],
        ),
        pytest.param(
            {
                # UpdateReferences with move and confirm - file group (partial file name provided)
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "TestB", "Test*"),
                "secondary": "UpdateReferences",
                "confirm": True,
                "expected_messages": [
                    "SUCCESS COUNT: 1",
                    "FAILURE COUNT: 0",
                    "SUCCESSFULLY UPDATED: 1",
                    "FAILED TO UPDATE: 0",
                    ["Updated", "TestB.prefab", "SUCCESS"],
                ],
                "unexpected_messages": [
                    ["SourceFileRelocator", "Error"],
                    "The following files have a product dependency on one or more of the products generated by this file and will break:",
                ],
            },
            id="C21968362_3",
            marks=[pytest.mark.test_case_id("C21968362")],
        ),
        pytest.param(
            {
                # UpdateReferences with move and confirm - all files ("*")
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "*"),
                "secondary": "UpdateReferences",
                "confirm": True,
                "expected_messages": [
                    "SUCCESS COUNT: 2",
                    "FAILURE COUNT: 0",
                    "SUCCESSFULLY UPDATED: 2",
                    "FAILED TO UPDATE: 2",
                    ["Updated", "TestB.prefab", "SUCCESS"],
                    "The following files have a product dependency on one or more of the products generated by this file",
                ],
                "unexpected_messages": [
                    ["SourceFileRelocator", "Error"],
                    ["Updated", "TestA.prefab", "SUCCESS"],
                ],
            },
            id="C21968362_4",
            marks=[pytest.mark.test_case_id("C21968362"), pytest.mark.skip(reason="Need a file type that supports sequence reference types.")],
        ),
        pytest.param(
            {
                # UpdateReferences with move and without confirm - one file
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "Prefabs", "TestA.prefab"),
                "secondary": "UpdateReferences",
                "confirm": False,
                "expected_messages": "Preview file move.  Run again with --confirm to actually make changes",
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968363_1",
            marks=[pytest.mark.test_case_id("C21968363")],
        ),
        pytest.param(
            {
                # UpdateReferences with move and without confirm - file group (file type provided)
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "TestA", "*.prefab"),
                "secondary": "UpdateReferences",
                "confirm": False,
                "expected_messages": [
                    "Preview file move.  Run again with --confirm to actually make changes",
                    "The following files have a source / job dependency on this file, we will attempt to fix the references but they may still break:",
                ],
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968363_2",
            marks=[pytest.mark.test_case_id("C21968363")],
        ),
        pytest.param(
            {
                # UpdateReferences with move and without confirm - file group (partial file name provided)
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "Test*"),
                "secondary": "UpdateReferences",
                "confirm": False,
                "expected_messages": [
                    "Preview file move.  Run again with --confirm to actually make changes",
                    "The following files have a source / job dependency on this file, we will attempt to fix the references but they may still break:",
                    ],
                "unexpected_messages": [
                    ["SourceFileRelocator", "Error"]
                ],
            },
            id="C21968363_3",
            marks=[pytest.mark.test_case_id("C21968363")],
        ),
        pytest.param(
            {
                # UpdateReferences with move and without confirm - all files ("*")
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "*"),
                "secondary": "UpdateReferences",
                "confirm": False,
                "expected_messages": [
                    "Preview file move.  Run again with --confirm to actually make changes",
                    "The following files have a source / job dependency on this file, we will attempt to fix the references but they may still break:",
                ],
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968363_4",
            marks=[pytest.mark.test_case_id("C21968363")],
        ),
        pytest.param(
            {
                # UpdateReferences with delete - one file
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "Prefabs", "TestA.prefab"),
                "secondary": "UpdateReferences",
                "confirm": False,
                "expected_messages": "Command --updateReferences must be used with command --move",
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968366_1",
            marks=[pytest.mark.test_case_id("C21968366")],
        ),
        pytest.param(
            {
                # UpdateReferences with delete - all files ("*")
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "Prefabs", "*"),
                "secondary": "UpdateReferences",
                "confirm": False,
                "expected_messages": "Command --updateReferences must be used with command --move",
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968366_2",
            marks=[pytest.mark.test_case_id("C21968366")],
        ),
        pytest.param(
            {
                # UpdateReferences with delete - one file
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "Prefabs", "TestA.prefab"),
                "secondary": "UpdateReferences",
                "confirm": True,
                "expected_messages": "Command --updateReferences must be used with command --move",
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968366_3",
            marks=[pytest.mark.test_case_id("C21968366")],
        ),
        pytest.param(
            {
                # UpdateReferences with delete - file group (file type provided)
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "Prefabs", "*.prefab"),
                "secondary": "UpdateReferences",
                "confirm": True,
                "expected_messages": "Command --updateReferences must be used with command --move",
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968366_4",
            marks=[pytest.mark.test_case_id("C21968366")],
        ),
        pytest.param(
            {
                # UpdateReferences with delete - file group (partial file name provided)
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "Prefabs", "Test*"),
                "secondary": "UpdateReferences",
                "confirm": True,
                "expected_messages": "Command --updateReferences must be used with command --move",
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968366_5",
            marks=[pytest.mark.test_case_id("C21968366")],
        ),
        pytest.param(
            {
                # UpdateReferences with delete - all files ("*")
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "Prefabs", "*"),
                "secondary": "UpdateReferences",
                "confirm": True,
                "expected_messages": "Command --updateReferences must be used with command --move",
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968366_6",
            marks=[pytest.mark.test_case_id("C21968366")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with move and confirm - one file
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "TestA", "TestAChild.prefab"),
                "secondary": "AllowBrokenDependencies",
                "confirm": True,
                "expected_messages": ["SUCCESS COUNT: 1", "FAILURE COUNT: 0"],
                "unexpected_messages": [
                    ["SourceFileRelocator", "Error"],
                    "The following files have a source / job dependency on this file, we will attempt to fix the references but they may still break:",
                ],
            },
            id="C21968370_1",
            marks=[pytest.mark.test_case_id("C21968370")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with move and confirm - file group (file type provided)
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "TestA", "*.prefab"),
                "secondary": "AllowBrokenDependencies",
                "confirm": True,
                "expected_messages": [
                    "SUCCESS COUNT: 1",
                    "FAILURE COUNT: 0",
                    "The following files have a source / job dependency on this file and will break:",
                ],
                "unexpected_messages": [
                    ["SourceFileRelocator", "Error"],
                    ["Updated", "TestA.prefab", "SUCCESS"],
                ],
            },
            id="C21968370_2",
            marks=[pytest.mark.test_case_id("C21968370")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with move and confirm - file group (partial file name provided)
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "Test*"),
                "secondary": "AllowBrokenDependencies",
                "confirm": True,
                "expected_messages": [
                    "SUCCESS COUNT: 2",
                    "FAILURE COUNT: 0",
                    "The following files have a source / job dependency on this file and will break:",
                ],
                "unexpected_messages": [["SourceFileRelocator", "Error"], ["Updated", "TestB.prefab", "SUCCESS"]],
            },
            id="C21968370_3",
            marks=[pytest.mark.test_case_id("C21968370")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with move and confirm - all files ("*")
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "*"),
                "secondary": "AllowBrokenDependencies",
                "confirm": True,
                "expected_messages": [
                    "SUCCESS COUNT: 2",
                    "FAILURE COUNT: 0",
                    "The following files have a source / job dependency on this file and will break:",
                ],
                "unexpected_messages": [
                    ["SourceFileRelocator", "Error"],
                    ["Updated", "TestA.prefab", "SUCCESS"],
                    ["Updated", "TestB.prefab", "SUCCESS"],
                ],
            },
            id="C21968370_4",
            marks=[pytest.mark.test_case_id("C21968370")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with move and without confirm - one file
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "TestA", "TestAChild.prefab"),
                "secondary": "AllowBrokenDependencies",
                "confirm": False,
                "expected_messages": "Preview file move.  Run again with --confirm to actually make changes",
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968371_1",
            marks=[pytest.mark.test_case_id("C21968371")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with move and without confirm - file group (file type provided)
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "TestA", "*.prefab"),
                "secondary": "AllowBrokenDependencies",
                "confirm": False,
                "expected_messages": [
                    "Preview file move.  Run again with --confirm to actually make changes",
                    "The following files have a source / job dependency on this file and will break:",
                ],
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968371_2",
            marks=[pytest.mark.test_case_id("C21968371")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with move and without confirm - file group (partial file name provided)
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "TestB", "Test*"),
                "secondary": "AllowBrokenDependencies",
                "confirm": False,
                "expected_messages": [
                    "Preview file move.  Run again with --confirm to actually make changes",
                    "The following files have a source / job dependency on this file and will break:",
                ],
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968371_3",
            marks=[pytest.mark.test_case_id("C21968371")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with move and without confirm - all files ("*")
                "primary": "move",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "*"),
                "secondary": "AllowBrokenDependencies",
                "confirm": False,
                "expected_messages": [
                    "Preview file move.  Run again with --confirm to actually make changes",
                    "The following files have a source / job dependency on this file and will break:",
                ],
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968371_4",
            marks=[pytest.mark.test_case_id("C21968371")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with delete - one file
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "Prefabs", "TestA.prefab"),
                "secondary": "AllowBrokenDependencies",
                "confirm": False,
                "expected_messages": "Preview file delete.  Run again with --confirm to actually make changes",
                "unexpected_messages": [
                    ["SourceFileRelocator", "Error"],
                    "The following files have a source / job dependency on this file and will break:",
                ],
            },
            id="C21968375_1",
            marks=[pytest.mark.test_case_id("C21968375")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with delete - all files ("*")
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "*"),
                "secondary": "AllowBrokenDependencies",
                "confirm": False,
                "expected_messages": [
                    "Preview file delete.  Run again with --confirm to actually make changes",
                    "The following files have a source / job dependency on this file and will break:",
                ],
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968375_2",
            marks=[pytest.mark.test_case_id("C21968375")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with delete - one file
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "Prefabs", "TestA.prefab"),
                "secondary": "AllowBrokenDependencies",
                "confirm": True,
                "expected_messages": ["SUCCESS COUNT: 1", "FAILURE COUNT: 0"],
                "unexpected_messages": [
                    ["SourceFileRelocator", "Error"],
                    "The following files have a source / job dependency on this file and will break:",
                ],
            },
            id="C21968375_3",
            marks=[pytest.mark.test_case_id("C21968375")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with delete - file group (file type provided)
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "TestB", "*.prefab"),
                "secondary": "AllowBrokenDependencies",
                "confirm": True,
                "expected_messages": [
                    "SUCCESS COUNT: 1",
                    "FAILURE COUNT: 0",
                    "The following files have a source / job dependency on this file and will break:",
                ],
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968375_4",
            marks=[pytest.mark.test_case_id("C21968375")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with delete - file group (partial file name provided)
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "Test*"),
                "secondary": "AllowBrokenDependencies",
                "confirm": True,
                "expected_messages": [
                    "SUCCESS COUNT: 2",
                    "FAILURE COUNT: 0",
                    "The following files have a source / job dependency on this file and will break:",
                ],
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968375_5",
            marks=[pytest.mark.test_case_id("C21968375")],
        ),
        pytest.param(
            {
                # AllowBrokenDependencies with delete - all files ("*")
                "primary": "delete",
                "src_rel_path": os.path.join("testingAssets", "OtherPrefabs", "*"),
                "secondary": "AllowBrokenDependencies",
                "confirm": True,
                "expected_messages": [
                    "SUCCESS COUNT: 2",
                    "FAILURE COUNT: 0",
                    "The following files have a source / job dependency on this file and will break:",
                ],
                "unexpected_messages": [["SourceFileRelocator", "Error"]],
            },
            id="C21968375_6",
            marks=[pytest.mark.test_case_id("C21968375")],
        ),
    ]

    @pytest.mark.parametrize("test", tests)
    def test_WindowsAndMac_UpdateReferencesAllowBrokenDependencies_PathExistenceAndMessage(self, ap_setup_fixture,
                                                                                           asset_processor, test):
        """
        C21968362 UpdateReferences with move and confirm
        C21968363 UpdateReferences with move and without confirm
        C21968366 UpdateReferences with delete
        C21968370 AllowBrokenDependencies with move and confirm
        C21968371 AllowBrokenDependencies with move and without confirm
        C21968375 AllowBrokenDependencies with delete

        Test Steps:
        1. Create temporary testing environment
        2. Run Asset Processor to Process Assets
        3. Build primary AP Batch parameter value and destination paths
        4. Validate resulting file paths in source and output directories
        5. Validate the log based on expected and unexpected messages
        """
        env = ap_setup_fixture
        all_test_asset_rel_paths = [
            "iwillstay.txt",
            "testFile - Copy (2).txt",
            "testFile - Copy (3).txt",
            "testFile - Copy (4).txt",
            "testFile - Copy (5).txt",
            "testFile - Copy.txt",
            "testFile.txt",
            os.path.join("OtherPrefabs", "TestB", "TestBChild.prefab"),
            os.path.join("OtherPrefabs", "TestA", "TestAChild.prefab"),
            os.path.join("Prefabs", "TestA.prefab"),
            os.path.join("Prefabs", "TestB.prefab"),
        ]

        asset_processor.create_temp_asset_root(False)
        source_folder, _ = asset_processor.prepare_test_environment(env["tests_dir"], "assets_with_dependencies", add_scan_folder=False, use_current_root=True)
        asset_processor.add_scan_folder(source_folder)

        # Extract testingAssets.zip to the project folder
        assets_src = os.path.join(env["tests_dir"], "assets", "assets_with_dependencies", "testingAssets.zip")
        fs.unzip(source_folder, assets_src)

        # Validate and process assets
        testingAssets_dir = os.path.join(source_folder, "testingAssets")
        assert utils.compare_lists(utils.get_relative_file_paths(testingAssets_dir), all_test_asset_rel_paths)

        # Build and verify source paths
        test_assets_initial = utils.get_relative_file_paths(testingAssets_dir)
        src_path_root = os.path.join(source_folder, os.path.split(test["src_rel_path"])[0:-1][0])
        src_path_end = os.path.split(test["src_rel_path"])[-1]

        if "*" in src_path_end:  # Handle path with wildcard
            src_full_paths_targeted = utils.get_paths_from_wildcard(src_path_root, src_path_end)
        else:  # Handle path without wildcard
            src_full_paths_targeted = [os.path.join(src_path_root, src_path_end)]
        src_rel_paths_targeted = [os.path.relpath(item, testingAssets_dir) for item in src_full_paths_targeted]
        lowercase_targets = [item.lower() for item in src_rel_paths_targeted]
        src_assets_untargeted = [item for item in test_assets_initial if item.lower() not in lowercase_targets]
        assert len(src_assets_untargeted) + len(src_rel_paths_targeted) == len(test_assets_initial), "Targetting failed"

        for item in src_full_paths_targeted:
            assert os.path.exists(item), f"Source file not found: {item}"

        # Build primary AP Batch parameter value and destination paths
        param_val = test["src_rel_path"]
        if test["primary"] == "move":
            output_assets_targeted = [os.path.split(item)[-1] for item in src_full_paths_targeted]
            dst_rel_path = os.path.join("MoveOutput", src_path_end)
            dst_path_root = os.path.join(source_folder, "MoveOutput")
            param_val = f"{test['src_rel_path']},{dst_rel_path}"

        # Build and run the AP Batch command with parameters
        extraParams = [f"--{test['primary']}={param_val}", f"--{test['secondary']}"]
        if test['confirm']:
            extraParams += ["--confirm"]

        result, ap_batch_output = asset_processor.batch_process(capture_output=True, extra_params=extraParams)

        # Validate resulting file paths in source and output directories
        src_assets_final = utils.get_relative_file_paths(testingAssets_dir)
        src_unchanged = utils.compare_lists(src_assets_final, test_assets_initial)
        untargeted_assets_remain = utils.compare_lists(src_assets_final, src_assets_untargeted)

        if test["primary"] == "move":
            output_assets_actual = [os.path.split(item)[-1] for item in utils.get_relative_file_paths(dst_path_root)]
            if test["confirm"]:  # Handle move with confirm
                targeted_assets_moved = utils.compare_lists(output_assets_actual, output_assets_targeted)
                # fmt:off
                assert (untargeted_assets_remain and targeted_assets_moved), \
                    f"Move with confirm unexpectedly failed on {test['src_rel_path']}"
                # fmt:on
            else:  # Handle move without confirm
                dst_empty = output_assets_actual == []
                assert src_unchanged and dst_empty, f"Move without confirm unexpectedly handled {test['src_rel_path']}"
        elif test["primary"] == "delete":
            if test["secondary"] == "AllowBrokenDependencies" and test["confirm"]:  # Expect deletion
                assert untargeted_assets_remain, f"Delete with confirm unexpectedly failed on {test['src_rel_path']}"
            else:  # Do not expect deletion
                assert src_unchanged, f"Delete unexpectedly handled {test['src_rel_path']}"

        # Validate the log based on expected and unexpected messages
        if test["expected_messages"] or test["unexpected_messages"]:
            utils.validate_log_output(
                ap_batch_output, test["expected_messages"], test["unexpected_messages"]
            )

    tests = [
                pytest.param(
                    {
                        "primary": "move",
                        "src_rel_path": os.path.join("TestBChild.prefab"),
                        "excludeMetaDataFiles": False,
                        "confirm": True,
                        "expected_messages": [
                            "SUCCESS COUNT: 1",
                            "FAILURE COUNT: 0"],
                        "unexpected_messages": [["SourceFileRelocator", "Error"]],
                    },
                    id="C30936451_1",
                    marks=[pytest.mark.test_case_id("C30936451")],
                ),
                pytest.param(
                    {
                        "primary": "move",
                        "src_rel_path": os.path.join("*.prefab"),
                        "excludeMetaDataFiles": False,
                        "confirm": True,
                        "expected_messages": [
                            "SUCCESS COUNT: 2",
                            "FAILURE COUNT: 0"],
                        "unexpected_messages": [["SourceFileRelocator", "Error"]],
                    },
                    id="C30936451_2",
                    marks=[pytest.mark.test_case_id("C30936451")],
                ),
                pytest.param(
                    {
                        "primary": "move",
                        "src_rel_path": os.path.join("TestBChild.prefab"),
                        "excludeMetaDataFiles": True,
                        "confirm": True,
                        "expected_messages": [
                            "SUCCESS COUNT: 1",
                            "FAILURE COUNT: 0",
                        ],
                        "unexpected_messages": [["SourceFileRelocator", "Error"]],
                    },
                    id="C30936451_3",
                    marks=[pytest.mark.test_case_id("C30936451")],
                ),
                pytest.param(
                    {
                        "primary": "move",
                        "src_rel_path": os.path.join("*.prefab"),
                        "excludeMetaDataFiles": True,
                        "confirm": True,
                        "expected_messages": [
                            "SUCCESS COUNT: 2",
                            "FAILURE COUNT: 0",
                        ],
                        "unexpected_messages": [["SourceFileRelocator", "Error"]],
                    },
                    id="C30936451_4",
                    marks=[pytest.mark.test_case_id("C30936451")],
                ),
                pytest.param(
                    {
                        "primary": "delete",
                        "src_rel_path": os.path.join("TestBChild.prefab"),
                        "excludeMetaDataFiles": False,
                        "confirm": True,
                        "expected_messages": [
                            "SUCCESS COUNT: 1",
                            "FAILURE COUNT: 0"],
                        "unexpected_messages": [["SourceFileRelocator", "Error"]],
                    },
                    id="C30936451_5",
                    marks=[pytest.mark.test_case_id("C30936451")],
                ),
                pytest.param(
                    {
                        "primary": "delete",
                        "src_rel_path": os.path.join("*.prefab"),
                        "excludeMetaDataFiles": False,
                        "confirm": True,
                        "expected_messages": [
                            "SUCCESS COUNT: 2",
                            "FAILURE COUNT: 0"],
                        "unexpected_messages": [["SourceFileRelocator", "Error"]],
                    },
                    id="C30936451_6",
                    marks=[pytest.mark.test_case_id("C30936451")],
                ),
                pytest.param(
                    {
                        "primary": "delete",
                        "src_rel_path": os.path.join("TestBChild.prefab"),
                        "excludeMetaDataFiles": True,
                        "confirm": True,
                        "expected_messages": [
                            "SUCCESS COUNT: 1",
                            "FAILURE COUNT: 0",
                        ],
                        "unexpected_messages": [["SourceFileRelocator", "Error"]],
                    },
                    id="C30936451_7",
                    marks=[pytest.mark.test_case_id("C30936451")],
                ),
                pytest.param(
                    {
                        "primary": "delete",
                        "src_rel_path": os.path.join("*.prefab"),
                        "excludeMetaDataFiles": True,
                        "confirm": True,
                        "expected_messages": [
                            "SUCCESS COUNT: 2",
                            "FAILURE COUNT: 0",
                        ],
                        "unexpected_messages": [["SourceFileRelocator", "Error"]],
                    },
                    id="C30936451_8",
                    marks=[pytest.mark.test_case_id("C30936451")],
                ),
    ]

    @pytest.mark.parametrize("test", tests)
    def test_WindowsAndMac_MoveMetadataFiles_PathExistenceAndMessage(self, workspace, request, ap_setup_fixture,
                                                                     asset_processor, test):
        """
        Tests whether moving metadata files can be moved

        Test Steps:
        1. Create temporary testing environment
        2. Determine if using wildcards on paths or not
        3. Determine if excludeMetaDataFiles is set or not
        4. Build primary AP Batch parameter value and destination paths
        5. Build and run the AP Batch command with parameters
        6. Validate resulting file paths in source and output directories
        7. Validate the log based on expected and unexpected messages
        """
        env = ap_setup_fixture

        def teardown():
            # Delete all files created/moved during test
            source_path_root = os.path.join(env["project_dir"], "TestMetaDataAssets")
            for file in utils.get_paths_from_wildcard(source_path_root, "*"):
                fs.delete(file, True, False)

            destination_path_root = os.path.join(env["project_dir"], "MoveOutput")
            for file in utils.get_paths_from_wildcard(destination_path_root, "*"):
                fs.delete(file, True, False)

        request.addfinalizer(teardown)

        asset_processor.create_temp_asset_root(False)
        source_folder, _ = asset_processor.prepare_test_environment(env["tests_dir"], "C30936451", use_current_root=True,
                                                                    add_scan_folder=False)
        asset_processor.add_scan_folder(source_folder)

        src_rel_paths_initial = ["TestAChild.prefab", "TestBChild.prefab", "TestBChild.prefab.exportsettings"]

        src_path_root = os.path.join(source_folder, os.path.split(test["src_rel_path"])[0:-1][0])
        src_path_end = os.path.split(test["src_rel_path"])[-1]
        if "*" in src_path_end:  # Handle path with wildcard
            src_full_paths_targeted = utils.get_paths_from_wildcard(src_path_root, src_path_end)
        else:  # Handle path without wildcard
            src_full_paths_targeted = [os.path.join(src_path_root, src_path_end)]
        src_rel_paths_targeted = [os.path.relpath(item, source_folder) for item in src_full_paths_targeted]

        if not test["excludeMetaDataFiles"]:
            # if we are here, we need to add metadata files as well
            for src_full_path in src_full_paths_targeted:
                imageMetadataFile = src_full_path + ".exportsettings"
                if os.path.exists(imageMetadataFile):
                    src_rel_paths_targeted.append(os.path.relpath(imageMetadataFile, source_folder))
        src_rel_paths_targeted_lowercase = [item.lower() for item in src_rel_paths_targeted]
        src_assets_untargeted = [item for item in src_rel_paths_initial if item.lower() not in src_rel_paths_targeted_lowercase]

        assert len(src_assets_untargeted) + len(src_rel_paths_targeted) == len(src_rel_paths_initial), "Targetting failed"

        for item in src_full_paths_targeted:
            assert os.path.exists(item), f"Source file not found: {item}"

        # Build primary AP Batch parameter value and destination paths
        param_val = test["src_rel_path"]
        if test["primary"] == "move":
            output_assets_targeted = [os.path.split(item)[-1] for item in src_full_paths_targeted]
            dst_rel_path = os.path.join("MoveOutput", src_path_end)
            dst_path_root = os.path.join(source_folder, "MoveOutput")
            param_val = f"{test['src_rel_path']},{dst_rel_path}"

        # Build and run the AP Batch command with parameters
        extraParams = [f"--{test['primary']}={param_val}"]
        if test["excludeMetaDataFiles"]:
            extraParams = extraParams + ['--excludeMetadatafiles']

        if test["confirm"]:
            extraParams = extraParams + ['--confirm']

        result, ap_batch_output = asset_processor.batch_process(capture_output=True, extra_params=extraParams)

        # Validate resulting file paths in source and output directories
        if test["primary"] =="move":
            src_assets_unchanged = utils.get_relative_file_paths(source_folder)
            src_unchanged = utils.compare_lists(src_assets_unchanged, src_rel_paths_initial)
            assert src_assets_unchanged.sort() == src_assets_untargeted.sort(), f"Asset relocator moved an unexpected asset"

            src_assets_changed = utils.get_relative_file_paths(dst_path_root)

            assert src_assets_changed.sort() == src_rel_paths_targeted.sort(), f"Asset relocator did not move an expected asset"
        elif test["primary"] == "delete":
            if test["confirm"]:  # Expect deletion
                for item in src_full_paths_targeted:
                    assert not os.path.exists(item), f"Source file not deleted: {item}"
            else:  # Do not expect deletion
                for item in src_full_paths_targeted:
                    assert os.path.exists(item), f"Source file deleted: {item}"

        # Validate the log based on expected and unexpected messages
        if test["expected_messages"] or test["unexpected_messages"]:
            utils.validate_log_output(
                ap_batch_output, test["expected_messages"], test["unexpected_messages"]
            )



@dataclass
class MoveTest:
    description: str  # test case title
    asset_folder: str  # which folder in ./assets will be used for this test
    encoded_command: str  # the command to execute
    encoded_output_dir: str  # the destination directory to validate
    move_successful: bool = True
    name_change_map: dict = None
    files_that_stay: List[str] = field(default_factory=lambda: [])
    output_messages: List[str] = field(default_factory=lambda: [])
    step: str = None  # the step of the test from test repository
    prefix_commands: List[str] = field(default_factory=lambda: ["AssetProcessorBatch", "--zeroAnalysisMode"])
    suffix_commands: List[str] = field(default_factory=lambda: ["--confirm"])
    env: dict = field(init=False, default=None)  # inject the ap_setup_fixture at runtime
    test_files: List[str] = field(init=False, default_factory=lambda: [])
    files_that_move: List[str] = field(init=False, default_factory=lambda: [])
    expected_filepaths: List[str] = field(init=False, default_factory=lambda: [])
    unexpected_filepaths: List[str] = field(init=False, default_factory=lambda: [])
    output_to_check: List[str] = field(init=False, default_factory=lambda: [])

    def command(self) -> List[str]:
        decoded_command = self._decode(self.encoded_command)
        if ("," not in decoded_command) or (decoded_command[-1] == ",") or (decoded_command[0] == ","):
            # C21968353 and C21968354 (negative tests)
            move_command = "--move=" + self._normalize_move_target(decoded_command)
        else:
            # correctly formed move command
            self.source = self._normalize_move_target(decoded_command.split(",")[0])
            self.destination = self._normalize_move_target(decoded_command.split(",")[1])
            move_command = "--move=" + self.source + "," + self.destination
        self.prefix_commands[0] = os.path.join(os.fspath(pathlib.Path(self.env["bin_dir"])), self.prefix_commands[0])
        command = [move_command, *self.suffix_commands]
        logger.info(f"The command is:\n{command}")
        return command

    def _decode(self, target) -> str:
        """
        command and output directory test data will be encoded using the keys
        from the ap_setup_fixture. We resolve them here into real paths.
        this works because the keys are unique AND have no overlapping matches
        """
        for key in self.env:
            if key in target:
                target = target.replace(key, self.env[key])
        logger.info(f"the decoded string is: {target}")
        return target

    def _normalize_move_target(self, target):
        if target[-1] == "\\":
            return os.fspath(pathlib.Path(target)) + os.sep
        else:
            return os.fspath(pathlib.Path(target))

    def map_env(self, env: dict, source_dir):
        """
        intake the ap_setup_fixture and map the runtime attributes from the test data
        """
        split_source = source_dir.rsplit("\\", 1)
        base_a = split_source[0]
        base_b = split_source[1]
        self.env = env
        self.env["project_test_assets_dir"] = source_dir
        self.env["project_dir"] = base_a
        self.test_files = os.listdir(os.path.join(pathlib.Path(self.env["tests_dir"]), "assets", self.asset_folder))
        self.test_file_paths = [
            os.path.join(source_dir, file) for file in self.test_files
        ]
        if not self.move_successful:
            self.files_that_stay = self.test_files
        if self.files_that_stay:
            # a set is safe here because there cannot be identically named files in directory
            self.files_that_move = set(self.test_files).difference(set(self.files_that_stay))
        else:
            self.files_that_move = self.test_files
        logger.info(f"files that will move {self.files_that_move}")
        logger.info(f"files that will stay {self.files_that_stay}")
        origin_dir = source_dir
        output_dir = (self._decode(self.encoded_output_dir))
        for file in self.files_that_move:
            destination_folder = self._decode(self.encoded_output_dir).split(base_a)[1].strip("\\").replace("\\", "/")
            if self.name_change_map:
                if file in self.name_change_map:
                    self.output_to_check.append(
                        f"CURRENT PATH: {base_b}/{file}, NEW PATH: {destination_folder}/{self.name_change_map[file]}"
                    )
                    self.expected_filepaths.append(os.path.join(output_dir, self.name_change_map[file]))
            else:
                self.output_to_check.append(
                    f"CURRENT PATH: {base_b}/{file}, NEW PATH: {destination_folder}/{file}"
                )
                self.expected_filepaths.append(os.path.join(output_dir, file))
            self.unexpected_filepaths.append(os.path.join(origin_dir, file))
        if self.move_successful:
            self.output_to_check.append(f"SUCCESS COUNT: {len(self.files_that_move)}")
            self.output_to_check.append("FAILURE COUNT: 0")
        if self.files_that_stay:
            for file in self.files_that_stay:
                self.expected_filepaths.append(os.path.join(origin_dir, file))
                self.unexpected_filepaths.append(os.path.join(output_dir, file))
        logger.info(f"expected output lines are:\n{self.output_to_check}")
        logger.info(f"expected filepaths are:\n{self.expected_filepaths}")
        logger.info(f"unexpected filepaths are:\n{self.unexpected_filepaths}")


move_a_file_tests = [
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file; destination = absolute-file (file name provided)",
            asset_folder="one_txt_file",
            encoded_command="project_test_assets_dir\\testFile.txt,project_test_assets_dir\\MoveOutput\\ferg.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            name_change_map={"testFile.txt": "ferg.txt"},
        ),
        id="C19462747",
        marks=pytest.mark.test_case_id("C19462747"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file; destination = relative-file (File Name Provided)",
            asset_folder="one_txt_file",
            encoded_command="project_test_assets_dir\\testFile.txt,one_txt_file\\MoveOutput\\ferg.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            name_change_map={"testFile.txt": "ferg.txt"},
        ),
        id="C19462749",
        marks=pytest.mark.test_case_id("C19462749"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file; destination = absolute-folder",
            asset_folder="one_txt_file",
            encoded_command="project_test_assets_dir\\testFile.txt,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
        ),
        id="C19462751",
        marks=pytest.mark.test_case_id("C19462751"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file; destination = relative-folder",
            asset_folder="one_txt_file",
            encoded_command="project_test_assets_dir\\testFile.txt,one_txt_file\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
        ),
        id="C19462752",
        marks=pytest.mark.test_case_id("C19462752"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file; destination = absolute (path outside scan folder)",
            asset_folder="one_txt_file",
            encoded_command="project_test_assets_dir\\testFile.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462753",
        marks=pytest.mark.test_case_id("C19462753"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,project_test_assets_dir\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="partial filename",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462755_1",
        marks=pytest.mark.test_case_id("C19462755"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*.txt,project_test_assets_dir\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="no filename",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462755_2",
        marks=pytest.mark.test_case_id("C19462755"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_fi*\\testFile.txt,project_test_assets_dir\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="wildcard on a folder",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462755_3",
        marks=pytest.mark.test_case_id("C19462755"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="partial filename",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462757_1",
        marks=pytest.mark.test_case_id("C19462757"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*.txt,\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="no filename",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462757_2",
        marks=pytest.mark.test_case_id("C19462757"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_fi*\\testFile.txt,\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="wildcard on a folder",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462757_3",
        marks=pytest.mark.test_case_id("C19462757"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            step="partial file name",
        ),
        id="C19462759_1",
        marks=pytest.mark.test_case_id("C19462759"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*.txt,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="no file name",
        ),
        id="C19462759_2",
        marks=pytest.mark.test_case_id("C19462759"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_fi*\\testFile.txt,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="wildcard on folder",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462759_3",
        marks=pytest.mark.test_case_id("C19462759"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            step="partial file name",
        ),
        id="C19462760_1",
        marks=[pytest.mark.test_case_id("C19462760")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*.txt,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="no file name",
        ),
        id="C19462760_2",
        marks=[pytest.mark.test_case_id("C19462760")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_fi*\\testFile.txt,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="wildcard on folder",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462760_3",
        marks=pytest.mark.test_case_id("C19462760"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="partial file name",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462761_1",
        marks=pytest.mark.test_case_id("C19462761"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="no filename",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462761_2",
        marks=pytest.mark.test_case_id("C19462761"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_fi*\\testFile.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="wildcard on folder",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462761_3",
        marks=pytest.mark.test_case_id("C19462761"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_fi*\\testFile.txt,project_test_assets_dir\\MoveOutput*\\testFile.txt",
            files_that_stay=[
                "imightstay.txt",
                "testFile - Copy.txt",
                "testFile - Copy (2).txt",
                "testFile - Copy (3).txt",
                "testFile - Copy (4).txt",
            ],
            encoded_output_dir="project_test_assets_dir\\MoveOutputles\\",
            step="wildcard on folder",
        ),
        id="C19462763_1",
        marks=[pytest.mark.test_case_id("C19462763")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,project_test_assets_dir\\MoveOutput\\outputTest*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            name_change_map={
                "testFile.txt": "outputTestFile.txt",
                "testFile - Copy.txt": "outputTestFile - Copy.txt",
                "testFile - Copy (2).txt": "outputTestFile - Copy (2).txt",
                "testFile - Copy (3).txt": "outputTestFile - Copy (3).txt",
                "testFile - Copy (4).txt": "outputTestFile - Copy (4).txt",
            },
            step="partial filename",
        ),
        id="C19462763_2",
        marks=[pytest.mark.test_case_id("C19462763")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*.txt,project_test_assets_dir\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="no filename",
        ),
        id="C19462763_3",
        marks=[pytest.mark.test_case_id("C19462763")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_fi*\\testFile.txt,wildcard_files\\MoveOutput*\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutputles\\",
            files_that_stay=[
                "imightstay.txt",
                "testFile - Copy.txt",
                "testFile - Copy (2).txt",
                "testFile - Copy (3).txt",
                "testFile - Copy (4).txt",
            ],
            step="wildcard on folder",
        ),
        id="C19462765_1",
        marks=[pytest.mark.test_case_id("C19462765")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,wildcard_files\\MoveOutput\\outputTest*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            name_change_map={
                "testFile.txt": "outputTestFile.txt",
                "testFile - Copy.txt": "outputTestFile - Copy.txt",
                "testFile - Copy (2).txt": "outputTestFile - Copy (2).txt",
                "testFile - Copy (3).txt": "outputTestFile - Copy (3).txt",
                "testFile - Copy (4).txt": "outputTestFile - Copy (4).txt",
            },
            step="partial filename",
        ),
        id="C19462765_2",
        marks=[pytest.mark.test_case_id("C19462765")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*.txt,wildcard_files\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="no filename",
        ),
        id="C19462765_3",
        marks=[pytest.mark.test_case_id("C19462765")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,project_test_assets_dir\\Move*\\",
            encoded_output_dir="project_test_assets_dir\\MoveFile\\",
            move_successful=False,
            step="Move with Wildcard on File to a wildcard before the folder declaration",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462767_1",
        marks=[pytest.mark.test_case_id("C19462767")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_fi*\\testFile.txt,project_test_assets_dir\\MoveOutput*\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutputles\\",
            files_that_stay=[
                "imightstay.txt",
                "testFile - Copy.txt",
                "testFile - Copy (2).txt",
                "testFile - Copy (3).txt",
                "testFile - Copy (4).txt",
            ],
            step="Move with wildcard on folder before filename to a folder with a wildcard before the folder declaration",
        ),
        id="C19462767_2",
        marks=pytest.mark.test_case_id("C19462767"),
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,project_test_assets_dir\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            name_change_map={
                "testFile.txt": "File",
                "testFile - Copy.txt": "File - Copy",
                "testFile - Copy (2).txt": "File - Copy (2)",
                "testFile - Copy (3).txt": "File - Copy (3)",
                "testFile - Copy (4).txt": "File - Copy (4)",
            },
            step="Move with Wildcard in Filename to a folder wildcard after the folder declaration",
        ),
        id="C19462767_3",
        marks=[pytest.mark.test_case_id("C19462767")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*.txt,project_test_assets_dir\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="Move with Wildcard for the Filename to a folder wildcard after the folder declaration",
        ),
        id="C19462767_4",
        marks=[pytest.mark.test_case_id("C19462767")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,wildcard_files\\Move*\\",
            encoded_output_dir="project_test_assets_dir\\MoveFile\\",
            move_successful=False,
            step="Move with Wildcard on File to a wildcard before the folder declaration",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462768_1",
        marks=[pytest.mark.test_case_id("C19462768")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_fi*\\testFile.txt,wildcard_files\\MoveOutput*\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutputles\\",
            files_that_stay=[
                "imightstay.txt",
                "testFile - Copy.txt",
                "testFile - Copy (2).txt",
                "testFile - Copy (3).txt",
                "testFile - Copy (4).txt",
            ],
            step="Move with wildcard on folder before filename to a folder with a wildcard before the folder declaration",
        ),
        id="C19462768_2",
        marks=[pytest.mark.test_case_id("C19462768")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,wildcard_files\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            name_change_map={
                "testFile.txt": "File",
                "testFile - Copy.txt": "File - Copy",
                "testFile - Copy (2).txt": "File - Copy (2)",
                "testFile - Copy (3).txt": "File - Copy (3)",
                "testFile - Copy (4).txt": "File - Copy (4)",
            },
            step="Move with Wildcard in Filename to a folder wildcard after the folder declaration",
        ),
        id="C19462768_3",
        marks=[pytest.mark.test_case_id("C19462768")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*.txt,wildcard_files\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="Move with Wildcard for the Filename to a folder wildcard after the folder declaration",
        ),
        id="C19462768_4",
        marks=[pytest.mark.test_case_id("C19462768")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard on File to a wildcard before the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462769_1",
        marks=[pytest.mark.test_case_id("C19462769")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_fi*\\testFile.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with wildcard on folder before filename to a folder with a wildcard before the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462769_2",
        marks=[pytest.mark.test_case_id("C19462769")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard for the Filename to a folder wildcard after the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462769_3",
        marks=[pytest.mark.test_case_id("C19462769")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,engine_dir\\Move*\\",
            encoded_output_dir="engine_dir\\MoveFile\\",
            move_successful=False,
            step="Move with Wildcard on File to a wildcard before the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462771_1",
        marks=[pytest.mark.test_case_id("C19462771")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_fi*\\testFile.txt,engine_dir\\MoveOutput*\\",
            encoded_output_dir="engine_dir\\MoveOutputles\\",
            move_successful=False,
            step="Move with wildcard on folder before filename to a folder with a wildcard before the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462771_2",
        marks=[pytest.mark.test_case_id("C19462771")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\test*.txt,engine_dir\\MoveOutput\\*",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard in Filename to a folder wildcard after the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462771_3",
        marks=[pytest.mark.test_case_id("C19462771")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = absolute-file (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*.txt,engine_dir\\MoveOutput\\*.txt",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard for the Filename to a folder wildcard after the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462771_4",
        marks=[pytest.mark.test_case_id("C19462771")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file; destination = absolute-file (file name provided)",
            asset_folder="one_txt_file",
            encoded_command="one_txt_file\\testFile.txt,project_test_assets_dir\\MoveOutput\\ferg.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            name_change_map={"testFile.txt": "ferg.txt"},
        ),
        id="C19462773",
        marks=[pytest.mark.test_case_id("C19462773")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file; destination = relative-file (File Name Provided)",
            asset_folder="one_txt_file",
            encoded_command="one_txt_file\\testFile.txt,one_txt_file\\MoveOutput\\ferg.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            name_change_map={"testFile.txt": "ferg.txt"},
        ),
        id="C19462775",
        marks=[pytest.mark.test_case_id("C19462775")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file; destination = absolute-folder",
            asset_folder="one_txt_file",
            encoded_command="one_txt_file\\testFile.txt,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
        ),
        id="C19462777",
        marks=[pytest.mark.test_case_id("C19462777")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file; destination = relative-folder",
            asset_folder="one_txt_file",
            encoded_command="one_txt_file\\testFile.txt,one_txt_file\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
        ),
        id="C19462778",
        marks=[pytest.mark.test_case_id("C19462778")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file; destination = absolute (path outside scan folder)",
            asset_folder="one_txt_file",
            encoded_command="one_txt_file\\testFile.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462779",
        marks=[pytest.mark.test_case_id("C19462779")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,project_test_assets_dir\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="Move files with partial file name",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462781_1",
        marks=[pytest.mark.test_case_id("C19462781")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*.txt,project_test_assets_dir\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="Move files with no file name",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462781_2",
        marks=[pytest.mark.test_case_id("C19462781")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_fi*\\testFile.txt,project_test_assets_dir\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard on Folder",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462781_3",
        marks=[pytest.mark.test_case_id("C19462781")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,wildcard_files\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="Move files with partial file name",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462783_1",
        marks=[pytest.mark.test_case_id("C19462783")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*.txt,wildcard_files\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="Move files with no file name",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462783_2",
        marks=[pytest.mark.test_case_id("C19462783")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_fi*\\testFile.txt,wildcard_files\\MoveOutput\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard on Folder",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462783_3",
        marks=[pytest.mark.test_case_id("C19462783")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_fi*\\testFile.txt,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard on Folder",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462785_1",
        marks=[pytest.mark.test_case_id("C19462785")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            step="Move files with partial file name",
        ),
        id="C19462785_2",
        marks=[pytest.mark.test_case_id("C19462785")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*.txt,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="Move files with no file name",
        ),
        id="C19462785_3",
        marks=[pytest.mark.test_case_id("C19462785")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_fi*\\testFile.txt,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard on Folder",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462786_1",
        marks=[pytest.mark.test_case_id("C19462786")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            step="Move files with partial file name",
        ),
        id="C19462786_2",
        marks=[pytest.mark.test_case_id("C19462786")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*.txt,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="Move files with no file name",
        ),
        id="C19462786_3",
        marks=[pytest.mark.test_case_id("C19462786")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard on File to a wildcard before the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462787_1",
        marks=[pytest.mark.test_case_id("C19462787")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_fi*\\testFile.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with wildcard on folder before filename to a folder with a wildcard before the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462787_2",
        marks=[pytest.mark.test_case_id("C19462787")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard for the Filename to a folder wildcard after the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462787_3",
        marks=[pytest.mark.test_case_id("C19462787")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_fi*\\testFile.txt,project_test_assets_dir\\MoveOutput*\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutputles\\",
            files_that_stay=[
                "imightstay.txt",
                "testFile - Copy.txt",
                "testFile - Copy (2).txt",
                "testFile - Copy (3).txt",
                "testFile - Copy (4).txt",
            ],
            step="Move with Wildcard on Folder",
        ),
        id="C19462789_1",
        marks=[pytest.mark.test_case_id("C19462789")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,project_test_assets_dir\\MoveOutput\\outputTest*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            name_change_map={
                "testFile.txt": "outputTestFile.txt",
                "testFile - Copy.txt": "outputTestFile - Copy.txt",
                "testFile - Copy (2).txt": "outputTestFile - Copy (2).txt",
                "testFile - Copy (3).txt": "outputTestFile - Copy (3).txt",
                "testFile - Copy (4).txt": "outputTestFile - Copy (4).txt",
            },
            step="Move files with partial file name",
        ),
        id="C19462789_2",
        marks=[pytest.mark.test_case_id("C19462789")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*.txt,project_test_assets_dir\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="Move files with no file name",
        ),
        id="C19462789_3",
        marks=[pytest.mark.test_case_id("C19462789")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_fi*\\testFile.txt,wildcard_files\\MoveOutput*\\testFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutputles\\",
            files_that_stay=[
                "imightstay.txt",
                "testFile - Copy.txt",
                "testFile - Copy (2).txt",
                "testFile - Copy (3).txt",
                "testFile - Copy (4).txt",
            ],
            step="Move with Wildcard on Folder",
        ),
        id="C19462791_1",
        marks=[pytest.mark.test_case_id("C19462791")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,wildcard_files\\MoveOutput\\outputTest*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            name_change_map={
                "testFile.txt": "outputTestFile.txt",
                "testFile - Copy.txt": "outputTestFile - Copy.txt",
                "testFile - Copy (2).txt": "outputTestFile - Copy (2).txt",
                "testFile - Copy (3).txt": "outputTestFile - Copy (3).txt",
                "testFile - Copy (4).txt": "outputTestFile - Copy (4).txt",
            },
            step="Move files with partial file name",
        ),
        id="C19462791_2",
        marks=[pytest.mark.test_case_id("C19462791")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*.txt,wildcard_files\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="Move files with no file name",
        ),
        id="C19462791_3",
        marks=[pytest.mark.test_case_id("C19462791")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,project_test_assets_dir\\Move*\\",
            encoded_output_dir="project_test_assets_dir\\MoveFile\\",
            move_successful=False,
            step="Move with Wildcard on File to a wildcard before the folder declaration",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462793_1",
        marks=[pytest.mark.test_case_id("C19462793")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_fi*\\testFile.txt,project_test_assets_dir\\MoveOutput*\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutputles\\",
            files_that_stay=[
                "imightstay.txt",
                "testFile - Copy.txt",
                "testFile - Copy (2).txt",
                "testFile - Copy (3).txt",
                "testFile - Copy (4).txt",
            ],
            step="Move with wildcard on folder before filename to a folder with a wildcard before the folder declaration",
        ),
        id="C19462793_2",
        marks=[pytest.mark.test_case_id("C19462793")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,project_test_assets_dir\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            name_change_map={
                "testFile.txt": "File",
                "testFile - Copy.txt": "File - Copy",
                "testFile - Copy (2).txt": "File - Copy (2)",
                "testFile - Copy (3).txt": "File - Copy (3)",
                "testFile - Copy (4).txt": "File - Copy (4)",
            },
            step="Move with Wildcard in Filename to a folder wildcard after the folder declaration",
        ),
        id="C19462793_3",
        marks=[pytest.mark.test_case_id("C19462793")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*.txt,project_test_assets_dir\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="Move with Wildcard for the Filename to a folder wildcard after the folder declaration",
        ),
        id="C19462793_4",
        marks=[pytest.mark.test_case_id("C19462793")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,wildcard_files\\Move*\\",
            encoded_output_dir="project_test_assets_dir\\MoveFile\\",
            move_successful=False,
            step="Move with Wildcard on File to a wildcard before the folder declaration",
            output_messages=["Source and destination paths must have the same number of wildcards."],
        ),
        id="C19462794_1",
        marks=[pytest.mark.test_case_id("C19462794")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_fi*\\testFile.txt,wildcard_files\\MoveOutput*\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutputles\\",
            files_that_stay=[
                "imightstay.txt",
                "testFile - Copy.txt",
                "testFile - Copy (2).txt",
                "testFile - Copy (3).txt",
                "testFile - Copy (4).txt",
            ],
            step="Move with wildcard on folder before filename to a folder with a wildcard before the folder declaration",
        ),
        id="C19462794_2",
        marks=[pytest.mark.test_case_id("C19462794")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,wildcard_files\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            files_that_stay=["imightstay.txt"],
            name_change_map={
                "testFile.txt": "File",
                "testFile - Copy.txt": "File - Copy",
                "testFile - Copy (2).txt": "File - Copy (2)",
                "testFile - Copy (3).txt": "File - Copy (3)",
                "testFile - Copy (4).txt": "File - Copy (4)",
            },
            step="Move with Wildcard in Filename to a folder wildcard after the folder declaration",
        ),
        id="C19462794_3",
        marks=[pytest.mark.test_case_id("C19462794")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*.txt,wildcard_files\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="Move with Wildcard for the Filename to a folder wildcard after the folder declaration",
        ),
        id="C19462794_4",
        marks=[pytest.mark.test_case_id("C19462794")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard on File to a wildcard before the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462795_1",
        marks=[pytest.mark.test_case_id("C19462795")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files*\\testFile.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with wildcard on folder before filename to a folder with a wildcard before the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462795_2",
        marks=[pytest.mark.test_case_id("C19462795")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*.txt,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard for the Filename to a folder wildcard after the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462795_3",
        marks=[pytest.mark.test_case_id("C19462795")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,engine_dir\\Move*\\",
            encoded_output_dir="engine_dir\\MoveFile\\",
            move_successful=False,
            step="Move with Wildcard on File to a wildcard before the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462797_1",
        marks=[pytest.mark.test_case_id("C19462797")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_fi*\\testFile.txt,engine_dir\\MoveOutput*\\",
            encoded_output_dir="engine_dir\\MoveOutputles\\",
            move_successful=False,
            step="Move with wildcard on folder before filename to a folder with a wildcard before the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462797_2",
        marks=[pytest.mark.test_case_id("C19462797")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\test*.txt,engine_dir\\MoveOutput\\*",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard in Filename to a folder wildcard after the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462797_3",
        marks=[pytest.mark.test_case_id("C19462797")],
    ),
    pytest.param(
        MoveTest(
            description="Move a file: source = relative-file (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*.txt,engine_dir\\MoveOutput\\*.txt",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            step="Move with Wildcard for the Filename to a folder wildcard after the folder declaration",
            output_messages=["Destination must exist within a scanfolder."],
        ),
        id="C19462797_4",
        marks=[pytest.mark.test_case_id("C19462797")],
    ),
    pytest.param(
        MoveTest(
            description="Moving a file: source: invalid asset in asset database",
            asset_folder="bad_file",
            encoded_command="bad_file\\badfile.bad,bad_file\\MoveOutput\\formerBadFile.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["matched an existing file but it is not a source asset"],
        ),
        id="C19462844",
        marks=pytest.mark.test_case_id("C19462844"),
    ),
    pytest.param(
        MoveTest(
            description="Moving a file: source = not provided; destination = provided",
            asset_folder="one_txt_file",
            encoded_command=",one_txt_file\\MoveOutput\\ferg.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Invalid format for move command.  Expected format is move=<source>,<destination>"],
        ),
        id="C21968351",
        marks=pytest.mark.test_case_id("C21968351"),
    ),
    pytest.param(
        MoveTest(
            description="Moving a file: source = provided; destination = not provided",
            asset_folder="one_txt_file",
            encoded_command="wildcard_files\\testFile.txt,",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Invalid format for move command.  Expected format is move=<source>,<destination>"],
        ),
        id="C21968352",
        marks=pytest.mark.test_case_id("C21968352"),
    ),
]

move_a_folder_tests = [
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder; destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\,project_test_assets_dir\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=[
                "Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory."
            ],
        ),
        id="C19444499",
        marks=pytest.mark.test_case_id("C19444499"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder; destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\,wildcard_files\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=[
                "Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory."
            ],
        ),
        id="C19444501",
        marks=pytest.mark.test_case_id("C19444501"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder; destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=[
                "Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory."
            ],
        ),
        id="C19462724",
        marks=pytest.mark.test_case_id("C19462724"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder; destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=[
                "Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory."
            ],
        ),
        id="C19462725",
        marks=pytest.mark.test_case_id("C19462725"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*,project_test_assets_dir\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step="1",
        ),
        id="C19462729_1",
        marks=pytest.mark.test_case_id("C19462729"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\,project_test_assets_dir\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step="2",
        ),
        id="C19462729_2",
        marks=pytest.mark.test_case_id("C19462729"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,project_test_assets_dir\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step="3",
        ),
        id="C19462729_3",
        marks=pytest.mark.test_case_id("C19462729"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder(wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*,wildcard_files\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step="1",
        ),
        id="C19462731_1",
        marks=pytest.mark.test_case_id("C19462731"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder(wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\,wildcard_files\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step="2",
        ),
        id="C19462731_2",
        marks=pytest.mark.test_case_id("C19462731"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder(wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,wildcard_files\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step="3",
        ),
        id="C19462731_3",
        marks=pytest.mark.test_case_id("C19462731"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step="1",
        ),
        id="C19462733_1",
        marks=pytest.mark.test_case_id("C19462733"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step="2",
        ),
        id="C19462733_2",
        marks=pytest.mark.test_case_id("C19462733"),
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="3",
        ),
        id="C19462733_3",
        marks=[pytest.mark.test_case_id("C19462733")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step="1",
        ),
        id="C19462734_1",
        marks=[pytest.mark.test_case_id("C19462734")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step="2",
        ),
        id="C19462734_2",
        marks=[pytest.mark.test_case_id("C19462734")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="3",
        ),
        id="C19462734_3",
        marks=[pytest.mark.test_case_id("C19462734")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Destination must exist within a scanfolder."],
            step="1",
        ),
        id="C19462743_1",
        marks=[pytest.mark.test_case_id("C19462743")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step="2",
        ),
        id="C19462743_2",
        marks=[pytest.mark.test_case_id("C19462743")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Destination must exist within a scanfolder."],
            step="3",
        ),
        id="C19462743_3",
        marks=[pytest.mark.test_case_id("C19462743")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,project_test_assets_dir\\MoveOutput\\test*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step="1",
        ),
        id="C19462735_1",
        marks=[pytest.mark.test_case_id("C19462735")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,project_test_assets_dir\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step="2",
        ),
        id="C19462735_2",
        marks=[pytest.mark.test_case_id("C19462735")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,project_test_assets_dir\\MoveOutput*\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutputs\\",
            name_change_map={
                "testFile.txt": "testFile.txt.txt",
                "testFile - Copy.txt": "testFile - Copy.txt.txt",
                "testFile - Copy(2).txt": "testFile - Copy(2).txt.txt",
                "testFile - Copy(3).txt": "testFile - Copy(3).txt.txt",
                "testFile - Copy(4).txt": "testFile - Copy(4).txt.txt",
                "imightstay.txt": "imightstay.txt.txt",
            },
            step="3",
        ),
        id="C19462735_3",
        marks=[pytest.mark.test_case_id("C19462735")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*,project_test_assets_dir\\MoveOutput\\test*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            name_change_map={
                "testFile.txt": "testtestFile.txt.txt",
                "testFile - Copy.txt": "testtestFile - Copy.txt.txt",
                "testFile - Copy(2).txt": "testtestFile - Copy(2).txt.txt",
                "testFile - Copy(3).txt": "testtestFile - Copy(3).txt.txt",
                "testFile - Copy(4).txt": "testtestFile - Copy(4).txt.txt",
                "imightstay.txt": "testimightstay.txt.txt",
            },
            step="4",
        ),
        id="C19462735_4",
        marks=[pytest.mark.test_case_id("C19462735")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\,wildcard_files\\MoveOutput\\test*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step="1",
        ),
        id="C19462737_1",
        marks=[pytest.mark.test_case_id("C19462737")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,ap_test_asset\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step="2",
        ),
        id="C19462737_2",
        marks=[pytest.mark.test_case_id("C19462737")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,wildcard_files\\MoveOutput*\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutputs\\",
            name_change_map={
                "testFile.txt": "testFile.txt.txt",
                "testFile - Copy.txt": "testFile - Copy.txt.txt",
                "testFile - Copy(2).txt": "testFile - Copy(2).txt.txt",
                "testFile - Copy(3).txt": "testFile - Copy(3).txt.txt",
                "testFile - Copy(4).txt": "testFile - Copy(4).txt.txt",
                "imightstay.txt": "imightstay.txt.txt",
            },
            step="3",
        ),
        id="C19462737_3",
        marks=[pytest.mark.test_case_id("C19462737")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,wildcard_files\\MoveOutput*\\test*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutputs\\",
            name_change_map={
                "testFile.txt": "testtestFile.txt.txt",
                "testFile - Copy.txt": "testtestFile - Copy.txt.txt",
                "testFile - Copy(2).txt": "testtestFile - Copy(2).txt.txt",
                "testFile - Copy(3).txt": "testtestFile - Copy(3).txt.txt",
                "testFile - Copy(4).txt": "testtestFile - Copy(4).txt.txt",
                "imightstay.txt": "testimightstay.txt.txt",
            },
            step="4",
        ),
        id="C19462737_4",
        marks=[pytest.mark.test_case_id("C19462737")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\,project_test_assets_dir\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step="1",
        ),
        id="C19462739_1",
        marks=[pytest.mark.test_case_id("C19462739")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,project_test_assets_dir\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step="2",
        ),
        id="C19462739_2",
        marks=[pytest.mark.test_case_id("C19462739")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*,project_test_assets_dir\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="3",
        ),
        id="C19462739_3",
        marks=[pytest.mark.test_case_id("C19462739")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,project_test_assets_dir\\MoveOutput*\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutputs\\",
            step="4",
        ),
        id="C19462739_4",
        marks=[pytest.mark.test_case_id("C19462739")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\,wildcard_files\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step="1",
        ),
        id="C19462740_1",
        marks=[pytest.mark.test_case_id("C19462740")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,wildcard_files\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step="2",
        ),
        id="C19462740_2",
        marks=[pytest.mark.test_case_id("C19462740")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*,wildcard_files\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step="3",
        ),
        id="C19462740_3",
        marks=[pytest.mark.test_case_id("C19462740")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,wildcard_files\\MoveOutput*\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutputs\\",
            step="4",
        ),
        id="C19462740_4",
        marks=[pytest.mark.test_case_id("C19462740")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder; destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\,project_test_assets_dir\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=[
                "Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory."
            ],
            step="1",
        ),
        id="C19462800",
        marks=[pytest.mark.test_case_id("C19462800")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder; destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\,wildcard_files\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=[
                "Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory."
            ],
        ),
        id="C19462802",
        marks=[pytest.mark.test_case_id("C19462802")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder; destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=[
                "Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory."
            ],
        ),
        id="C19462804",
        marks=[pytest.mark.test_case_id("C19462804")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder; destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=[
                "Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory."
            ],
        ),
        id="C19462805",
        marks=[pytest.mark.test_case_id("C19462805")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder; destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=[
                "Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory."
            ],
        ),
        id="C19462806",
        marks=[pytest.mark.test_case_id("C19462806")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\,project_test_assets_dir\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=[
                "Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory."
            ],
        ),
        id="C19462808_1",
        marks=[pytest.mark.test_case_id("C19462808")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\,project_test_assets_dir\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=2,
        ),
        id="C19462808_2",
        marks=[pytest.mark.test_case_id("C19462808")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-file (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,project_test_assets_dir\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step=3,
        ),
        id="C19462808_3",
        marks=[pytest.mark.test_case_id("C19462808")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder(wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*,ap_tests_assets\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step=1,
        ),
        id="C19462810_1",
        marks=[pytest.mark.test_case_id("C19462810")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder(wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\,ap_tests_assets\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=2,
        ),
        id="C19462810_2",
        marks=[pytest.mark.test_case_id("C19462810")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder(wildcards); destination = relative-file (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,ap_tests_assets\\MoveOutput\\testOutput.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step=3,
        ),
        id="C19462810_3",
        marks=[pytest.mark.test_case_id("C19462810")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=1,
        ),
        id="C19462812_1",
        marks=[pytest.mark.test_case_id("C19462812")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step=2,
        ),
        id="C19462812_2",
        marks=[pytest.mark.test_case_id("C19462812")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*,project_test_assets_dir\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step=3,
        ),
        id="C19462812_3",
        marks=[pytest.mark.test_case_id("C19462812")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step=1,
        ),
        id="C19462813_1",
        marks=[pytest.mark.test_case_id("C19462813")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=2,
        ),
        id="C19462813_2",
        marks=[pytest.mark.test_case_id("C19462813")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = relative-folder",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*,wildcard_files\\MoveOutput\\",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step=3,
        ),
        id="C19462813_3",
        marks=[pytest.mark.test_case_id("C19462813")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*,engine_dir\\MoveOutput\\*",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Destination must exist within a scanfolder."],
            step=1,
        ),
        id="C19462814_1",
        marks=[pytest.mark.test_case_id("C19462814")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,engine_dir\\MoveOutput\\*",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Destination must exist within a scanfolder."],
            step=2,
        ),
        id="C19462814_2",
        marks=[pytest.mark.test_case_id("C19462814")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\,engine_dir\\MoveOutput\\*",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=3,
        ),
        id="C19462814_3",
        marks=[pytest.mark.test_case_id("C19462814")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\,engine_dir\\MoveOutput*\\*",
            encoded_output_dir="engine_dir\\MoveOutputs\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=4,
        ),
        id="C19462814_4",
        marks=[pytest.mark.test_case_id("C19462814")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\,project_test_assets_dir\\MoveOutput\\test*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=1,
        ),
        id="C19462816_1",
        marks=[pytest.mark.test_case_id("C19462816")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,project_test_assets_dir\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step=2,
        ),
        id="C19462816_2",
        marks=[pytest.mark.test_case_id("C19462816")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,project_test_assets_dir\\MoveOutput*\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutputs\\",
            name_change_map={
                "testFile.txt": "testFile.txt.txt",
                "testFile - Copy.txt": "testFile - Copy.txt.txt",
                "testFile - Copy(2).txt": "testFile - Copy(2).txt.txt",
                "testFile - Copy(3).txt": "testFile - Copy(3).txt.txt",
                "testFile - Copy(4).txt": "testFile - Copy(4).txt.txt",
                "imightstay.txt": "imightstay.txt.txt",
            },
            step=3,
        ),
        id="C19462816_3",
        marks=[pytest.mark.test_case_id("C19462816")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-file (wildcards) (file name provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*,project_test_assets_dir\\MoveOutput\\test*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            name_change_map={
                "testFile.txt": "testtestFile.txt.txt",
                "testFile - Copy.txt": "testtestFile - Copy.txt.txt",
                "testFile - Copy(2).txt": "testtestFile - Copy(2).txt.txt",
                "testFile - Copy(3).txt": "testtestFile - Copy(3).txt.txt",
                "testFile - Copy(4).txt": "testtestFile - Copy(4).txt.txt",
                "imightstay.txt": "testimightstay.txt.txt",
            },
            step=4,
        ),
        id="C19462816_4",
        marks=[pytest.mark.test_case_id("C19462816")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\,wildcard_files\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=1,
        ),
        id="C19462818_1",
        marks=[pytest.mark.test_case_id("C19462818")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,wildcard_files\\MoveOutput\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step=2,
        ),
        id="C19462818_2",
        marks=[pytest.mark.test_case_id("C19462818")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,wildcard_files\\MoveOutput*\\*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutputs\\",
            name_change_map={
                "testFile.txt": "testFile.txt.txt",
                "testFile - Copy.txt": "testFile - Copy.txt.txt",
                "testFile - Copy(2).txt": "testFile - Copy(2).txt.txt",
                "testFile - Copy(3).txt": "testFile - Copy(3).txt.txt",
                "testFile - Copy(4).txt": "testFile - Copy(4).txt.txt",
                "imightstay.txt": "imightstay.txt.txt",
            },
            step=3,
        ),
        id="C19462818_3",
        marks=[pytest.mark.test_case_id("C19462818")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = relative-file (wildcards) (File Name Provided)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*,wildcard_files\\MoveOutput\\test*.txt",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            name_change_map={
                "testFile.txt": "testtestFile.txt.txt",
                "testFile - Copy.txt": "testtestFile - Copy.txt.txt",
                "testFile - Copy(2).txt": "testtestFile - Copy(2).txt.txt",
                "testFile - Copy(3).txt": "testtestFile - Copy(3).txt.txt",
                "testFile - Copy(4).txt": "testtestFile - Copy(4).txt.txt",
                "imightstay.txt": "testimightstay.txt.txt",
            },
            step=4,
        ),
        id="C19462818_4",
        marks=[pytest.mark.test_case_id("C19462818")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\,project_test_assets_dir\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=1,
        ),
        id="C19462820_1",
        marks=[pytest.mark.test_case_id("C19462820")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,project_test_assets_dir\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step=2,
        ),
        id="C19462820_2",
        marks=[pytest.mark.test_case_id("C19462820")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*,project_test_assets_dir\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step=3,
        ),
        id="C19462820_3",
        marks=[pytest.mark.test_case_id("C19462820")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files*\\*,project_test_assets_dir\\MoveOutput*\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step=4,
        ),
        id="C19462820_4",
        marks=[pytest.mark.test_case_id("C19462820")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\,wildcard_files\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=1,
        ),
        id="C19462821_1",
        marks=[pytest.mark.test_case_id("C19462821")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,wildcard_files\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Source and destination paths must have the same number of wildcards."],
            step=2,
        ),
        id="C19462821_2",
        marks=[pytest.mark.test_case_id("C19462821")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*,wildcard_files\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step=3,
        ),
        id="C19462821_3",
        marks=[pytest.mark.test_case_id("C19462821")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = relative-folder (wildcards)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files*\\*,wildcard_files\\MoveOutput*\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            step=4,
        ),
        id="C19462821_4",
        marks=[pytest.mark.test_case_id("C19462821")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\*,engine_dir\\MoveOutput\\*",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Destination must exist within a scanfolder."],
            step=1,
        ),
        id="C19462824_1",
        marks=[pytest.mark.test_case_id("C19462824")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,engine_dir\\MoveOutput\\*",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            output_messages=["Destination must exist within a scanfolder."],
            move_successful=False,
            step=2,
        ),
        id="C19462824_2",
        marks=[pytest.mark.test_case_id("C19462824")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\,engine_dir\\MoveOutput\\*",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=3,
        ),
        id="C19462824_3",
        marks=[pytest.mark.test_case_id("C19462824")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = relative-folder (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="wildcard_file*\\*,engine_dir\\MoveOutput*\\*",
            encoded_output_dir="engine_dir\\MoveOutputs\\",
            output_messages=["Destination must exist within a scanfolder."],
            move_successful=False,
            step=4,
        ),
        id="C19462824_4",
        marks=[pytest.mark.test_case_id("C19462824")],
    ),
    pytest.param(
        MoveTest(
            description="Moving a folder: source = not provided; destination = provided",
            asset_folder="wildcard_files",
            encoded_command=",wildcard_files\\MoveOutput\\*",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Invalid format for move command.  Expected format is move=<source>,<destination>"],
        ),
        id="C21968353",
        marks=[pytest.mark.test_case_id("C21968353")],
    ),
    pytest.param(
        MoveTest(
            description="Moving a folder: source = not provided; destination = provided",
            asset_folder="wildcard_files",
            encoded_command="wildcard_files\\MoveOutput\\*,",
            encoded_output_dir="project_test_assets_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Invalid format for move command.  Expected format is move=<source>,<destination>"],
        ),
        id="C21968354",
        marks=[pytest.mark.test_case_id("C21968354")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder; destination = absolute (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\,engine_dir\\MoveOutput\\",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=[
                "Cannot operate on directories.  Please specify a file or use a wildcard to select all files within a directory."
            ],
        ),
        id="C19462727",
        marks=[pytest.mark.test_case_id("C19462727")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_test_assets_dir\\*,engine_dir\\MoveOutput\\*",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Destination must exist within a scanfolder."],
            step=1,
        ),
        id="C19462745_1",
        marks=[pytest.mark.test_case_id("C19462745")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,engine_dir\\MoveOutput\\*",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Destination must exist within a scanfolder."],
            step=2,
        ),
        id="C19462745_2",
        marks=[pytest.mark.test_case_id("C19462745")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\,engine_dir\\MoveOutput\\*",
            encoded_output_dir="engine_dir\\MoveOutput\\",
            move_successful=False,
            output_messages=["Wildcard search did not match any files."],
            step=3,
        ),
        id="C19462745_3",
        marks=[pytest.mark.test_case_id("C19462745")],
    ),
    pytest.param(
        MoveTest(
            description="Move a folder: source = absolute-folder (wildcards); destination = absolute (wildcards) (path outside scan folder)",
            asset_folder="wildcard_files",
            encoded_command="project_dir\\wildcard_file*\\*,engine_dir\\MoveOutput*\\*",
            encoded_output_dir="engine_dir\\MoveOutputs\\",
            move_successful=False,
            output_messages=["Destination must exist within a scanfolder."],
            step=4,
        ),
        id="C19462745_4",
        marks=[pytest.mark.test_case_id("C19462745")],
    ),
]


@pytest.mark.usefixtures("asset_processor")
@pytest.mark.usefixtures("ap_setup_fixture")
@pytest.mark.usefixtures("local_resources")
@pytest.mark.usefixtures("clear_moveoutput")
@pytest.mark.parametrize("project", targetProjects)
@pytest.mark.assetpipeline
@pytest.mark.SUITE_periodic
class TestsAssetProcessorMove_WindowsAndMac:

    # run one test:
    # -k C19462747_1
    # run all the tests for a test case:
    # -k C19462747

    @pytest.mark.skipif(sys.platform.startswith('linux'), reason="https://github.com/o3de/o3de/issues/14514")
    @pytest.mark.parametrize("test", move_a_file_tests + move_a_folder_tests)
    def test_WindowsMacPlatforms_MoveCommand_CommandResult(self, asset_processor, ap_setup_fixture, test: MoveTest, project):
        """

        Test Steps:
        1. Create temporary testing environment based on test data
        2. Validate that temporary testing environment was created successfully
        3. Execute the move command based upon the test data
        4. Validate that files are where they're expected according to the test data
        5. Validate unexpected files are not found according to the test data
        6. Validate output messages according to the test data
        7. Validate move status according to the test data
        """

        source_folder, _ = asset_processor.prepare_test_environment(ap_setup_fixture["tests_dir"], test.asset_folder)
        test.map_env(ap_setup_fixture, source_folder)
        # validate the tests assets are correct
        # hard coding the expected assets so if they change the test fails
        bad_file_path = os.path.join(test.env["tests_dir"], "assets", "bad_file", "badfile.bad")
        one_file_path = os.path.join(test.env["tests_dir"], "assets", "one_txt_file", "testFile.txt")
        wildcard_files = [
            "imightstay.txt",
            "testFile.txt",
            "testFile - Copy.txt",
            "testFile - Copy (2).txt",
            "testFile - Copy (3).txt",
            "testFile - Copy (4).txt",
        ]
        assert os.path.exists(bad_file_path), f"no file exists at the following path but it should: {bad_file_path}"
        assert os.path.exists(one_file_path), f"no file exists at the following path but it should: {one_file_path}"
        for file in wildcard_files:
            path = os.path.join(test.env["tests_dir"], "assets", "wildcard_files", file)
            assert os.path.exists(path), f"no file exists at the following path but it should: {path}"
        # validate only these files exist
        assert (
            len([name for name in os.listdir(os.path.join(test.env["tests_dir"], "assets", "bad_file"))]) == 1
        ), "extra files or directories are in assets/bad_file"
        assert (
            len([name for name in os.listdir(os.path.join(test.env["tests_dir"], "assets", "one_txt_file"))]) == 1
        ), "extra files or directories are in assets/one_txt_file"
        assert len(
            [name for name in os.listdir(os.path.join(test.env["tests_dir"], "assets", "wildcard_files"))]
        ) == len(wildcard_files), "extra files or directories are in assets/wildcard_files"

        # validate the test assets are in the project folder (validate test_assets fixture works)
        for filepath in test.test_file_paths:
            assert os.path.exists(os.path.join(source_folder, filepath)), f"Test asset is missing: {filepath}"
        result, command_output = asset_processor.batch_process(extra_params=test.command(), capture_output=True)
        lowercase_output = str(list(map(str.lower, command_output)))
        for path in test.expected_filepaths:
            assert os.path.exists(path), f"file should be found at path: {path}"
        for path in test.unexpected_filepaths:
            assert not os.path.exists(path), f"file should NOT be found at path: {filepath}"
        if test.output_messages:
            for message in test.output_messages:
                assert (
                    message.lower() in lowercase_output
                ), f"{message.lower()} not in command output\n{lowercase_output}"
        if test.move_successful:
            for line in test.output_to_check:
                assert line.lower() in lowercase_output