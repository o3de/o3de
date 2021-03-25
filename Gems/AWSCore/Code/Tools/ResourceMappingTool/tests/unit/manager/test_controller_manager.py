"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

        TestControllerManager._expected_controller_manager.setup()
        mocked_view_edit_controller.setup.assert_called_once()
        mocked_import_resources_controller.setup.assert_called_once()
        mocked_import_resources_controller.add_import_resources.connect.assert_called_once_with(
            mocked_view_edit_controller.add_import_resources)
