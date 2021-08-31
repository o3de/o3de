"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from PySide2.QtCore import Slot
from PySide2.QtGui import (QIcon, QPixmap)
from PySide2.QtWidgets import (QFrame, QHBoxLayout, QLabel, QLayout, QLineEdit, QPushButton,
                               QSizePolicy, QVBoxLayout, QWidget)


class SearchFilterLineEdit(QLineEdit):
    """SearchFilterLineEdit is composed of search icon and search text input"""
    def __init__(self, parent, placeholder_text: str) -> None:
        super().__init__(parent)

        search_icon: QIcon = QIcon(":/stylesheet/img/search-dark.svg")
        self.addAction(search_icon, QLineEdit.LeadingPosition)
        self.setPlaceholderText(placeholder_text)
        self.home(True)
        self.deselect()


class NotificationFrame(QFrame):
    """NotificationFrame is composed of information icon, notification title and close icon"""
    def __init__(self, parent: QWidget, pixmap: QPixmap, content: str, closeable: bool, title: str = '') -> None:
        super().__init__(parent)

        self.setObjectName("NotificationFrame")
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Maximum)
        self.setFrameStyle(QFrame.StyledPanel | QFrame.Plain)

        self._header_layout: QHBoxLayout = QHBoxLayout(self)
        self._header_layout.setSizeConstraint(QLayout.SetMinimumSize)
        self._header_layout.setContentsMargins(0, 0, 0, 0)
        self._header_layout.setSpacing(0)

        icon_label: QLabel = QLabel(self)
        icon_label.setObjectName("NotificationIcon")
        icon_label.setPixmap(pixmap)
        self._header_layout.addWidget(icon_label)

        text_widget = QWidget(self)
        text_layout: QVBoxLayout = QVBoxLayout(text_widget)
        text_layout.setContentsMargins(0, 0, 0, 0)
        text_layout.setSpacing(5)

        self._title_label: QLabel = QLabel(title, self)
        self._title_label.setObjectName("NotificationTitle")
        self._title_label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        text_layout.addWidget(self._title_label)
        if not title:
            self._title_label.hide()

        self._content_label: QLabel = QLabel(content, self)
        self._content_label.setOpenExternalLinks(True)
        self._content_label.setObjectName("NotificationContent")
        self._content_label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred)
        self._content_label.setWordWrap(True)
        text_layout.addWidget(self._content_label)

        self._header_layout.addWidget(text_widget)

        if closeable:
            cancel_button: QPushButton = QPushButton(self)
            cancel_button.setIcon(QIcon(":/stylesheet/img/close.svg"))
            cancel_button.setFlat(True)
            cancel_button.clicked.connect(self.hide)
            self._header_layout.addWidget(cancel_button)
        self.setLayout(self._header_layout)

        self.setVisible(False)

    @Slot(str)
    def set_frame_text_receiver(self, content: str, title: str = '') -> None:
        if title:
            self._title_label.setText(title)
            self._title_label.show()
        else:
            self._title_label.hide()

        self._content_label.setText(content)
        self.setVisible(True)
