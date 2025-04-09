"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit Tests for ly_test_tools._internal.managers.abstract_resource_locator
"""
import os

import unittest.mock as mock
import pytest

import ly_test_tools._internal.managers.abstract_resource_locator as abstract_resource_locator

from ly_test_tools._internal.exceptions import LyTestToolsFrameworkException

pytestmark = pytest.mark.SUITE_smoke

mock_initial_path = "mock_initial_path"
mock_engine_root = "mock_engine_root"
mock_dev_path = "mock_dev_path"
mock_build_directory = 'mock_build_directory'
mock_project = 'mock_project'
mock_manifest_json_file = {'projects': [mock_project]}
mock_project_json_file = {'project_name': mock_project}
mock_project_json = os.path.join(mock_project, 'project.json')


class TestFindEngineRoot(object):

    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.abspath', mock.MagicMock())
    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.exists')
    def test_FindEngineRoot_InitialPathExists_ReturnsTuple(self, mock_path_exists):
        mock_path_exists.return_value = True

        engine_root = abstract_resource_locator._find_engine_root(mock_engine_root)

        assert engine_root == mock_engine_root
        mock_path_exists.assert_called_once()

    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.abspath')
    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.exists')
    def test_FindEngineRoot_InitialPathDoesntExist_RaisesOSError(self, mock_path_exists, mock_abspath):
        mock_path_exists.return_value = False
        mock_abspath.return_value = mock_engine_root

        with pytest.raises(LyTestToolsFrameworkException):
            abstract_resource_locator._find_engine_root(mock_initial_path)


@mock.patch('builtins.open', mock.MagicMock())
class TestFindProjectJson(object):

    @mock.patch('os.path.isfile', mock.MagicMock(return_value=True))
    @mock.patch('os.path.basename', mock.MagicMock(return_value=mock_project))
    @mock.patch('json.loads', mock.MagicMock(side_effect=[mock_manifest_json_file, mock_project_json_file]))
    def test_FindProjectJson_ManifestJson_ReturnsProjectJson(self):
        project = abstract_resource_locator._find_project_json(mock_engine_root, mock_project)

        assert project == mock_project_json

    @mock.patch('os.path.isfile', mock.MagicMock(side_effect=[False, True]))
    @mock.patch('json.loads', mock.MagicMock())
    def test_FindProjectJson_EngineRoot_ReturnsProjectJson(self):
        project = abstract_resource_locator._find_project_json(mock_engine_root, mock_project)

        assert project == os.path.join(mock_engine_root, mock_project_json)

    @mock.patch('os.path.isfile', mock.MagicMock(side_effect=[False, False]))
    @mock.patch('json.loads', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.file_system.find_ancestor_file')
    def test_FindProjectJson_FindAncestorProject_ReturnsProjectJson(self, mock_find_ancestor):
        mock_find_ancestor_project = 'mock_ancestor_file//project'
        mock_find_ancestor.return_value = mock_find_ancestor_project
        project = abstract_resource_locator._find_project_json(mock_engine_root, mock_project)

        assert project == mock_find_ancestor_project
        assert mock_find_ancestor.called

    @mock.patch('os.path.isfile', mock.MagicMock(side_effect=[False, False]))
    @mock.patch('json.loads', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.file_system.find_ancestor_file')
    def test_FindProjectJson_NoProjectReturned_RaisesOSError(self, mock_find_ancestor):
        mock_find_ancestor.return_value = None

        with pytest.raises(OSError):
            abstract_resource_locator._find_project_json(mock_engine_root, mock_project)


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.abspath',
            mock.MagicMock(return_value=mock_initial_path))
@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=mock_engine_root))
@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_project_json',
            mock.MagicMock(return_value=os.path.join(mock_project, 'project.json')))
class TestAbstractResourceLocator(object):

    def test_Init_HasEngineRoot_SetsAttrs(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)

        assert mock_abstract_resource_locator._build_directory == mock_build_directory
        assert mock_abstract_resource_locator._engine_root == mock_engine_root
        assert mock_abstract_resource_locator._project == mock_project
        assert mock_abstract_resource_locator._project_json == os.path.join(mock_project, 'project.json')

    def test_EngineRoot_IsCalled_ReturnsEngineRoot(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)

        assert mock_abstract_resource_locator.engine_root() == mock_engine_root

    def test_BuildDirectory_IsCalled_ReturnsBuildDirectoryPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)

        assert mock_abstract_resource_locator.build_directory() == mock_build_directory

    def test_Project_IsCalled_ReturnsProjectDir(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)

        assert mock_abstract_resource_locator.project() == os.path.dirname(mock_project_json)

    def test_AssetProcessor_IsCalled_ReturnsAssetProcessorPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.build_directory(), 'AssetProcessor')

        assert mock_abstract_resource_locator.asset_processor() == expected_path

    def test_AssetProcessorBatch_IsCalled_ReturnsAssetProcessorBatchPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.build_directory(), 'AssetProcessorBatch')

        assert mock_abstract_resource_locator.asset_processor_batch() == expected_path

    def test_Editor_IsCalled_ReturnsEditorPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.build_directory(), 'Editor')

        assert mock_abstract_resource_locator.editor() == expected_path

    def test_Cache_IsCalled_ReturnsCachePath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.cache())

        assert mock_abstract_resource_locator.cache() == expected_path

    def test_AssetProcessorConfigFile_IsCalled_ReturnsAssetProcessorConfigFilePath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.engine_root(), 'Registry', 'AssetProcessorPlatformConfig.setreg')

        assert mock_abstract_resource_locator.asset_processor_config_file() == expected_path

    def test_AutoexecFile_IsCalled_ReturnsAutoexecFilePath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator._project,
                                     'autoexec.cfg')

        assert mock_abstract_resource_locator.autoexec_file() == expected_path

    def test_TestResults_IsCalled_TestResultsPath(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        expected_path = os.path.join(mock_abstract_resource_locator.engine_root(), 'TestResults')

        assert mock_abstract_resource_locator.test_results() == expected_path

    @mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.expanduser')
    def test_DevicesFile_IsCalled_ReturnsDevicesFilePath(self, mock_expanduser):
        mock_expanded_path = 'C:/somepath/'
        mock_expanduser.return_value = mock_expanded_path
        expected_path = os.path.join(mock_expanded_path, 'ly_test_tools', 'devices.ini')

        assert abstract_resource_locator.AbstractResourceLocator.devices_file() == expected_path

    def test_PlatformCache_NotImplemented_RaisesNotImplementedError(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        with pytest.raises(NotImplementedError):
            mock_abstract_resource_locator.platform_cache()

    def test_ProjectLog_NotImplemented_RaisesNotImplementedError(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        with pytest.raises(NotImplementedError):
            mock_abstract_resource_locator.project_log()

    def test_ProjectScreenshots_NotImplemented_RaisesNotImplementedError(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        with pytest.raises(NotImplementedError):
            mock_abstract_resource_locator.project_screenshots()

    def test_EditorLog_NotImplemented_RaisesNotImplementedError(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        with pytest.raises(NotImplementedError):
            mock_abstract_resource_locator.editor_log()

    def test_CrashLog_NotImplemented_RaisesNotImplementedError(self):
        mock_abstract_resource_locator = abstract_resource_locator.AbstractResourceLocator(
            mock_build_directory, mock_project)
        with pytest.raises(NotImplementedError):
            mock_abstract_resource_locator.crash_log()
