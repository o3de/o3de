"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit tests for ly_test_tools._internal.managers.platforms.mac
"""

import unittest.mock as mock
import os
import pytest
import ly_test_tools

from ly_test_tools._internal.managers.platforms.mac import (
    _MacResourceLocator, MacWorkspaceManager,
    CACHE_DIR)
from ly_test_tools import MAC

pytestmark = pytest.mark.SUITE_smoke

mock_engine_root = 'mock_engine_root'
mock_dev_path = 'mock_dev_path'
mock_build_directory = 'mock_build_directory'
mock_project = 'mock_project'
mock_tmp_path = 'mock_tmp_path'
mock_output_path = 'mock_output_path'


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=mock_engine_root))
@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_project_json',
            mock.MagicMock(return_value=mock_project))
class TestMacResourceLocator(object):

    def test_PlatformCache_HasPath_ReturnsPath(self):
        mac_resource_locator = ly_test_tools._internal.managers.platforms.mac._MacResourceLocator(mock_build_directory,
                                                                                                  mock_project)
        expected = os.path.join(
            mac_resource_locator.project_cache(),
            CACHE_DIR)

        assert mac_resource_locator.platform_cache() == expected

    def test_ProjectLog_HasPath_ReturnsPath(self):
        mac_resource_locator = ly_test_tools._internal.managers.platforms.mac._MacResourceLocator(mock_build_directory,
                                                                                                  mock_project)
        expected = os.path.join(
            mac_resource_locator.project(),
            'user',
            'log')

        assert mac_resource_locator.project_log() == expected

    def test_ProjectScreenshots_HasPath_ReturnsPath(self):
        mac_resource_locator = ly_test_tools._internal.managers.platforms.mac._MacResourceLocator(mock_build_directory,
                                                                                                  mock_project)
        expected = os.path.join(
            mac_resource_locator.project(),
            'user',
            'ScreenShots')

        assert mac_resource_locator.project_screenshots() == expected

    def test_EditorLog_HasPath_ReturnsPath(self):
        mac_resource_locator = ly_test_tools._internal.managers.platforms.mac._MacResourceLocator(mock_build_directory,
                                                                                                  mock_project)
        expected = os.path.join(
            mac_resource_locator.project_log(),
            'Editor.log')

        assert mac_resource_locator.editor_log() == expected


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=mock_engine_root))
@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_project_json',
            mock.MagicMock(return_value=mock_project))
class TestMacWorkspaceManager(object):

    def test_Init_SetDummyParams_ReturnsMacWorkspaceManager(self):
        mac_workspace_manager = MacWorkspaceManager(
            build_directory=mock_build_directory,
            project=mock_project,
            tmp_path=mock_tmp_path,
            output_path=mock_output_path)

        assert type(mac_workspace_manager) == MacWorkspaceManager
        assert mac_workspace_manager.paths.build_directory() == mock_build_directory
        assert mac_workspace_manager.paths._project == mock_project
        assert mac_workspace_manager.tmp_path == mock_tmp_path
        assert mac_workspace_manager.output_path == mock_output_path
