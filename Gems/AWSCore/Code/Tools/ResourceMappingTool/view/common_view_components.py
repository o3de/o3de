"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from PySide2.QtGui import (QIcon, QPixmap)
from PySide2.QtWidgets import (QFrame, QHBoxLayout, QLabel, QLayout, QLineEdit, QPushButton,
                               QSizePolicy, QWidget)


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
    def __init__(self, parent: QWidget, pixmap: QPixmap, title: str, closeable: bool) -> None:
        super().__init__(parent)

        self.setObjectName("NotificationFrame")
        self.setSizePolicy(QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Maximum))
        self.setFrameStyle(QFrame.StyledPanel | QFrame.Plain)

        icon_label: QLabel = QLabel(self)
        icon_label.setPixmap(pixmap)

        self._title_label: QLabel = QLabel(title, self)
        self._title_label.setObjectName("Title")
        self._title_label.setSizePolicy(QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Preferred))
        self._title_label.setWordWrap(True)

        header_layout: QHBoxLayout = QHBoxLayout(self)
        header_layout.setSizeConstraint(QLayout.SetMinimumSize)
        header_layout.setContentsMargins(0, 0, 0, 0)
        header_layout.addWidget(icon_label)
        header_layout.addWidget(self._title_label)
        if closeable:
            cancel_button: QPushButton = QPushButton(self)
            cancel_button.setIcon(QIcon(":/stylesheet/img/close.svg"))
            cancel_button.setFlat(True)
            cancel_button.clicked.connect(self.hide)
            header_layout.addWidget(cancel_button)
        self.setLayout(header_layout)

        self.setVisible(False)

    def set_text(self, text: str) -> None:
        self._title_label.setText(text)
