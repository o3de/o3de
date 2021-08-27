"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2.QtCore import (QCoreApplication, QObject)

from manager.view_manager import ViewManager
from view.error_page import ErrorPage


class ErrorController(QObject):
    """
    ErrorPage Controller
    """
    def __init__(self) -> None:
        super(ErrorController, self).__init__()
        # Initialize manager references
        self._view_manager: ViewManager = ViewManager.get_instance()
        # Initialize view references
        self._error_page: ErrorPage = self._view_manager.get_error_page()

    def _ok(self) -> None:
        QCoreApplication.instance().quit()

    def setup(self) -> None:
        self._error_page.ok_button.clicked.connect(self._ok)
