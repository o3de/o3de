"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from typing import List
from unittest import TestCase
from unittest.mock import (MagicMock, patch)

from utils import file_utils


class TestFileUtils(TestCase):
    """
    file utils unit test cases
    TODO: add test cases once error handling is ready
    """
    _expected_path_name: str = "dummy/path/"
    _expected_suffix: str = "_dummy_suffix.json"
    _expected_file_name: str = "dummy.json"
    
    def setUp(self) -> None:
        path_patcher: patch = patch("pathlib.Path")
        self.addCleanup(path_patcher.stop)
        self._mock_path: MagicMock = path_patcher.start()

    def test_check_path_exists_returns_true(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_path.exists.return_value = True

        actual_result: bool = file_utils.check_path_exists(TestFileUtils._expected_path_name)
        self._mock_path.assert_called_once_with(TestFileUtils._expected_path_name)
        mocked_path.exists.assert_called_once()
        assert actual_result

    def test_check_path_exists_returns_false(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_path.exists.return_value = False

        actual_result: bool = file_utils.check_path_exists(TestFileUtils._expected_path_name)
        self._mock_path.assert_called_once_with(TestFileUtils._expected_path_name)
        mocked_path.exists.assert_called_once()
        assert not actual_result

    def test_create_directory_return_true(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value

        actual_result: bool = file_utils.create_directory("dummy")
        self._mock_path.assert_called_once()
        mocked_path.mkdir.assert_called_once()
        assert actual_result

    def test_create_directory_return_false_when_exception_raised(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_path.mkdir.side_effect = FileExistsError()

        actual_result: bool = file_utils.create_directory("dummy")
        self._mock_path.assert_called_once()
        mocked_path.mkdir.assert_called_once()
        assert not actual_result

    def test_get_current_directory_path_return_expected_path_name(self) -> None:
        self._mock_path.cwd.return_value = TestFileUtils._expected_path_name

        actual_path_name: str = file_utils.get_current_directory_path()
        self._mock_path.cwd.assert_called_once()
        assert actual_path_name == TestFileUtils._expected_path_name

    def test_get_parent_directory_path_return_empty_when_invalid_input(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_path.exists.return_value = False

        actual_path_name: str = file_utils.get_parent_directory_path("dummy")
        self._mock_path.assert_called_once()
        assert actual_path_name == ""

    def test_get_parent_directory_path_return_empty_when_parent_invalid(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_path.exists.return_value = True
        mocked_parent_path: MagicMock = MagicMock()
        mocked_path.parent = mocked_parent_path
        mocked_parent_path.exists.return_value = False

        actual_path_name: str = file_utils.get_parent_directory_path("dummy")
        self._mock_path.assert_called()
        assert actual_path_name == ""

    def test_get_parent_directory_path_return_expected_path_when_parent_valid(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_path.exists.return_value = True
        mocked_parent_path: MagicMock = MagicMock()
        mocked_path.parent = mocked_parent_path
        mocked_parent_path.exists.return_value = True
        mocked_parent_path.resolve.return_value = TestFileUtils._expected_file_name

        actual_path_name: str = file_utils.get_parent_directory_path("dummy")
        self._mock_path.assert_called()
        assert actual_path_name == TestFileUtils._expected_file_name

    def test_get_parent_directory_path_return_empty_when_level_two_parent_invalid(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_path.exists.return_value = True
        mocked_parent_path1: MagicMock = MagicMock()
        mocked_path.parent = mocked_parent_path1
        mocked_parent_path1.exists.return_value = True
        mocked_parent_path2: MagicMock = MagicMock()
        mocked_parent_path1.parent = mocked_parent_path2
        mocked_parent_path2.exists.return_value = False

        actual_path_name: str = file_utils.get_parent_directory_path("dummy", 2)
        self._mock_path.assert_called()
        assert actual_path_name == ""

    def test_get_parent_directory_path_return_expected_path_when_level_two_parent_valid(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_path.exists.return_value = True
        mocked_parent_path1: MagicMock = MagicMock()
        mocked_path.parent = mocked_parent_path1
        mocked_parent_path1.exists.return_value = True
        mocked_parent_path2: MagicMock = MagicMock()
        mocked_parent_path1.parent = mocked_parent_path2
        mocked_parent_path2.exists.return_value = True
        mocked_parent_path2.resolve.return_value = TestFileUtils._expected_file_name

        actual_path_name: str = file_utils.get_parent_directory_path("dummy", 2)
        self._mock_path.assert_called()
        assert actual_path_name == TestFileUtils._expected_file_name

    def test_find_files_with_suffix_under_directory_return_expected_file_name(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_matched_path: MagicMock = MagicMock()
        mocked_path.glob.return_value = [mocked_matched_path]
        mocked_matched_path.is_file.return_value = True
        mocked_matched_path.name = TestFileUtils._expected_file_name

        actual_files: List[str] = \
            file_utils.find_files_with_suffix_under_directory(TestFileUtils._expected_path_name,
                                                              TestFileUtils._expected_suffix)
        self._mock_path.assert_called_once_with(TestFileUtils._expected_path_name)
        mocked_path.glob.assert_called_once_with(f"*{TestFileUtils._expected_suffix}")
        assert actual_files == [TestFileUtils._expected_file_name]

    def test_find_files_with_suffix_under_directory_return_empty_list_when_matched_not_a_file(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_matched_path: MagicMock = MagicMock()
        mocked_path.glob.return_value = [mocked_matched_path]
        mocked_matched_path.is_file.return_value = False

        actual_files: List[str] = \
            file_utils.find_files_with_suffix_under_directory(TestFileUtils._expected_path_name,
                                                              TestFileUtils._expected_suffix)
        self._mock_path.assert_called_once_with(TestFileUtils._expected_path_name)
        mocked_path.glob.assert_called_once_with(f"*{TestFileUtils._expected_suffix}")
        assert not actual_files

    def test_find_files_with_suffix_under_directory_return_empty_list_when_no_matching(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_path.glob.return_value = []

        actual_files: List[str] = \
            file_utils.find_files_with_suffix_under_directory(TestFileUtils._expected_path_name,
                                                              TestFileUtils._expected_suffix)
        self._mock_path.assert_called_once_with(TestFileUtils._expected_path_name)
        mocked_path.glob.assert_called_once_with(f"*{TestFileUtils._expected_suffix}")
        assert not actual_files

    def test_join_path_return_expected_result(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        expected_join_path_name: str = f"{TestFileUtils._expected_path_name}{TestFileUtils._expected_file_name}"
        mocked_path.joinpath.return_value = expected_join_path_name

        actual_join_path_name: str = file_utils.join_path(TestFileUtils._expected_path_name,
                                                          TestFileUtils._expected_file_name)
        self._mock_path.assert_called_once_with(TestFileUtils._expected_path_name)
        mocked_path.joinpath.assert_called_once_with(TestFileUtils._expected_file_name)
        assert actual_join_path_name == expected_join_path_name

    def test_normalize_file_path_return_empty_when_input_is_empty(self) -> None:
        actual_normalized_path: str = file_utils.normalize_file_path("")
        assert actual_normalized_path == ""

    def test_normalize_file_path_return_expected_result(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        expected_resolve_path: str = TestFileUtils._expected_path_name
        mocked_path.resolve.return_value = expected_resolve_path

        actual_resolve_path: str = file_utils.normalize_file_path("dummy")
        self._mock_path.assert_called_once()
        mocked_path.resolve.assert_called_once()
        assert actual_resolve_path == expected_resolve_path

    def test_normalize_file_path_return_empty_when_exception_raised(self) -> None:
        mocked_path: MagicMock = self._mock_path.return_value
        mocked_path.resolve.side_effect = RuntimeError()

        actual_resolve_path: str = file_utils.normalize_file_path("dummy")
        self._mock_path.assert_called_once()
        mocked_path.resolve.assert_called_once()
        assert actual_resolve_path == ""
