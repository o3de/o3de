"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Unit Tests for WorkspaceManager class
"""

import datetime
import os
import stat
import unittest.mock as mock
import unittest
import os

import pytest

import ly_test_tools._internal.managers.artifact_manager


TIMESTAMP_FORMAT = '%Y-%m-%dT%H-%M-%S-%f'
DATE = datetime.datetime.now().strftime(TIMESTAMP_FORMAT)
ROOT_LOG_FOLDER = os.path.join("TestResults", DATE, "pytest_results")


class TestSetTestName(unittest.TestCase):
    def setUp(self):
        self.mock_root = ROOT_LOG_FOLDER
        self.mock_artifact_manager = ly_test_tools._internal.managers.artifact_manager.ArtifactManager(
            self.mock_root)

    @mock.patch('ly_test_tools._internal.managers.artifact_manager.ArtifactManager.set_dest_path')
    def test_Init_Called_CallsSetDestPath(self, mock_set_dest_path):
        self.mock_artifact_manager = ly_test_tools._internal.managers.artifact_manager.ArtifactManager(
            self.mock_root)
        assert mock_set_dest_path.called

    @mock.patch('os.makedirs', mock.MagicMock())
    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    def test_SetDestPath_NoPathExists_CreatesPathSetsAttributes(self):
        create_amount = 1
        updated_path = "{}".format(self.mock_artifact_manager.artifact_path, create_amount)
        self.mock_artifact_manager.dest_path = None  # Reset the value to test the function
        self.mock_artifact_manager.set_dest_path(test_name=None, amount=create_amount)

        assert self.mock_artifact_manager.artifact_path == self.mock_root
        assert self.mock_artifact_manager.dest_path == updated_path

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    @mock.patch('os.makedirs')
    def test_SetDestPath_PathExists_NoPathCreatedSetsAttributes(self, mock_makedirs):
        self.mock_artifact_manager.dest_path = None  # Reset the value to test the function
        self.mock_artifact_manager.set_dest_path()

        assert self.mock_artifact_manager.artifact_path == self.mock_root
        assert self.mock_artifact_manager.dest_path == self.mock_artifact_manager.artifact_path
        assert not mock_makedirs.called

    @mock.patch('os.makedirs', mock.MagicMock())
    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    @mock.patch('ly_test_tools._internal.managers.artifact_manager.ArtifactManager._get_collision_handled_filename')
    def test_SetDestPathAmount_IsCalled_CallsGetCollision(self, mock_get_collision):
        create_amount = 2
        test_name = 'dummy'
        mock_get_collision.return_value = test_name

        self.mock_artifact_manager.set_dest_path(test_name, create_amount)
        mock_get_collision.assert_called_once_with(os.path.join(self.mock_root, test_name), create_amount)

    @mock.patch('os.makedirs', mock.MagicMock())
    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    def test_SetDestPathAmount_ValidFilePathExists_NoFilePathCreatedSetsAttributes(self):
        test_name = 'dummy'
        expected_path = os.path.join(self.mock_artifact_manager.artifact_path, test_name)

        self.mock_artifact_manager.set_dest_path(test_name)
        assert self.mock_artifact_manager.dest_path == expected_path


@mock.patch('os.makedirs', mock.MagicMock())
class TestSaveArtifact(unittest.TestCase):

    def setUp(self):
        self.mock_root = ROOT_LOG_FOLDER
        self.mock_artifact_manager = ly_test_tools._internal.managers.artifact_manager.ArtifactManager(self.mock_root)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    @mock.patch('shutil.copytree')
    @mock.patch('os.path.isdir')
    @mock.patch('ly_test_tools.environment.file_system.reduce_file_name_length')
    def test_SaveArtifact_ArtifactNameIsNotNone_CopyTreeCalledCorrectly(self, mock_reducer, mock_path_isdir,
                                                                        mock_copy_tree):
        test_name = 'dummy'
        mock_artifact_name = 'mock_artifact_name'
        updated_path = "{}".format(os.path.join(self.mock_artifact_manager.artifact_path,
                                                test_name))
        mock_path_isdir.return_value = True
        mock_copy_tree.return_value = True
        mock_reducer.return_value = mock_artifact_name[:-5]

        self.mock_artifact_manager.set_dest_path(test_name)
        self.mock_artifact_manager.save_artifact(updated_path, mock_artifact_name)

        assert self.mock_artifact_manager.dest_path == updated_path
        mock_copy_tree.assert_called_once_with(updated_path, os.path.join(updated_path, mock_artifact_name[:-5]),
                                               dirs_exist_ok=True)
        mock_reducer.assert_called_once_with(file_name=mock_artifact_name, max_length=25)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    @mock.patch('shutil.copytree')
    @mock.patch('os.path.isdir')
    @mock.patch('ly_test_tools.environment.file_system.reduce_file_name_length')
    def test_SaveArtifact_ArtifactNameIsNone_CopyTreeCalledCorrectly(self, mock_reducer, mock_path_isdir,
                                                                     mock_copy_tree):
        mock_artifact_name = None
        mock_path_isdir.return_value = True
        mock_copy_tree.return_value = True

        self.mock_artifact_manager.save_artifact(self.mock_artifact_manager.artifact_path, mock_artifact_name)

        assert self.mock_artifact_manager.dest_path == self.mock_artifact_manager.artifact_path
        mock_copy_tree.assert_called_once_with(
            self.mock_artifact_manager.artifact_path,
            os.path.join(self.mock_artifact_manager.artifact_path, 'pytest_results'), dirs_exist_ok=True)
        mock_reducer.assert_not_called()

    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    @mock.patch('os.chmod', mock.MagicMock())
    @mock.patch('shutil.copy')
    @mock.patch('os.path.isdir')
    @mock.patch('ly_test_tools.environment.file_system.reduce_file_name_length')
    def test_SaveArtifact_ArtifactNameIsNotNone_CopyCalledCorrectly(self, mock_reducer, mock_path_isdir,
                                                                    mock_copy):
        mock_artifact_name = 'mock_artifact_name'
        mock_path_isdir.return_value = False
        mock_reducer.return_value = mock_artifact_name[:-5]
        mock_copy.return_value = True

        self.mock_artifact_manager.save_artifact(self.mock_artifact_manager.artifact_path, mock_artifact_name)

        assert self.mock_artifact_manager.dest_path == self.mock_artifact_manager.artifact_path
        mock_copy.assert_called_once_with(
            self.mock_artifact_manager.artifact_path,
            os.path.join(self.mock_artifact_manager.artifact_path, mock_artifact_name[:-5]))
        mock_reducer.assert_called_once_with(file_name=mock_artifact_name, max_length=25)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    @mock.patch('os.chmod', mock.MagicMock())
    @mock.patch('shutil.copy')
    @mock.patch('os.path.isdir')
    @mock.patch('ly_test_tools.environment.file_system.reduce_file_name_length')
    def test_SaveArtifact_ArtifactNameIsNone_CopyCalledCorrectly(self, mock_reducer, mock_path_isdir,
                                                                 mock_copy):
        mock_artifact_name = None
        mock_path_isdir.return_value = False
        mock_copy.return_value = True

        self.mock_artifact_manager.save_artifact(self.mock_artifact_manager.artifact_path, mock_artifact_name)

        assert self.mock_artifact_manager.dest_path == self.mock_artifact_manager.artifact_path
        mock_copy.assert_called_once_with(
            self.mock_artifact_manager.artifact_path,
            os.path.join(self.mock_artifact_manager.artifact_path, 'pytest_results'))
        mock_reducer.assert_not_called()

    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    @mock.patch('shutil.copy', mock.MagicMock())
    @mock.patch('os.chmod')
    @mock.patch('os.path.isdir')
    @mock.patch('ly_test_tools.environment.file_system.reduce_file_name_length')
    def test_SaveArtifact_ArtifactNameIsNotNone_ChmodCalledCorrectly(self, mock_reducer, mock_path_isdir, mock_chmod):
        mock_artifact_name = 'mock_artifact_name'
        mock_path_isdir.return_value = False
        mock_reducer.return_value = mock_artifact_name[:-5]

        self.mock_artifact_manager.save_artifact(self.mock_artifact_manager.artifact_path, mock_artifact_name)
        mock_chmod.assert_called_once_with(
            os.path.join(self.mock_artifact_manager.artifact_path, mock_artifact_name[:-5]),
            stat.S_IWRITE | stat.S_IREAD | stat.S_IEXEC)

        assert self.mock_artifact_manager.dest_path == self.mock_artifact_manager.artifact_path
        mock_reducer.assert_called_once_with(file_name=mock_artifact_name, max_length=25)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    @mock.patch('shutil.copy', mock.MagicMock())
    @mock.patch('os.chmod')
    @mock.patch('os.path.isdir')
    @mock.patch('ly_test_tools.environment.file_system.reduce_file_name_length')
    def test_SaveArtifact_ArtifactNameIsNone_ChmodCalledCorrectly(self, mock_reducer, mock_path_isdir, mock_chmod):
        mock_artifact_name = None
        mock_path_isdir.return_value = False

        self.mock_artifact_manager.save_artifact(self.mock_artifact_manager.artifact_path, mock_artifact_name)
        mock_chmod.assert_called_once_with(
            os.path.join(self.mock_artifact_manager.artifact_path, 'pytest_results'),
            stat.S_IWRITE | stat.S_IREAD | stat.S_IEXEC)

        assert self.mock_artifact_manager.dest_path == self.mock_artifact_manager.artifact_path
        mock_reducer.assert_not_called()

    @mock.patch('os.path.exists', mock.MagicMock(return_value=True))
    @mock.patch('os.path.isdir', mock.MagicMock(return_value=False))
    @mock.patch('shutil.copy', mock.MagicMock())
    @mock.patch('os.chmod', mock.MagicMock())
    @mock.patch('ly_test_tools._internal.managers.artifact_manager.ArtifactManager._get_collision_handled_filename')
    def test_SaveArtifact_DestinationCollides_CallsGetCollisionHandledFilename(self, under_test):
        self.mock_artifact_manager.save_artifact(self.mock_artifact_manager.artifact_path)

        under_test.assert_called_once()


@mock.patch('os.path.exists', mock.MagicMock(return_value=True))
class TestGenerateArtifactFileName(unittest.TestCase):

    def setUp(self):
        self.mock_root = ROOT_LOG_FOLDER
        self.mock_artifact_manager = ly_test_tools._internal.managers.artifact_manager.ArtifactManager(self.mock_root)

    @mock.patch('os.path.isdir')
    @mock.patch('ly_test_tools.environment.file_system.reduce_file_name_length')
    def test_GenerateFileName_HasArtifactName_ReturnsFilePath(self, mock_reducer, mock_path_isdir):
        mock_artifact_name = 'mock_artifact_name'
        mock_path_isdir.return_value = True

        mock_artifact_file = self.mock_artifact_manager.generate_artifact_file_name(mock_artifact_name)

        assert self.mock_artifact_manager.dest_path == self.mock_artifact_manager.artifact_path, (
            'dest_path does not match self.mock_artifact_manager.artifact_path')
        assert mock_artifact_file == os.path.join(self.mock_artifact_manager.artifact_path, mock_artifact_name), (
            'mock_artifact_file does not match expected value from generate_artifact_file_name()')
        mock_reducer.assert_not_called()

    @mock.patch('os.path.isdir')
    @mock.patch('ly_test_tools.environment.file_system.reduce_file_name_length')
    def test_GenerateFileName_ArtifactNameIsNone_ReturnsFilePath(self, mock_reducer, mock_path_isdir):
        mock_artifact_name = None
        mock_path_isdir.return_value = True

        with pytest.raises(ValueError):
            self.mock_artifact_manager.generate_artifact_file_name(mock_artifact_name)
            mock_reducer.assert_not_called()


@mock.patch('os.path.exists', mock.MagicMock(return_value=True))
class TestGatherArtifacts(unittest.TestCase):

    def setUp(self):
        self.mock_root = ROOT_LOG_FOLDER
        self.mock_artifact_manager = ly_test_tools._internal.managers.artifact_manager.ArtifactManager(self.mock_root)

    @mock.patch('shutil.make_archive')
    def test_TestGatherArtifacts_TestNameIsNone_MakeArchiveCalled(self, mock_make_archive):
        mock_destination = 'mock_destination'

        self.mock_artifact_manager.gather_artifacts(mock_destination)
        mock_make_archive.assert_called_once_with(mock_destination, 'zip', self.mock_artifact_manager.artifact_path)

    @mock.patch('ly_test_tools.environment.file_system.sanitize_file_name')
    @mock.patch('shutil.make_archive')
    def test_TestGatherArtifacts_TestNameIsNotNone_MakeArchiveCalled(self, mock_make_archive, mock_sanitize_file_name):
        test_name = 'dummy'
        mock_destination = 'mock_destination'
        mock_sanitize_file_name.return_value = test_name

        self.mock_artifact_manager.set_dest_path(test_name)
        self.mock_artifact_manager.gather_artifacts(mock_destination)

        mock_make_archive.assert_called_once_with(
            mock_destination, 'zip',
            os.path.join(self.mock_artifact_manager.artifact_path, test_name))


class TestGetCollisionHandledFilename(unittest.TestCase):

    def setUp(self):
        self.mock_root = ROOT_LOG_FOLDER
        self.mock_artifact_manager = ly_test_tools._internal.managers.artifact_manager.ArtifactManager(self.mock_root)

    @mock.patch('os.path.exists', mock.MagicMock(return_value=False))
    def test_GetCollisionHandledFilename_FileNameExists_FileNameAppendsNumber(self):
        create_amount = 2
        test_name = 'dummy'
        updated_path = f"{test_name}_1"

        under_test = self.mock_artifact_manager._get_collision_handled_filename(test_name, create_amount)
        assert under_test == updated_path

    def test_GetCollisionHandledFilename_AmountOne_ReturnsParam(self):
        mock_file_path = 'foo'
        amount = 1
        under_test = self.mock_artifact_manager._get_collision_handled_filename(mock_file_path, amount)

        assert under_test == mock_file_path

    @mock.patch('os.path.exists', mock.MagicMock())
    def test_GetCollisionHandledFilename_HasExt_RenameWithExtProperly(self):
        mock_file_path = 'foo'
        mock_file_ext = '.ext'
        mock_file_with_ext = mock_file_path + mock_file_ext
        amount = 2
        under_test = self.mock_artifact_manager._get_collision_handled_filename(mock_file_with_ext, amount)

        assert under_test == f'{mock_file_path}_{amount-1}{mock_file_ext}'

    @mock.patch('os.path.exists', mock.MagicMock())
    def test_GetCollisionHandledFilename_HasNoExt_RenameWithoutExtProperly(self):
        mock_file_path = 'foo'
        amount = 2
        under_test = self.mock_artifact_manager._get_collision_handled_filename(mock_file_path, amount)

        assert under_test == f'{mock_file_path}_{amount-1}'

    @mock.patch('os.path.exists')
    def test_GetCollisionHandledFilename_HasLargeAmountAndNotPathExists_EndsLoopEarly(self, mock_path_exists):
        mock_file_path = 'foo'
        amount = 99
        mock_path_exists.side_effect = [True, True, False]
        under_test = self.mock_artifact_manager._get_collision_handled_filename(mock_file_path, amount)

        # The integer should be the index of False in mock_path_exists
        assert under_test == f'{mock_file_path}_{3}'

    @mock.patch('os.path.exists')
    def test_GetCollisionHandledFilename_HasLargeAmountAndCollides_ReachesMaxAmount(self, mock_path_exists):
        mock_file_path = 'foo'
        amount = 99
        mock_path_exists.return_value = True
        under_test = self.mock_artifact_manager._get_collision_handled_filename(mock_file_path, amount)

        assert under_test == f'{mock_file_path}_{amount-1}'
