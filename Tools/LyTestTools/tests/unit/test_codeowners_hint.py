"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ly_test_tools.cli.codeowners_hint
"""
import pathlib

import pytest

import ly_test_tools.cli.codeowners_hint as hint
import unittest.mock as mock

_DUMMY_REPO_PATH = "/home/myuser/github/myrepo"
_DUMMY_CODEOWNERS_PATH = _DUMMY_REPO_PATH + "/.github/CODEOWNERS"

_dummy_target_paths = [
    ".",
    "relative/path",
    "/absolute/path",
    "/some/fairly/long/path/which/eventually/ends/in/a/directory/a/s/d/f/g/h/j/k/l",
    "/some/fairly/long/path/which/eventually/ends/in/a/file/a/s/d/f/g/h/j/k/l/dummy.txt"
    "C:\\github\\o3de\\Tools\\LyTestTools\\tests\\unit\\test_codeowners_hint.py",
    "/home/myuser/github/o3de/Tools/LyTestTools/tests/unit/test_codeowners_hint.py",
]


@mock.patch("os.path.exists")
@pytest.mark.parametrize("target_path", _dummy_target_paths)
def test_FindCodeowners_MockExistsInRepo_Found(mock_exists, target_path):
    mock_exists.side_effect = [False, True]  # found in parent directory
    codeowners_path = hint.find_github_codeowners(target_path)
    assert codeowners_path
    mock_exists.assert_called()


@mock.patch("os.path.exists")
@pytest.mark.parametrize("target_path", _dummy_target_paths)
def test_FindCodeowners_MockNotInRepo_NotFound(mock_exists, target_path):
    mock_exists.return_value = False  # never found
    codeowners_path = hint.find_github_codeowners(target_path)
    assert not codeowners_path
    mock_exists.assert_called()


def _assert_owners_from(target_relative_path, mock_ownership_data, expected_path_match, expected_alias_match):
    """
    :param target_relative_path: string path relative to root of repo, starting with slash
    :param mock_ownership_data: string contents representing a CODEOWNERS file
    :param expected_path_match: expected line's path-value to match
    :param expected_alias_match: expected line's alias-value to match
    """
    with mock.patch('builtins.open', mock.mock_open(read_data=mock_ownership_data)):
        path, alias = hint.get_codeowners_from(pathlib.Path(_DUMMY_REPO_PATH + target_relative_path),
                                               pathlib.Path(_DUMMY_CODEOWNERS_PATH))
    assert path == expected_path_match
    assert alias == expected_alias_match


@mock.patch("os.path.isfile", mock.MagicMock(return_value=True))
@mock.patch("os.path.getsize", mock.MagicMock(return_value=1))
class TestGetOwnersFrom:
    def test_ExactMatch_OneExactMatch_Matched(self):
        target = "/some/path/to/my/file"
        ownership_data = target + "  @alias\n"
        _assert_owners_from(target, ownership_data, target, "@alias")

    @pytest.mark.parametrize("target_path,codeowners_path", [
        (pathlib.PurePosixPath("/some/owned/unix/path"), pathlib.PurePosixPath("/some/.github/CODEOWNERS")),
        (pathlib.PurePosixPath("/some/owned/unix/.dot/path"), pathlib.PurePosixPath("/some/.github/CODEOWNERS")),
        (pathlib.PureWindowsPath("\\some\\owned\\windows\\path"), pathlib.PureWindowsPath("\\some\\.github\\CODEOWNERS")),
        (pathlib.PureWindowsPath("\\some\\owned\\windows\\.dot\\path"), pathlib.PureWindowsPath("\\some\\.github\\CODEOWNERS")),
    ])
    def test_ExactMatch_VariantSystemPaths_Matched(self, target_path, codeowners_path):
        with mock.patch('builtins.open', mock.mock_open(read_data=f"/owned @alias")):
            path, alias = hint.get_codeowners_from(target_path, codeowners_path)
        assert path, "unexpectedly None or empty"
        assert alias, "unexpectedly None or empty"

    def test_NoMatches_MultipleMismatched_Negative(self):
        target = _DUMMY_REPO_PATH + "/does/not/exist"
        ownership_data = \
            "/a/b/c/ @alias\n"\
            "/a/b/d/ @alias2\n"\
            "/a/b/e/ @alias3\n"\
            "/a/b/f/ @alias4\n"
        # warning: negative assertions can easily pass by accident
        _assert_owners_from(target, ownership_data, "", "")

    def test_WildcardMatch_GlobalWildcard_Matches(self):
        target = _DUMMY_REPO_PATH + "/dummy.py"
        ownership_data = "* @alias\n"
        _assert_owners_from(target, ownership_data, "*", "@alias")

    def test_WildcardMatch_GlobalExtensionWildcard_Matches(self):
        target = _DUMMY_REPO_PATH + "/some/dummy.py"
        ownership_data = "*.py @alias\n"
        _assert_owners_from(target, ownership_data, "*.py", "@alias")

    def test_WildcardMismatch_GlobalExtensionWildcard_Negative(self):
        target = _DUMMY_REPO_PATH + "/some/dummy.py"
        ownership_data = "*.txt @alias\n"
        # warning: negative assertions can easily pass by accident
        _assert_owners_from(target, ownership_data, "", "")

    def test_WildcardMatch_RootedExtensionWildcard_Matches(self):
        target = _DUMMY_REPO_PATH + "/some/dummy.py"
        ownership_data = "/some/*.py @alias\n"
        _assert_owners_from(target, ownership_data, "/some/*.py", "@alias")

    def test_WildcardMatch_UnrootedExtensionWildcard_Matches(self):
        target = _DUMMY_REPO_PATH + "/some/other/dummy.py"
        ownership_data = "other/*.py @alias\n"
        _assert_owners_from(target, ownership_data, "other/*.py", "@alias")

    def test_WildcardMatch_RootedQuestionWildcard_Matches(self):
        target = _DUMMY_REPO_PATH + "/some/dummy.py"
        ownership_data = "/some/du?my.py @alias\n"
        _assert_owners_from(target, ownership_data, "/some/du?my.py", "@alias")

    def test_WildcardMatch_UnrootedQuestionWildcard_Matches(self):
        target = _DUMMY_REPO_PATH + "/some/dummy.py"
        ownership_data = "some/du?my.py @alias\n"
        _assert_owners_from(target, ownership_data, "some/du?my.py", "@alias")

    def test_WildcardMatch_ComplexRootedExtension_Matches(self):
        target = _DUMMY_REPO_PATH + "/some/path/to/my/dummy.py"
        ownership_data = "some/pa?h/to/*/*.py @alias\n"
        _assert_owners_from(target, ownership_data, "some/pa?h/to/*/*.py", "@alias")

    def test_WildcardMatch_ComplexRelativePattern_Matches(self):
        target = _DUMMY_REPO_PATH + "/some/path/to/my/dummy.py"
        ownership_data = "path/t?/my/*.py @alias\n"
        _assert_owners_from(target, ownership_data, "path/t?/my/*.py", "@alias")

    def test_PartialMatch_MultipleMatches_FinalMatch(self):
        target = "/some/path/to/my/file"
        ownership_data = \
            "* @alias\n"\
            "/some/path/ @alias2\n"\
            "/some/path/to/my/ @alias3\n"\
            "/some/other/mismatch/ @alias4\n"
        _assert_owners_from(target, ownership_data, "/some/path/to/my/", "@alias3")

    def test_PartialMatch_NonFinalExactMatch_FinalMatched(self):
        target = "/some/path/to/my/file"
        ownership_data = \
            "* @alias\n"\
            "/some/path/ @alias2\n"\
            "/some/path/to/my/file @alias3\n"\
            "/some/path/to/my/ @alias4\n"  # expect final matching entry, despite the previous exact match
        _assert_owners_from(target, ownership_data, "/some/path/to/my/", "@alias4")
