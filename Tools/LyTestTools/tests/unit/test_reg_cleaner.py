"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest
from pytest_mock import MockFixture

import ly_test_tools.environment.reg_cleaner
from ly_test_tools import WINDOWS

pytestmark = pytest.mark.SUITE_smoke


if not WINDOWS:
    pytestmark = pytest.mark.skipif(
        not WINDOWS,
        reason="tests.unit.test_reg_cleaner is restricted to the Windows platform.")


def test_DeleteKey_KeyDoesNotExist_SilentlyFails(mocker):
    # type: (MockFixture) -> None
    mock_delete_child_keys_and_values = mocker.patch(
        "ly_test_tools.environment.reg_cleaner._delete_child_keys_and_values")
    mock_delete_child_keys_and_values.side_effect = WindowsError

    ly_test_tools.environment.reg_cleaner._delete_key("key_1")


def test_DeleteKey_KeyIsEmpty_KeyDeleted(mocker):
    # type: (MockFixture) -> None
    mockwinreg = mocker.patch("ly_test_tools.environment.reg_cleaner.winreg")
    mock_delete_child_keys_and_values = mocker.patch(
        "ly_test_tools.environment.reg_cleaner._delete_child_keys_and_values")
    mock_delete_child_keys_and_values.return_value = True

    ly_test_tools.environment.reg_cleaner._delete_key("key_1")

    assert mockwinreg.DeleteKey.called


def test_DeleteKey_KeyIsNotEmpty_KeyNotDeleted(mocker):
    # type: (MockFixture) -> None
    mockwinreg = mocker.patch("ly_test_tools.environment.reg_cleaner.winreg")
    mock_delete_child_keys_and_values = mocker.patch(
        "ly_test_tools.environment.reg_cleaner._delete_child_keys_and_values")
    mock_delete_child_keys_and_values.return_value = False

    ly_test_tools.environment.reg_cleaner._delete_key("key_1")

    assert not mockwinreg.DeleteKey.called


def test_DeleteChildKeysAndValues_KeyDoesNotExist_RaisesWindowsError(mocker):
    # type: (MockFixture) -> None
    mockwinreg = mocker.patch("ly_test_tools.environment.reg_cleaner.winreg")
    mockwinreg.OpenKey.side_effect = WindowsError

    with pytest.raises(WindowsError):
        ly_test_tools.environment.reg_cleaner._delete_child_keys_and_values("key_1")


def test_DeleteChildKeysAndValues_KeyIsEmpty_ReturnsTrue(mocker):
    # type: (MockFixture) -> None
    mockwinreg = mocker.patch("ly_test_tools.environment.reg_cleaner.winreg")
    mockwinreg.EnumKey.side_effect = WindowsError
    mockwinreg.EnumValue.side_effect = WindowsError

    assert ly_test_tools.environment.reg_cleaner._delete_child_keys_and_values("key_1")


def test_DeleteChildKeysAndValues_KeyWithNoChildrenInExceptionList_ReturnsTrue(mocker):
    # type: (MockFixture) -> None
    mockwinreg = mocker.patch("ly_test_tools.environment.reg_cleaner.winreg")
    mockwinreg.EnumKey.side_effect = ["child_key_1", WindowsError, WindowsError]
    mockwinreg.EnumValue.side_effect = [WindowsError, ("child_value_1", None, None), WindowsError]

    assert ly_test_tools.environment.reg_cleaner._delete_child_keys_and_values("key_1")


def test_DeleteChildKeysAndValues_KeyWithChildrenInExceptionList_ReturnsFalse(mocker):
    # type: (MockFixture) -> None
    mockwinreg = mocker.patch("ly_test_tools.environment.reg_cleaner.winreg")
    mockwinreg.EnumKey.side_effect = ["child_key_1", WindowsError]
    mockwinreg.EnumValue.side_effect = [("child_value_1", None, None), WindowsError]
    exception_list = ["key_1\\child_key_1", "key_1\\child_value_1"]

    assert not ly_test_tools.environment.reg_cleaner._delete_child_keys_and_values("key_1", exception_list)
