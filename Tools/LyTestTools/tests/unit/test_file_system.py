"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import errno
import logging
import os
import stat
import psutil
import subprocess
import sys
import tarfile
import unittest.mock as mock
import unittest
import zipfile

import pytest

from ly_test_tools.environment import file_system

logger = logging.getLogger(__name__)

pytestmark = pytest.mark.SUITE_smoke


class TestCheckFreeSpace(unittest.TestCase):

    def setUp(self):
        # sdiskusage is: (total, used, free, percent)
        self.disk_usage = psutil._common.sdiskusage(100 * file_system.ONE_GIB, 0, 100 * file_system.ONE_GIB, 100)

    def tearDown(self):
        self.disk_usage = None

    @mock.patch('psutil.disk_usage')
    def test_CheckFreeSpace_NoSpace_RaisesIOError(self, mock_disk_usage):

        total = 100 * file_system.ONE_GIB
        used = total
        mock_disk_usage.return_value = psutil._common.sdiskusage(total, used, total - used, used / total)

        with self.assertRaises(IOError):
            file_system.check_free_space('', 1, 'Raise')

    @mock.patch('psutil.disk_usage')
    def test_CheckFreeSpace_EnoughSpace_NoRaise(self, mock_disk_usage):
        total = 100 * file_system.ONE_GIB
        needed = 1
        used = total - needed
        mock_disk_usage.return_value = psutil._common.sdiskusage(total, used, total - used, used / total)

        dest_path = 'dest'
        file_system.check_free_space(dest_path, needed, 'No Raise')
        mock_disk_usage.assert_called_once_with(dest_path)


class TestSafeMakedirs(unittest.TestCase):

    @mock.patch('os.makedirs')
    def test_SafeMakedirs_ValidCreation_Success(self, mock_makedirs):
        file_system.safe_makedirs('dummy')
        mock_makedirs.assert_called_once()

    @mock.patch('os.makedirs')
    def test_SafeMakedirs_RaisedOSErrorErrnoEEXIST_DoesNotPropagate(self, mock_makedirs):
        error = OSError()
        error.errno = errno.EEXIST
        mock_makedirs.side_effect = error
        file_system.safe_makedirs('')

    @mock.patch('os.makedirs')
    def test_SafeMakedirs_RaisedOSErrorNotErrnoEEXIST_Propagates(self, mock_makedirs):
        error = OSError()
        error.errno = errno.EINTR
        mock_makedirs.side_effect = error
        with self.assertRaises(OSError):
            file_system.safe_makedirs('')

    @mock.patch('os.makedirs')
    def test_SafeMakedirs_RootDir_DoesNotPropagate(self, mock_makedirs):
        error = OSError()
        error.errno = errno.EACCES
        if sys.platform == 'win32':
            mock_makedirs.side_effect = error
            file_system.safe_makedirs('C:\\')


class TestGetNewestFileInDir(unittest.TestCase):

    @mock.patch('glob.iglob')
    def test_GetNewestFileInDir_NoResultsFound_ReturnsNone(self, mock_glob):
        mock_glob.return_value.iglob = None
        result = file_system.get_newest_file_in_dir('', '')
        self.assertEqual(result, None)

    @mock.patch('os.path.getctime')
    @mock.patch('glob.iglob')
    def test_GetNewestFileInDir_ThreeResultsTwoExts_CtimeCalledSixTimes(self, mock_glob, mock_ctime):
        mock_glob.return_value = ['fileA.zip', 'fileB.zip', 'fileC.zip']
        mock_ctime.side_effect = range(6)
        file_system.get_newest_file_in_dir('', ['.zip', '.tgz'])

        self.assertEqual(len(mock_ctime.mock_calls), 6)


class TestRemovePathAndExtension(unittest.TestCase):

    def test_GetNewestFileInDir_ValidPath_ReturnsStripped(self):
        result = file_system.remove_path_and_extension(os.path.join("C", "packages", "lumberyard-XXXX.zip"))
        self.assertEqual(result, "lumberyard-XXXX")


class TestUnZip(unittest.TestCase):

    decomp_obj_name = 'zipfile.ZipFile'

    def setUp(self):
        self.file_list = []
        for i in range(25):
            new_src_info = zipfile.ZipInfo('{}.txt'.format(i))
            new_src_info.file_size = i
            self.file_list.append(new_src_info)

        self.mock_handle = mock.MagicMock()
        self.mock_handle.infolist.return_value = self.file_list
        self.mock_handle.__enter__.return_value = self.mock_handle

        self.mock_decomp = mock.MagicMock()
        self.mock_decomp.return_value = self.mock_handle

        self.src_path = 'src.zip'
        self.dest_path = 'dest'
        self.exists = False

    def tearDown(self):
        self.mock_handle = None
        self.mock_decomp = None

    def call_decomp(self, dest, src, force=True, allow_exists=False):
        return file_system.unzip(dest, src, force, allow_exists)


    @mock.patch('os.path.exists')
    @mock.patch('ly_test_tools.environment.file_system.check_free_space')
    @mock.patch('os.path.join')
    def test_Unzip_NotEnoughSpaceInDestination_FailsWithErrorMessage(self, mock_join, mock_check_free, mock_exists):

        mock_exists.return_value = self.exists
        total_size = sum(info.file_size for info in self.file_list)

        with mock.patch(self.decomp_obj_name, self.mock_decomp):
            self.call_decomp(self.dest_path, self.src_path)

        mock_check_free.assert_called_once_with(self.dest_path,
                                                total_size + file_system.ONE_GIB,
                                                'Not enough space to safely extract: ')

    @mock.patch('ly_test_tools.environment.file_system.check_free_space')
    @mock.patch('os.path.join')
    def test_Unzip_ReleaseBuild_ReturnsCorrectPath(self, mock_join, mock_check_free):

        build_name = 'lumberyard-1.2.0.3-54321-pc-1234'
        expected_path = self.dest_path + '\\' + build_name
        mock_join.return_value = expected_path

        path = ''
        self.src_path = r'C:\packages\lumberyard-1.2.0.3-54321-pc-1234.zip'
        with mock.patch(self.decomp_obj_name, self.mock_decomp):
            path = self.call_decomp(self.dest_path, self.src_path)

        self.assertEqual(path, expected_path)

    @mock.patch('ly_test_tools.environment.file_system.logger')
    @mock.patch('os.path.exists')
    @mock.patch('ly_test_tools.environment.file_system.check_free_space')
    @mock.patch('os.path.join')
    def test_Unzip_BuildDirExistsForceAndAllowExistsNotSet_CRITICALLogged(self, mock_join, mock_check_free,
                                                                          mock_exists, mock_log):

        force = False
        allow_exists = False
        self.exists = True
        mock_exists.return_value = self.exists
        level = logging.getLevelName("CRITICAL")

        with mock.patch(self.decomp_obj_name, self.mock_decomp):
            path = self.call_decomp(self.dest_path, self.src_path, force)

        mock_log.log.assert_called_with(level, 'Found existing {}.  Will not overwrite.'.format(path))

    @mock.patch('ly_test_tools.environment.file_system.logger')
    @mock.patch('os.path.exists')
    @mock.patch('ly_test_tools.environment.file_system.check_free_space', mock.MagicMock())
    @mock.patch('os.path.join', mock.MagicMock())
    def test_Unzip_AllowExistsSet_INFOLogged(self, mock_exists, mock_log):

        force = False
        allow_exists = True
        self.exists = True
        mock_exists.return_value = self.exists
        level = logging.getLevelName("INFO")

        with mock.patch(self.decomp_obj_name, self.mock_decomp):
            path = self.call_decomp(self.dest_path, self.src_path, force, allow_exists)

        mock_log.log.assert_called_with(level, 'Found existing {}.  Will not overwrite.'.format(path))

    @mock.patch('ly_test_tools.environment.file_system.logger')
    @mock.patch('os.path.exists')
    @mock.patch('ly_test_tools.environment.file_system.check_free_space', mock.MagicMock())
    @mock.patch('os.path.join', mock.MagicMock())
    def test_Unzip_BuildDirExistsForceSetTrue_INFOLogged(self, mock_exists, mock_log):

        path = ''
        self.exists = True
        mock_exists.return_value = self.exists

        with mock.patch(self.decomp_obj_name, self.mock_decomp):
            path = self.call_decomp(self.dest_path, self.src_path)

        mock_log.info.assert_called_once()


class TestUnTgz(unittest.TestCase):

    decomp_obj_name = 'tarfile.open'

    def setUp(self):
        self.file_list = []
        for i in range(25):
            new_src_info = tarfile.TarInfo('{}.txt'.format(i))
            new_src_info.size = i
            self.file_list.append(new_src_info)

        self.mock_handle = mock.MagicMock()
        self.mock_handle.__iter__.return_value = self.file_list
        self.mock_handle.__enter__.return_value = self.mock_handle

        self.mock_decomp = mock.MagicMock()
        self.mock_decomp.return_value = self.mock_handle

        self.src_path = 'src.tgz'
        self.dest_path = 'dest'

        # os.stat_result is (mode, inode, device, hard links, owner uid, size, atime, mtime, ctime)
        self.src_stat = os.stat_result((0, 0, 0, 0, 0, 0, file_system.ONE_GIB, 0, 0, 0))

    def tearDown(self):
        self.mock_decomp = None
        self.mock_handle = None

    def call_decomp(self, dest, src, exact=False, force=True, allow_exists=False):
        return file_system.untgz(dest, src, exact, force, allow_exists)

    @mock.patch('ly_test_tools.environment.file_system.check_free_space')
    @mock.patch('os.path.join')
    @mock.patch('os.stat')
    def test_Untgz_ReleaseBuild_ReturnsCorrectPath(self, mock_stat, mock_join, mock_check_free):

        build_name = 'lumberyard-1.2.0.3-54321-pc-1234'
        expected_path = self.dest_path + '\\' + build_name
        mock_join.return_value = expected_path
        mock_stat.return_value = self.src_stat

        path = ''
        self.src_path = r'C:\packages\lumberyard-1.2.0.3-54321-pc-1234.zip'
        with mock.patch(self.decomp_obj_name, self.mock_decomp):
            path = self.call_decomp(self.dest_path, self.src_path)

        self.assertEqual(path, expected_path)

    @mock.patch('ly_test_tools.environment.file_system.logger')
    @mock.patch('os.path.exists')
    @mock.patch('ly_test_tools.environment.file_system.check_free_space', mock.MagicMock())
    @mock.patch('os.path.join', mock.MagicMock())
    @mock.patch('os.stat')
    def test_Untgz_BuildDirExistsForceAndAllowExistsNotSet_CRITICALLogged(self, mock_stat, mock_exists, mock_log):

        force = False
        mock_exists.return_value = True
        mock_stat.return_value = self.src_stat

        with mock.patch(self.decomp_obj_name, self.mock_decomp):
            path = self.call_decomp(self.dest_path, self.src_path, False, force)

        level = logging.getLevelName("CRITICAL")
        mock_log.log.assert_called_with(level, 'Found existing {}.  Will not overwrite.'.format(path))

    @mock.patch('ly_test_tools.environment.file_system.logger')
    @mock.patch('os.path.exists')
    @mock.patch('ly_test_tools.environment.file_system.check_free_space', mock.MagicMock())
    @mock.patch('os.path.join', mock.MagicMock())
    @mock.patch('os.stat')
    def test_Untgz_AllowExiststSet_INFOLogged(self, mock_stat, mock_exists, mock_log):

        allow_exists = True
        force = False
        mock_exists.return_value = True
        mock_stat.return_value = self.src_stat
        level = logging.getLevelName("INFO")

        with mock.patch(self.decomp_obj_name, self.mock_decomp):
            path = self.call_decomp(self.dest_path, self.src_path, False, force, allow_exists)

        mock_log.log.assert_called_with(level, 'Found existing {}.  Will not overwrite.'.format(path))

    @mock.patch('ly_test_tools.environment.file_system.logger')
    @mock.patch('os.path.exists')
    @mock.patch('ly_test_tools.environment.file_system.check_free_space')
    @mock.patch('os.path.join')
    @mock.patch('os.stat')
    def test_Untgz_BuildDirExistsForceSetTrue_INFOLogged(self, mock_stat, mock_join,
                                                                mock_check_free, mock_exists, mock_log):

        path = ''
        mock_exists.return_value = True
        mock_stat.return_value = self.src_stat

        with mock.patch(self.decomp_obj_name, self.mock_decomp):
            path = self.call_decomp(self.dest_path, self.src_path)

        mock_log.info.assert_called_once()


class TestChangePermissions(unittest.TestCase):

    def setUp(self):
        # Create a mock of a os.walk return iterable.
        self.root = 'root'
        self.dirs = ['dir1', 'dir2']
        self.files = ['file1', 'file2']
        self.walk_iter = iter([(self.root, self.dirs, self.files)])

    def tearDown(self):
        self.root = None
        self.dirs = None
        self.files = None
        self.walk_iter = None

    @mock.patch('os.walk')
    @mock.patch('os.chmod')
    def test_ChangePermissions_DefaultValues_ChmodCalledCorrectly(self, mock_chmod, mock_walk):
        os.walk.return_value = self.walk_iter
        file_system.change_permissions('.', 0o777)
        self.assertEqual(mock_chmod.mock_calls, [mock.call(os.path.join(self.root, self.dirs[0]), 0o777),
                                                 mock.call(os.path.join(self.root, self.dirs[1]), 0o777),
                                                 mock.call(os.path.join(self.root, self.files[0]), 0o777),
                                                 mock.call(os.path.join(self.root, self.files[1]), 0o777)])

    @mock.patch('os.walk')
    @mock.patch('os.chmod')
    def test_ChangePermissions_DefaultValues_ReturnsTrueOnSuccess(self, mock_chmod, mock_walk):
        os.walk.return_value = self.walk_iter
        self.assertEqual(file_system.change_permissions('.', 0o777), True)

    @mock.patch('os.walk')
    @mock.patch('os.chmod')
    def test_ChangePermissions_OSErrorRaised_ReturnsFalse(self, mock_chmod, mock_walk):
        os.walk.return_value = self.walk_iter
        os.chmod.side_effect = OSError()
        self.assertEqual(file_system.change_permissions('.', 0o777), False)


class MockStatResult():
    def __init__(self, st_mode):
        self.st_mode = st_mode


class TestUnlockFile(unittest.TestCase):

    def setUp(self):
        self.file_name = 'file'

    @mock.patch('os.stat')
    @mock.patch('os.chmod')
    @mock.patch('os.access')
    def test_UnlockFile_WriteLocked_UnlockFile(self, mock_access, mock_chmod, mock_stat):
        mock_access.return_value = False
        os.stat.return_value = MockStatResult(stat.S_IREAD)

        success = file_system.unlock_file(self.file_name)
        mock_chmod.assert_called_once_with(self.file_name, stat.S_IREAD | stat.S_IWRITE)

        self.assertTrue(success)

    @mock.patch('os.stat')
    @mock.patch('os.chmod')
    @mock.patch('os.access')
    def test_UnlockFile_AlreadyUnlocked_LogAlreadyUnlocked(self, mock_access, mock_chmod, mock_stat):
        mock_access.return_value = True
        os.stat.return_value = MockStatResult(stat.S_IREAD | stat.S_IWRITE)

        success = file_system.unlock_file(self.file_name)

        self.assertFalse(success)


class TestLockFile(unittest.TestCase):

    def setUp(self):
        self.file_name = 'file'

    @mock.patch('os.stat')
    @mock.patch('os.chmod')
    @mock.patch('os.access')
    def test_LockFile_UnlockedFile_FileLockedSuccessReturnsTrue(self, mock_access, mock_chmod, mock_stat):
        mock_access.return_value = True
        os.stat.return_value = MockStatResult(stat.S_IREAD | stat.S_IWRITE)

        success = file_system.lock_file(self.file_name)
        mock_chmod.assert_called_once_with(self.file_name, stat.S_IREAD)

        self.assertTrue(success)

    @mock.patch('os.stat')
    @mock.patch('os.chmod')
    @mock.patch('os.access')
    def test_LockFile_AlreadyLocked_FileLockedFailedReturnsFalse(self, mock_access, mock_chmod, mock_stat):
        mock_access.return_value = False
        os.stat.return_value = MockStatResult(stat.S_IREAD)

        success = file_system.lock_file(self.file_name)

        self.assertFalse(success)


class TestRemoveSymlinks(unittest.TestCase):

    def setUp(self):
        # Create a mock of a os.walk return iterable.
        self.root = 'root'
        self.dirs = ['dir1', 'dir2']
        self.files = ['file1', 'file2']
        self.walk_iter = iter([(self.root, self.dirs, self.files)])

    @mock.patch('os.walk')
    @mock.patch('os.rmdir')
    def test_RemoveSymlinks_DefaultValues_RmdirCalledCorrectly(self, mock_rmdir, mock_walk):
        os.walk.return_value = self.walk_iter

        file_system.remove_symlinks('.')

        self.assertEqual(mock_rmdir.mock_calls, [mock.call(os.path.join(self.root, self.dirs[0])),
                                                 mock.call(os.path.join(self.root, self.dirs[1]))])

    @mock.patch('os.walk')
    @mock.patch('os.rmdir')
    def test_RemoveSymlinks_OSErrnoEEXISTRaised_RaiseOSError(self, mock_rmdir, mock_walk):
        os.walk.return_value = self.walk_iter
        error = OSError()
        error.errno = errno.EEXIST
        mock_rmdir.side_effect = error

        with self.assertRaises(OSError):
            file_system.remove_symlinks('.')


class TestDelete(unittest.TestCase):

    def setUp(self):
        self.path_list = ['file1', 'file2', 'dir1', 'dir2']

    def tearDown(self):
        self.path_list = None

    @mock.patch('os.path.isdir')
    @mock.patch('os.path.isfile')
    @mock.patch('ly_test_tools.environment.file_system.change_permissions', mock.MagicMock())
    @mock.patch('shutil.rmtree', mock.MagicMock())
    @mock.patch('os.remove', mock.MagicMock())
    def test_Delete_StringArg_ConvertsToList(self, mock_isfile, mock_isdir):
        mock_file_str = 'foo'
        mock_isdir.return_value = False
        mock_isfile.return_value = False
        file_system.delete(mock_file_str, del_files=True, del_dirs=True)

        mock_isfile.assert_called_once_with(mock_file_str)
        mock_isdir.assert_called_once_with(mock_file_str)

    @mock.patch('ly_test_tools.environment.file_system.change_permissions')
    @mock.patch('shutil.rmtree')
    @mock.patch('os.remove')
    @mock.patch('os.path.isdir')
    @mock.patch('os.path.isfile')
    def test_ChangePermissions_OSErrorRaised_ReturnsZero(self, mock_isfile, mock_isdir, mock_remove, mock_rmtree, mock_chper):
        mock_rmtree.side_effect = OSError()
        self.assertEqual(file_system.delete(self.path_list, del_files=True, del_dirs=True), False)

    @mock.patch('ly_test_tools.environment.file_system.change_permissions')
    @mock.patch('shutil.rmtree')
    @mock.patch('os.remove')
    @mock.patch('os.path.isdir')
    @mock.patch('os.path.isfile')
    def test_ChangePermissions_DefaultValues_ReturnsLenOfList(self,
                                                              mock_isfile, mock_isdir, mock_remove, mock_rmtree, mock_chper):
        self.assertEqual(file_system.delete(self.path_list, del_files=True, del_dirs=True), True)

    @mock.patch('os.chmod')
    @mock.patch('ly_test_tools.environment.file_system.change_permissions')
    @mock.patch('shutil.rmtree')
    @mock.patch('os.remove')
    @mock.patch('os.path.isdir')
    @mock.patch('os.path.isfile')
    def test_ChangePermissions_DirsFalse_RMTreeNotCalled(self,
                                                         mock_isfile, mock_isdir, mock_remove, mock_rmtree, mock_chper,
                                                         mock_chmod):
        file_system.delete(self.path_list, del_files=True, del_dirs=False)
        self.assertEqual(mock_rmtree.called, False)
        self.assertEqual(mock_chmod.called, True)
        self.assertEqual(mock_remove.called, True)

    @mock.patch('ly_test_tools.environment.file_system.change_permissions')
    @mock.patch('shutil.rmtree')
    @mock.patch('os.remove')
    @mock.patch('os.path.isdir')
    @mock.patch('os.path.isfile')
    def test_ChangePermissions_NoDirs_RMTreeNotCalled(self,
                                                      mock_isfile, mock_isdir, mock_remove, mock_rmtree, mock_chper):
        mock_isdir.return_value = False
        file_system.delete(self.path_list, del_files=False, del_dirs=True)
        self.assertEqual(mock_rmtree.called, False)

    @mock.patch('ly_test_tools.environment.file_system.change_permissions')
    @mock.patch('shutil.rmtree')
    @mock.patch('os.remove')
    @mock.patch('os.path.isdir')
    @mock.patch('os.path.isfile')
    def test_ChangePermissions_FilesFalse_RemoveNotCalled(self,
                                                          mock_isfile, mock_isdir, mock_remove, mock_rmtree, mock_chper):
        file_system.delete(self.path_list, del_files=False, del_dirs=True)
        self.assertEqual(mock_rmtree.called, True)
        self.assertEqual(mock_remove.called, False)

    @mock.patch('ly_test_tools.environment.file_system.change_permissions')
    @mock.patch('shutil.rmtree')
    @mock.patch('os.remove')
    @mock.patch('os.path.isdir')
    @mock.patch('os.path.isfile')
    def test_ChangePermissions_NoFiles_RemoveNotCalled(self,
                                                       mock_isfile, mock_isdir, mock_remove, mock_rmtree, mock_chper):
        mock_isfile.return_value = False
        file_system.delete(self.path_list, del_files=True, del_dirs=False)
        self.assertEqual(mock_remove.called, False)


class TestRename(unittest.TestCase):

    def setUp(self):
        self.file1 = 'file1'
        self.file2 = 'file2'

    def tearDown(self):
        self.file1 = None
        self.file2 = None

    @mock.patch('os.path')
    @mock.patch('os.chmod')
    @mock.patch('os.rename')
    def test_Rename_TwoFiles_SuccessfulRename(self, mock_rename, mock_chmod, mock_path):
        mock_path.exists.side_effect = [True, False]

        self.assertTrue(file_system.rename(self.file1, self.file2))
        self.assertTrue(mock_rename.called)

    @mock.patch('os.rename')
    def test_Rename_SourceDoesNotExist_ErrorReported(self, mock_rename):
        with self.assertLogs('ly_test_tools.environment.file_system', 'ERROR') as captured_logs:
            with mock.patch('os.path') as mock_path:
                mock_path.exists.return_value = False
                self.assertFalse(file_system.rename(self.file1, self.file2))
        self.assertTrue(mock_path.exists.called)
        self.assertIn("No file located at:", captured_logs.records[0].getMessage())
        self.assertFalse(mock_rename.called)

    @mock.patch('os.rename')
    def test_Rename_DestinationExists_ErrorReported(self, mock_rename):
        with self.assertLogs('ly_test_tools.environment.file_system', 'ERROR') as captured_logs:
            with mock.patch('os.path') as mock_path:
                mock_path.exists.return_value = True
                self.assertFalse(file_system.rename(self.file1, self.file2))
        self.assertEqual(2, mock_path.exists.call_count)
        self.assertIn("File already exists at:", captured_logs.records[0].getMessage())
        self.assertFalse(mock_rename.called)

    @mock.patch('os.rename')
    @mock.patch('os.chmod')
    def test_Rename_PermissionsError_ErrorReported(self, mock_chmod, mock_rename):
        with self.assertLogs('ly_test_tools.environment.file_system', 'ERROR') as captured_logs:
            with mock.patch('os.path') as mock_path:
                mock_path.exists.side_effect = [True, False]
                mock_path.isfile.return_value = True
                mock_chmod.side_effect = OSError()
                self.assertFalse(file_system.rename(self.file1, self.file2))
        self.assertEqual(2, mock_path.exists.call_count)
        self.assertIn('Could not rename', captured_logs.records[0].getMessage())
        self.assertEqual(mock_rename.called, False)


class TestDeleteOldest(unittest.TestCase):

    def setUp(self):
        self.path_list = ['file1', 'file2', 'dir1', 'dir2']
        self.age_list = [4, 3, 2, 1]

    def tearDown(self):
        self.path_list = None
        self.age_list = None

    @mock.patch('glob.iglob')
    @mock.patch('os.path.getctime')
    @mock.patch('ly_test_tools.environment.file_system.delete')
    def test_DeleteOldest_DefaultValuesKeepAll_DeleteCalledWithEmptyList(self, mock_delete, mock_ctime, mock_glob):
        mock_glob.return_value = self.path_list
        mock_ctime.side_effect = self.age_list
        # Nothing will be deleted because it's keeping everything.
        file_system.delete_oldest('', len(self.path_list), del_files=True, del_dirs=False)
        mock_delete.assert_called_once_with([], True, False)

    @mock.patch('glob.iglob')
    @mock.patch('os.path.getctime')
    @mock.patch('ly_test_tools.environment.file_system.delete')
    def test_DeleteOldest_DefaultValuesKeepNone_DeleteCalledWithList(self, mock_delete, mock_ctime, mock_glob):
        mock_glob.return_value = self.path_list
        mock_ctime.side_effect = self.age_list
        # Everything will be deleted because it's keeping nothing.
        file_system.delete_oldest('', 0, del_files=True, del_dirs=False)
        mock_delete.assert_called_once_with(self.path_list, True, False)

    @mock.patch('glob.iglob')
    @mock.patch('os.path.getctime')
    @mock.patch('ly_test_tools.environment.file_system.delete')
    def test_DeleteOldest_DefaultValuesKeepOne_DeleteCalledWithoutNewest(self, mock_delete, mock_ctime, mock_glob):
        mock_glob.return_value = self.path_list
        mock_ctime.side_effect = self.age_list
        file_system.delete_oldest('', 1, del_files=True, del_dirs=False)
        self.path_list.pop(0)
        mock_delete.assert_called_once_with(self.path_list, True, False)

    @mock.patch('glob.iglob')
    @mock.patch('os.path.getctime')
    @mock.patch('ly_test_tools.environment.file_system.delete')
    def test_DeleteOldest_UnsortedListDeleteOldest_DeleteCalledWithOldest(self, mock_delete, mock_ctime, mock_glob):
        mock_glob.return_value = ['newest', 'old', 'newer']
        mock_ctime.side_effect = [100, 0, 50]
        file_system.delete_oldest('', 2, del_files=True, del_dirs=False)
        mock_delete.assert_called_once_with(['old'], True, False)


class TestMakeJunction(unittest.TestCase):

    def test_MakeJunction_Nondir_RaisesIOError(self):
        with self.assertRaises(IOError):
            file_system.make_junction('', '')

    @mock.patch('os.path.isdir')
    @mock.patch('sys.platform', 'linux2')
    def test_MakeJunction_NoSupportPlatform_RaisesIOError(self, mock_isdir):
        mock_isdir.return_value = True
        with self.assertRaises(IOError):
            file_system.make_junction('', '')

    @mock.patch('subprocess.check_call')
    @mock.patch('os.path.isdir')
    @mock.patch('sys.platform', 'win32')
    def test_MakeJunction_Win32_SubprocessFails_RaiseSubprocessError(self, mock_isdir, mock_sub_call):
        mock_isdir.return_value = True
        mock_sub_call.side_effect = subprocess.CalledProcessError(1, 'cmd', 'output')

        with self.assertRaises(subprocess.CalledProcessError):
            file_system.make_junction('', '')

    @mock.patch('subprocess.check_output')
    @mock.patch('os.path.isdir')
    @mock.patch('sys.platform', 'darwin')
    def test_MakeJunction_Darwin_SubprocessFails_RaiseSubprocessError(self, mock_isdir, mock_sub_call):
        mock_isdir.return_value = True
        mock_sub_call.side_effect = subprocess.CalledProcessError(1, 'cmd', 'output')

        with self.assertRaises(subprocess.CalledProcessError):
            file_system.make_junction('', '')

    @mock.patch('subprocess.check_output')
    @mock.patch('os.path.isdir')
    @mock.patch('sys.platform', 'win32')
    def test_MakeJunction_Win32_SubprocessCall_Calls(self, mock_isdir, mock_sub_call):
        mock_isdir.return_value = True
        src = 'source'
        dest = 'destination'

        file_system.make_junction(dest, src)

        mock_sub_call.assert_called_once_with(['mklink', '/J', dest, src], shell=True)

    @mock.patch('subprocess.check_output')
    @mock.patch('os.path.isdir')
    @mock.patch('sys.platform', 'darwin')
    def test_MakeJunction_Darwin_SubprocessCall_Calls(self, mock_isdir, mock_sub_call):
        mock_isdir.return_value = True
        src = 'source'
        dest = 'destination'

        file_system.make_junction(dest, src)

        mock_sub_call.assert_called_once_with(['ln', dest, src])


class TestFileBackup(unittest.TestCase):

    def setUp(self):
        self._dummy_dir = os.path.join('somewhere', 'something')
        self._dummy_file = 'dummy.txt'
        self._dummy_backup_file = os.path.join(self._dummy_dir, '{}.bak'.format(self._dummy_file))

    @mock.patch('shutil.copy2')
    @mock.patch('os.path.exists')
    @mock.patch('os.path.isdir')
    def test_BackupSettings_SourceExists_BackupCreated(self, mock_path_isdir, mock_backup_exists, mock_copy):
        mock_path_isdir.return_value = True
        mock_backup_exists.side_effect = [True, False]

        file_system.create_backup(self._dummy_file, self._dummy_dir)

        mock_copy.assert_called_with(self._dummy_file, self._dummy_backup_file)

    @mock.patch('ly_test_tools.environment.file_system.logger.warning')
    @mock.patch('shutil.copy2')
    @mock.patch('os.path.exists')
    @mock.patch('os.path.isdir')
    def test_BackupSettings_BackupExists_WarningLogged(self, mock_path_isdir, mock_backup_exists, mock_copy, mock_logger_warning):
        mock_path_isdir.return_value = True
        mock_backup_exists.return_value = True

        file_system.create_backup(self._dummy_file, self._dummy_dir)

        mock_copy.assert_called_with(self._dummy_file, self._dummy_backup_file)
        mock_logger_warning.assert_called_once()

    @mock.patch('ly_test_tools.environment.file_system.logger.warning')
    @mock.patch('shutil.copy2')
    @mock.patch('os.path.exists')
    @mock.patch('os.path.isdir')
    def test_BackupSettings_SourceNotExists_WarningLogged(self, mock_path_isdir, mock_backup_exists, mock_copy, mock_logger_warning):
        mock_path_isdir.return_value = True
        mock_backup_exists.return_value = False

        file_system.create_backup(self._dummy_file, self._dummy_dir)

        mock_copy.assert_not_called()
        mock_logger_warning.assert_called_once()

    @mock.patch('ly_test_tools.environment.file_system.logger.warning')
    @mock.patch('shutil.copy2')
    @mock.patch('os.path.exists')
    @mock.patch('os.path.isdir')
    def test_BackupSettings_CannotCopy_WarningLogged(self, mock_path_isdir, mock_backup_exists, mock_copy, mock_logger_warning):
        mock_path_isdir.return_value = True
        mock_backup_exists.side_effect = [True, False]
        mock_copy.side_effect = Exception('some error')

        file_system.create_backup(self._dummy_file, self._dummy_dir)

        mock_copy.assert_called_with(self._dummy_file, self._dummy_backup_file)
        mock_logger_warning.assert_called_once()

    @mock.patch('ly_test_tools.environment.file_system.logger.error')
    @mock.patch('os.path.exists')
    @mock.patch('os.path.isdir')
    def test_BackupSettings_InvalidDir_ErrorLogged(self, mock_path_isdir, mock_backup_exists, mock_logger_error):
        mock_path_isdir.return_value = False
        mock_backup_exists.return_value = False

        file_system.create_backup(self._dummy_file, None)

        mock_logger_error.assert_called_once()


class TestFileBackupRestore(unittest.TestCase):

    def setUp(self):
        self._dummy_dir = os.path.join('somewhere', 'something')
        self._dummy_file = 'dummy.txt'
        self._dummy_backup_file = os.path.join(self._dummy_dir, '{}.bak'.format(self._dummy_file))

    @mock.patch('shutil.copy2')
    @mock.patch('os.path.exists')
    @mock.patch('os.path.isdir')
    def test_RestoreSettings_BackupRestore_Success(self, mock_path_isdir, mock_exists, mock_copy):
        mock_path_isdir.return_value = True
        mock_exists.return_value = True
        file_system.restore_backup(self._dummy_file, self._dummy_dir)

        mock_copy.assert_called_with(self._dummy_backup_file, self._dummy_file)

    @mock.patch('ly_test_tools.environment.file_system.logger.warning')
    @mock.patch('shutil.copy2')
    @mock.patch('os.path.exists')
    @mock.patch('os.path.isdir')
    def test_RestoreSettings_CannotCopy_WarningLogged(self, mock_path_isdir, mock_exists, mock_copy, mock_logger_warning):
        mock_path_isdir.return_value = True
        mock_exists.return_value = True
        mock_copy.side_effect = Exception('some error')

        file_system.restore_backup(self._dummy_file, self._dummy_dir)

        mock_copy.assert_called_with(self._dummy_backup_file, self._dummy_file)
        mock_logger_warning.assert_called_once()

    @mock.patch('ly_test_tools.environment.file_system.logger.warning')
    @mock.patch('shutil.copy2')
    @mock.patch('os.path.exists')
    @mock.patch('os.path.isdir')
    def test_RestoreSettings_BackupNotExists_WarningLogged(self, mock_path_isdir, mock_exists, mock_copy, mock_logger_warning):
        mock_path_isdir.return_value = True
        mock_exists.return_value = False

        file_system.restore_backup(self._dummy_file, self._dummy_dir)

        mock_copy.assert_not_called()
        mock_logger_warning.assert_called_once()

    @mock.patch('ly_test_tools.environment.file_system.logger.error')
    def test_RestoreSettings_InvalidDir_ErrorLogged(self, mock_logger_error):
        file_system.restore_backup(self._dummy_file, None)

        mock_logger_error.assert_called_once()

    @mock.patch('ly_test_tools.environment.file_system.logger.error')
    @mock.patch('os.path.isdir')
    def test_RestoreSettings_InvalidDir_ErrorLogged(self, mock_path_isdir, mock_logger_error):
        mock_path_isdir.return_value = False
        file_system.restore_backup(self._dummy_file, self._dummy_dir)

        mock_logger_error.assert_called_once()


class TestReduceFileName(unittest.TestCase):

    def test_Reduce_LongString_ReturnsReducedString(self):
        target_name = 'really_long_string_that_needs_reduction'  # len(mock_file_name) == 39
        max_length = 25

        under_test = file_system.reduce_file_name_length(target_name, max_length)

        assert len(under_test) == max_length

    def test_Reduce_ShortString_ReturnsSameString(self):
        target_name = 'less_than_max'  # len(mock_file_name) == 13
        max_length = 25

        under_test = file_system.reduce_file_name_length(target_name, max_length)

        assert under_test == target_name

    def test_Reduce_NoString_RaisesTypeError(self):
        with pytest.raises(TypeError):
            file_system.reduce_file_name_length(max_length=25)

    def test_Reduce_NoMaxLength_RaisesTypeError(self):
        target_name = 'raises_type_error'

        with pytest.raises(TypeError):
            file_system.reduce_file_name_length(file_name=target_name)

class TestFindAncestorFile(unittest.TestCase):

    @mock.patch('os.path.exists', mock.MagicMock(side_effect=[False, False, True, True]))
    def test_Find_OneLevel_ReturnsPath(self):
        mock_file = 'mock_file.txt'
        mock_start_path = os.path.join('foo1', 'foo2', 'foo3')
        expected = os.path.abspath(os.path.join('foo1', mock_file))

        actual = file_system.find_ancestor_file(mock_file, mock_start_path)
        assert actual == expected