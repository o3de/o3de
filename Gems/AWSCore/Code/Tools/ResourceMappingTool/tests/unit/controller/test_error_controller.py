"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from unittest import TestCase
from unittest.mock import (call, MagicMock, patch)

from controller.error_controller import ErrorController

class TestErrorController(TestCase):
    """
    ErrorController unit test cases
    """
    def setUp(self) -> None:
        view_manager_patcher: patch = patch("controller.error_controller.ViewManager")
        self.addCleanup(view_manager_patcher.stop)
        self._mock_view_manager: MagicMock = view_manager_patcher.start()

        self._mocked_view_manager: MagicMock = self._mock_view_manager.get_instance.return_value
        self._mocked_error_page: MagicMock = self._mocked_view_manager.get_error_page.return_value

        self._test_error_controller: ErrorController = ErrorController()
        self._test_error_controller.setup()

    def test_setup_with_expected_behavior_connected(self) -> None:
        self._mocked_error_page.ok_button.clicked.connect.assert_called_once_with(self._test_error_controller._ok)
