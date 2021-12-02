"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2.QtCore import Qt
from PySide2.QtGui import QPixmap
from PySide2.QtWidgets import (QHBoxLayout, QLayout, QPushButton, QSizePolicy, QSpacerItem, QVBoxLayout, QWidget)

from model import (error_messages, notification_label_text, view_size_constants)
from view.common_view_components import NotificationFrame


class ErrorPage(QWidget):
    """
    Error Page
    """
    def __init__(self) -> None:
        super().__init__()
        self.setGeometry(0, 0,
                         view_size_constants.ERROR_PAGE_MAIN_WINDOW_WIDTH,
                         view_size_constants.ERROR_PAGE_MAIN_WINDOW_HEIGHT)

        page_vertical_layout: QVBoxLayout = QVBoxLayout(self)
        page_vertical_layout.setSizeConstraint(QLayout.SetMinimumSize)
        page_vertical_layout.setContentsMargins(
            view_size_constants.ERROR_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
            view_size_constants.ERROR_PAGE_LAYOUT_MARGIN_TOPBOTTOM,
            view_size_constants.ERROR_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
            view_size_constants.ERROR_PAGE_LAYOUT_MARGIN_TOPBOTTOM
        )
        page_vertical_layout.setSpacing(view_size_constants.ERROR_PAGE_LAYOUT_SPACING)

        self._setup_notification_area()
        page_vertical_layout.addWidget(self._notification_area)

        self._setup_footer_area()
        page_vertical_layout.addWidget(self._footer_area)

    def _setup_notification_area(self) -> None:
        self._notification_area: QWidget = QWidget(self)
        self._notification_area.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Minimum)
        self._notification_area.setMinimumSize(view_size_constants.ERROR_PAGE_MAIN_WINDOW_WIDTH,
                                               view_size_constants.ERROR_PAGE_NOTIFICATION_AREA_HEIGHT)

        notification_area_layout: QVBoxLayout = QVBoxLayout(self._notification_area)
        notification_area_layout.setSpacing(0)
        notification_area_layout.setSizeConstraint(QLayout.SetMinimumSize)
        notification_area_layout.setContentsMargins(0, 0, 0, 0)
        notification_area_layout.setSpacing(0)
        notification_area_layout.setAlignment(Qt.AlignTop | Qt.AlignLeft)

        notification_frame: NotificationFrame = \
            NotificationFrame(self, QPixmap(":/error_report_helper.svg"),
                              error_messages.ERROR_PAGE_TOOL_SETUP_ERROR_MESSAGE,
                              False, error_messages.ERROR_PAGE_TOOL_SETUP_ERROR_TITLE)
        notification_frame.setObjectName("ErrorPage")
        notification_frame.setMinimumSize(view_size_constants.ERROR_PAGE_MAIN_WINDOW_WIDTH,
                                          view_size_constants.ERROR_PAGE_NOTIFICATION_AREA_HEIGHT)
        notification_frame.setVisible(True)
        notification_area_layout.addWidget(notification_frame)

    def _setup_footer_area(self) -> None:
        self._footer_area: QWidget = QWidget(self)
        self._footer_area.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Minimum)
        self._footer_area.setMaximumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                         view_size_constants.ERROR_PAGE_FOOTER_AREA_HEIGHT)

        footer_area_layout: QHBoxLayout = QHBoxLayout(self._footer_area)
        footer_area_layout.setSizeConstraint(QLayout.SetMinimumSize)
        footer_area_layout.setContentsMargins(0, 0, 0, 0)
        footer_area_layout.setAlignment(Qt.AlignBottom | Qt.AlignRight)

        footer_area_spacer: QSpacerItem = QSpacerItem(view_size_constants.ERROR_PAGE_MAIN_WINDOW_WIDTH,
                                                      view_size_constants.INTERACTION_COMPONENT_HEIGHT,
                                                      QSizePolicy.MinimumExpanding, QSizePolicy.Minimum)
        footer_area_layout.addItem(footer_area_spacer)

        self._ok_button: QPushButton = QPushButton(self._footer_area)
        self._ok_button.setObjectName("Secondary")
        self._ok_button.setText(notification_label_text.ERROR_PAGE_OK_TEXT)
        self._ok_button.setMinimumSize(view_size_constants.OK_BUTTON_WIDTH,
                                       view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        footer_area_layout.addWidget(self._ok_button)

    @property
    def ok_button(self) -> QPushButton:
        return self._ok_button
