"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from unittest import TestCase
from unittest.mock import (MagicMock, patch)

from manager.controller_manager import ControllerManager


class TestControllerManager(TestCase):
    """
    ControllerManager unit test cases
    """
    _mock_import_resources_controller: MagicMock
    _mock_view_edit_controller: MagicMock
    _expected_controller_manager: ControllerManager

    @classmethod
    def setUpClass(cls) -> None:
        error_controller_patcher: patch = patch("manager.controller_manager.ErrorController")
        cls._mock_error_controller = error_controller_patcher.start()

        import_resources_controller_patcher: patch = patch("manager.controller_manager.ImportResourcesController")
        cls._mock_import_resources_controller = import_resources_controller_patcher.start()

        view_edit_controller_patcher: patch = patch("manager.controller_manager.ViewEditController")
        cls._mock_view_edit_controller = view_edit_controller_patcher.start()

        cls._expected_controller_manager = ControllerManager()

    @classmethod
    def tearDownClass(cls) -> None:
        patch.stopall()

    def setUp(self) -> None:
        TestControllerManager._mock_import_resources_controller.reset_mock()
        TestControllerManager._mock_view_edit_controller.reset_mock()

    def test_get_instance_return_same_instance(self) -> None:
        assert TestControllerManager._expected_controller_manager is ControllerManager.get_instance()

    def test_get_instance_raise_exception(self) -> None:
        self.assertRaises(Exception, ControllerManager)

    def test_setup_each_controller_setup_gets_invoked(self) -> None:
        mocked_view_edit_controller: MagicMock = TestControllerManager._mock_view_edit_controller.return_value
        mocked_import_resources_controller: MagicMock = \
            TestControllerManager._mock_import_resources_controller.return_value

        TestControllerManager._expected_controller_manager.setup(False)
        mocked_view_edit_controller.setup.assert_called_once()
        mocked_import_resources_controller.setup.assert_called_once()
        mocked_import_resources_controller.add_import_resources_sender.connect.assert_called_once_with(
            mocked_view_edit_controller.add_import_resources_receiver)

    def test_setup_error_controller_setup_gets_invoked(self) -> None:
        mocked_error_controller: MagicMock = TestControllerManager._mock_error_controller.return_value

        TestControllerManager._expected_controller_manager.setup(True)
        mocked_error_controller.setup.assert_called_once()
