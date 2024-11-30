"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit Tests for workspace module
"""

import unittest.mock as mock

import pytest
import unittest

import ly_test_tools._internal.managers.workspace

pytestmark = pytest.mark.SUITE_smoke

mock_initial_path = "mock_initial_path"
mock_engine_root = "mock_engine_root"
mock_dev_path = "mock_dev_path"


@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator.os.path.abspath',
            mock.MagicMock(return_value=mock_initial_path))
@mock.patch('ly_test_tools._internal.managers.abstract_resource_locator._find_engine_root',
            mock.MagicMock(return_value=(mock_engine_root, mock_dev_path)))
class MockedWorkspaceManager(ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager):
    def __init__(
            self,
            resource_locator=mock.MagicMock(),
            project=mock.MagicMock(),
            tmp_path=mock.MagicMock(),
            output_path=mock.MagicMock()
    ):
        super(MockedWorkspaceManager, self).__init__(
            resource_locator=resource_locator,
            project=project,
            tmp_path=tmp_path,
            output_path=output_path
        )

    def setup(self):
        super(MockedWorkspaceManager, self).setup()

    def run_setup_assistant(self):
        pass


class TestWorkspaceManager:

    def test_Init_NoInheritanceAbstractWorkspaceManager_RaisesTypeError(self):
        mock_resource_locator = mock.MagicMock()
        with pytest.raises(TypeError):
            ly_test_tools._internal.managers.workspace.AbstractWorkspaceManager(resource_locator=mock_resource_locator)

    @mock.patch('ly_test_tools._internal.managers.artifact_manager.NullArtifactManager.__init__')
    @mock.patch('tempfile.mkdtemp')
    def test_Init_TmpPathIsNone_TmpPathIsCreated(self, under_test, mock_null_artifact_manager):
        mock_null_artifact_manager.return_value = None
        mock_workspace = MockedWorkspaceManager(tmp_path=None)

        under_test.assert_called_once()

    @mock.patch('os.path.exists', mock.MagicMock())
    @mock.patch('tempfile.mkdtemp', mock.MagicMock())
    def test_Init_LogsPathIsNone_LogsPathIsSetToDefault(self):
        dummy_path = 'mockTestResults'
        mock_resource_locator = mock.MagicMock()
        mock_resource_locator.test_results.return_value = dummy_path
        mock_workspace = MockedWorkspaceManager(resource_locator=mock_resource_locator, output_path=None)

        assert mock_workspace.output_path.startswith(dummy_path)


class TestSetup(unittest.TestCase):
    def setUp(self):
        self.mock_workspace = MockedWorkspaceManager()

    @mock.patch('os.makedirs')
    @mock.patch('os.path.exists')
    def test_Setup_TmpPathExists_CallsMakeDirs(self, mock_path_exists, mock_makedirs):
        self.mock_workspace.tmp_path = 'mock_tmp_path'
        mock_path_exists.side_effect = [False, True, True]  # ArtifactManager.__init__() calls os.path.exists()
        self.mock_workspace._custom_output_path = True

        self.mock_workspace.setup()

        mock_makedirs.assert_called_once_with(self.mock_workspace.tmp_path, exist_ok=True)

    @mock.patch('os.makedirs')
    @mock.patch('os.path.exists')
    def test_Setup_TmpPathNotExists_NoCallsMakeDirs(self, mock_path_exists, mock_makedirs):
        mock_path_exists.return_value = True
        self.mock_workspace.tmp_path = None
        self.mock_workspace._custom_output_path = True

        self.mock_workspace.setup()

        assert not mock_makedirs.called

    @mock.patch('os.makedirs')
    @mock.patch('os.path.exists')
    def test_Setup_LogPathExists_NoCallsMakeDirs(self, mock_path_exists, mock_makedirs):
        mock_path_exists.return_value = True
        self.mock_workspace.tmp_path = None

        self.mock_workspace.setup()

        mock_makedirs.assert_not_called()

    @mock.patch('os.makedirs')
    @mock.patch('os.path.exists')
    def test_Setup_LogPathNotExists_CallsMakeDirs(self, mock_path_exists, mock_makedirs):
        mock_path_exists.return_value = False
        self.mock_workspace.tmp_path = None
        self.mock_workspace.output_path = 'mock_output_path'

        self.mock_workspace.setup()
        mock_makedirs.assert_called_with(self.mock_workspace.output_path, exist_ok=True)
        assert mock_makedirs.call_count == 2  # ArtifactManager.__init__() calls os.path.exists()


class TestTeardown(unittest.TestCase):
    def setUp(self):
        self.mock_workspace = MockedWorkspaceManager()

    @mock.patch('os.chdir', mock.MagicMock())
    @mock.patch('ly_test_tools.environment.file_system.delete')
    def test_Teardown_TmpPathNotNone_DeletesTmpPath(self, mock_delete):
        self.mock_workspace.tmp_path = 'tmp_path'

        self.mock_workspace.teardown()
        mock_delete.assert_called_once_with([self.mock_workspace.tmp_path], True, True)

    @mock.patch('ly_test_tools.environment.file_system.delete', mock.MagicMock())
    @mock.patch('os.chdir')
    def test_Teardown_CwdModified_RevertsCwdToDefault(self, mock_chdir):
        self.mock_workspace.tmp_path = 'tmp_path'

        self.mock_workspace.teardown()
        mock_chdir.assert_called_once_with(self.mock_workspace._original_cwd)


class TestClearCache(unittest.TestCase):
    def setUp(self):
        self.mock_workspace = MockedWorkspaceManager()

    @mock.patch('ly_test_tools.environment.file_system.delete')
    @mock.patch('os.path.exists')
    def test_ClearCache_CacheExists_CacheIsDeleted(self, mock_path_exists, mock_delete):
        mock_path_exists.return_value = True

        self.mock_workspace.clear_cache()
        mock_delete.assert_called_once_with([self.mock_workspace.paths.cache()], True, True)

    @mock.patch('ly_test_tools.environment.file_system.delete')
    @mock.patch('os.path.exists')
    def test_ClearCache_CacheNotExists_CacheNotDeleted(self, mock_path_exists, mock_delete):
        mock_path_exists.return_value = False

        self.mock_workspace.clear_cache()
        mock_delete.assert_not_called()


class TestClearBin(unittest.TestCase):
    def setUp(self):
        self.mock_workspace = MockedWorkspaceManager()

    @mock.patch('ly_test_tools.environment.file_system.delete')
    @mock.patch('os.path.exists')
    def test_ClearBin_BinExists_BinIsDeleted(self, mock_path_exists, mock_delete):
        mock_path_exists.return_value = True

        self.mock_workspace.clear_bin()
        mock_delete.assert_called_once_with([self.mock_workspace.paths.build_directory()], True, True)

    @mock.patch('ly_test_tools.environment.file_system.delete')
    @mock.patch('os.path.exists')
    def test_ClearBin_BinNotExists_BinNotDeleted(self, mock_path_exists, mock_delete):
        mock_path_exists.return_value = False

        self.mock_workspace.clear_bin()
        mock_delete.assert_not_called()
