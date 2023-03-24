"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ly_test_tools._internal.managers.platforms.windows
"""

import unittest.mock as mock
import os
import pytest
import ly_test_tools

from ly_test_tools._internal.managers.platforms.windows import (
    _WindowsResourceLocator, WindowsWorkspaceManager,
    CACHE_DIR)
from ly_test_tools import WINDOWS

pytestmark = pytest.mark.SUITE_smoke

if not WINDOWS:
    pytestmark = pytest.mark.skipif(
        not WINDOWS,
        reason="test_manager_platforms_windows.py only runs on Windows")

mock_engine_root = 'mock_engine_root'
mock_dev_path = 'mock_dev_path'
mock_build_directory = 'mock_build_directory'
mock_project = 'mock_project'
mock_tmp_path = 'mock_tmp_path'
mock_output_path = 'mock_output_path'


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=mock_engine_root))
@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_project_json', mock.MagicMock(
    return_value=mock_project))
class TestWindowsResourceLocator(object):

    def test_PlatformCache_HasPath_ReturnsPath(self):
        windows_resource_locator = ly_test_tools._internal.managers.platforms.windows._WindowsResourceLocator(
            mock_build_directory, mock_project)
        expected = os.path.join(
            windows_resource_locator.project_cache(), CACHE_DIR)

        assert windows_resource_locator.platform_cache() == expected

    def test_ProjectLog_HasPath_ReturnsPath(self):
        windows_resource_locator = ly_test_tools._internal.managers.platforms.windows._WindowsResourceLocator(
            mock_build_directory, mock_project)
        expected = os.path.join(
            windows_resource_locator.project(),
            'user',
            'log')

        assert windows_resource_locator.project_log() == expected

    def test_ProjectScreenshots_HasPath_ReturnsPath(self):
        windows_resource_locator = ly_test_tools._internal.managers.platforms.windows._WindowsResourceLocator(
            mock_build_directory, mock_project)
        expected = os.path.join(
            windows_resource_locator.project(),
            'user',
            'ScreenShots')

        assert windows_resource_locator.project_screenshots() == expected

    def test_EditorLog_HasPath_ReturnsPath(self):
        windows_resource_locator = ly_test_tools._internal.managers.platforms.windows._WindowsResourceLocator(
            mock_build_directory, mock_project)
        expected = os.path.join(
            windows_resource_locator.project_log(),
            'Editor.log')

        assert windows_resource_locator.editor_log() == expected


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=mock_engine_root))
@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_project_json', mock.MagicMock(
    return_value=mock_project))
class TestWindowsWorkspaceManager(object):

    @mock.patch('ly_test_tools.environment.reg_cleaner.create_ly_keys')
    def test_SetRegistryKeys_NewWorkspaceManager_KeyCreateCalled(self, mock_create_keys):
        windows_workspace_manager = ly_test_tools._internal.managers.platforms.windows.WindowsWorkspaceManager()
        windows_workspace_manager.set_registry_keys()

        mock_create_keys.assert_called_once()

    @mock.patch('ly_test_tools.environment.reg_cleaner.clean_ly_keys')
    def test_ClearSettings_NewWorkspaceManager_KeyClearCalled(self, mock_clear_keys):
        windows_workspace_manager = ly_test_tools._internal.managers.platforms.windows.WindowsWorkspaceManager()
        windows_workspace_manager.clear_settings()

        mock_clear_keys.assert_called_with(exception_list=r"SOFTWARE\O3DE\O3DE\Identity")
