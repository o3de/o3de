"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
