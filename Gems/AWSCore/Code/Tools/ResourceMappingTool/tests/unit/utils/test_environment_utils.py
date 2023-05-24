"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import platform
from unittest import TestCase
from unittest.mock import (ANY, call, MagicMock, patch)
from utils import environment_utils


class TestEnvironmentUtils(TestCase):
    """
    environment utils unit test cases
    """
    def setUp(self) -> None:
        os_environ_patcher: patch = patch("os.environ")
        self.addCleanup(os_environ_patcher.stop)
        self._mock_os_environ: MagicMock = os_environ_patcher.start()

        os_pathsep_patcher: patch = patch("os.pathsep")
        self.addCleanup(os_pathsep_patcher.stop)
        self._mock_os_pathsep: MagicMock = os_pathsep_patcher.start()

        os_add_dll_directory_patcher: patch = patch("os.add_dll_directory")
        self.addCleanup(os_add_dll_directory_patcher.stop)
        self._mock_os_dll_directory: MagicMock = os_add_dll_directory_patcher.start()

    @patch('os.path.exists')
    @patch('ctypes.CDLL')
    def test_setup_qt_environment_global_flag_is_set(self, mock_os_path_exists, mock_ctype_cdll) -> None:
        mock_os_path_exists.return_value = True
        environment_utils.setup_qt_environment("dummy")
        self._mock_os_environ.copy.assert_called_once()
        self._mock_os_pathsep.join.assert_called_once()
        assert environment_utils.is_qt_linked() is True
        if platform.system() == 'Linux':
            mock_os_path_exists.assert_called()
        elif platform.system() == 'Windows':
            self._mock_os_dll_directory.assert_called_once()

    @patch('os.path.exists')
    @patch('ctypes.CDLL')
    def test_cleanup_qt_environment_global_flag_is_set(self, mock_os_path_exists, mock_ctype_cdll) -> None:
        mock_os_path_exists.return_value = True
        environment_utils.setup_qt_environment("dummy")
        assert environment_utils.is_qt_linked() is True
        environment_utils.cleanup_qt_environment()
        assert environment_utils.is_qt_linked() is False
        if platform.system() == 'Linux':
            mock_os_path_exists.assert_called()
        elif platform.system() == 'Windows':
            self._mock_os_dll_directory.assert_called_once()
