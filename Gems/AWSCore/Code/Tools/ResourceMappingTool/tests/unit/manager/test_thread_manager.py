"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from unittest import TestCase
from unittest.mock import (MagicMock, patch)

from manager.thread_manager import ThreadManager


class TestThreadManager(TestCase):
    """
    ThreadManager unit test cases
    """
    _mock_thread_pool: MagicMock
    _expected_thread_manager: ThreadManager

    @classmethod
    def setUpClass(cls) -> None:
        thread_pool_patcher: patch = patch("manager.thread_manager.QThreadPool")
        cls._mock_thread_pool = thread_pool_patcher.start()

        cls._expected_thread_manager = ThreadManager()

    @classmethod
    def tearDownClass(cls) -> None:
        patch.stopall()

    def setUp(self) -> None:
        TestThreadManager._mock_thread_pool.reset_mock()

    def test_get_instance_return_same_instance(self) -> None:
        assert ThreadManager.get_instance() is TestThreadManager._expected_thread_manager

    def test_get_instance_raise_exception(self) -> None:
        self.assertRaises(Exception, ThreadManager)

    def test_setup_thread_pool_setup_with_expected_configuration(self) -> None:
        mocked_thread_pool: MagicMock = TestThreadManager._mock_thread_pool.return_value

        TestThreadManager._expected_thread_manager.setup(False)
        mocked_thread_pool.setMaxThreadCount.assert_called_once_with(1)

    def test_setup_thread_pool_skip_setup(self) -> None:
        mocked_thread_pool: MagicMock = TestThreadManager._mock_thread_pool.return_value

        TestThreadManager._expected_thread_manager.setup(True)
        mocked_thread_pool.setMaxThreadCount.asset_not_called()

    def test_start_thread_pool_start_expected_worker(self) -> None:
        mocked_thread_pool: MagicMock = TestThreadManager._mock_thread_pool.return_value
        expected_mocked_worker: MagicMock = MagicMock()

        TestThreadManager._expected_thread_manager.start(expected_mocked_worker)
        mocked_thread_pool.start.assert_called_once_with(expected_mocked_worker)

    def test_clear_thread_pool_clear_gets_called(self) -> None:
        mocked_thread_pool: MagicMock = TestThreadManager._mock_thread_pool.return_value

        TestThreadManager._expected_thread_manager.clear()
        mocked_thread_pool.clear.assert_called_once()
