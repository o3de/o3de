"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from PySide2.QtCore import (QRect, QSize, Qt)
from PySide2.QtGui import QFont
from PySide2.QtWidgets import (QFrame, QHBoxLayout, QLabel, QLayout, QLineEdit, QPushButton,
                               QSizePolicy, QVBoxLayout, QWidget)

from model import view_size_constants


class SearchFilterWidget(object):
    """SearchFilterWidget is composed of search icon and search text input"""
    def __init__(self, parent, placeholder_text: str, label_rect: QRect, input_width: int) -> None:
        super().__init__()

        # TODO: unicode set up for now, will replace with icon
        self._search_filter_label: QLabel = QLabel(parent)
        self._search_filter_label.setObjectName("SearchTextLabel")
        self._search_filter_label.setText("🔎")
        font: QFont = self._search_filter_label.font()
        font.setPointSize(view_size_constants.ICON_FRONT_SIZE)  # temp value, will remove once replace with icon
        self._search_filter_label.setFont(font)
        self._search_filter_label.setGeometry(label_rect)

        # TODO: revise this front end to be user friendly
        self._search_filter_input: QLineEdit = QLineEdit(parent)
        self._search_filter_input.setObjectName("SearchTextInput")
        self._search_filter_input.setPlaceholderText(placeholder_text)
        self._search_filter_input.setGeometry(label_rect.x() + label_rect.width(),
                                              label_rect.y(), input_width, label_rect.height())
        self._search_filter_input.home(True)
        self._search_filter_input.deselect()

    @property
    def search_filter_input(self) -> QLineEdit:
        return self._search_filter_input


class NotificationFrame(QFrame):
    def __init__(self, parent: QWidget, title: str = "",
                 icon_size: int = view_size_constants.ICON_FRONT_SIZE,
                 cancel_size: int = view_size_constants.INTERACTION_COMPONENT_HEIGHT) -> None:
        super().__init__(parent)

        self.setObjectName("AzQtComponents--CardNotification")
        self.setSizePolicy(QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Maximum))
        self.setFrameStyle(QFrame.StyledPanel | QFrame.Plain)

        header_frame: QFrame = QFrame(self)
        header_frame.setObjectName("HeaderFrame")
        header_frame.setSizePolicy(QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Maximum))

        # TODO: unicode set up for now, will replace with icon
        icon_label: QLabel = QLabel(header_frame)
        icon_label.setObjectName("Icon")
        icon_label.setText("🛈")
        font: QFont = icon_label.font()
        font.setPointSize(icon_size)
        icon_label.setFont(font)

        self._title_label: QLabel = QLabel(title, header_frame)
        self._title_label.setObjectName("Title")
        self._title_label.setSizePolicy(QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed))
        self._title_label.setWordWrap(True)

        cancel_button: QPushButton = QPushButton("CancelButton", header_frame)
        cancel_button.setText("🗙")
        font: QFont = cancel_button.font()
        font.setPointSize(icon_size)
        cancel_button.setFont(font)
        cancel_button.setMaximumSize(cancel_size, cancel_size)
        cancel_button.clicked.connect(self.hide)

        header_layout: QHBoxLayout = QHBoxLayout(header_frame)
        header_layout.setSizeConstraint(QLayout.SetMinimumSize)
        header_layout.setContentsMargins(0, 0, 0, 0)
        header_layout.addWidget(icon_label)
        header_layout.addWidget(self._title_label)
        header_layout.addWidget(cancel_button)
        header_frame.setLayout(header_layout)

        feature_layout: QVBoxLayout = QVBoxLayout(self)
        feature_layout.setSizeConstraint(QLayout.SetMinimumSize)
        feature_layout.setContentsMargins(0, 0, 0, 0)
        feature_layout.addWidget(header_frame)

        self.setVisible(False)

    def hide(self) -> None:
        self._title_label.setText("")
        self.setVisible(False)

    def set_text(self, text: str) -> None:
        self._title_label.setText(text)
        self.setVisible(True)


class NotificationPrompt(QWidget):
    def __init__(self, parent: QWidget, prompt_rect: QRect, button_size: QSize) -> None:
        super().__init__(parent)
        self.setObjectName("NotificationPrompt")
        self.setSizePolicy(QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding))
        self.setGeometry(prompt_rect)

        notification_prompt_label: QLabel = \
            QLabel("Unable to locate a resource mapping config file. "
                   "We can either make this file for you or you can rescan your directory")
        notification_prompt_label.setObjectName("NotificationPromptLabel")
        notification_prompt_label.setSizePolicy(QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding))
        notification_prompt_label.setWordWrap(True)
        notification_prompt_label.setAlignment(Qt.AlignCenter)

        notification_prompt_interactions: QWidget = QWidget(self)
        notification_prompt_interactions.setObjectName("NotificationInteractions")
        notification_prompt_interactions.setSizePolicy(QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding))

        self._create_new_button: QPushButton = QPushButton("Create New", notification_prompt_interactions)
        self._create_new_button.setObjectName("CreateNewButton")
        self._create_new_button.setMaximumSize(button_size)

        self._rescan_button: QPushButton = QPushButton("Rescan", notification_prompt_interactions)
        self._rescan_button.setObjectName("RescanButton")
        self._rescan_button.setMaximumSize(button_size)

        notification_prompt_interactions_layout: QHBoxLayout = QHBoxLayout(notification_prompt_interactions)
        notification_prompt_interactions_layout.addWidget(self._create_new_button)
        notification_prompt_interactions_layout.addWidget(self._rescan_button)
        notification_prompt_interactions.setLayout(notification_prompt_interactions_layout)

        feature_layout: QVBoxLayout = QVBoxLayout(self)
        feature_layout.addWidget(notification_prompt_label)
        feature_layout.addWidget(notification_prompt_interactions)

        self.setVisible(False)

    @property
    def create_new_button(self) -> QPushButton:
        return self._create_new_button

    @property
    def rescan_button(self) -> QPushButton:
        return self._rescan_button

    def set_visible(self, enabled: bool) -> None:
        self.setVisible(enabled)
