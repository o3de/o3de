"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from typing import List
from PySide2.QtCore import (QItemSelectionModel, QModelIndex, QRect, Qt)
from PySide2.QtWidgets import (QAbstractItemView, QComboBox, QCommandLinkButton, QHeaderView, QLabel, QLineEdit,
                               QPushButton, QSizePolicy, QStackedWidget, QTreeView, QVBoxLayout, QWidget)

from model import constants
from model.resource_proxy_model import ResourceProxyModel
from model.resource_tree_model import (ResourceTreeConstants, ResourceTreeModel, ResourceTreeNode)
from view.common_view_components import SearchFilterWidget


class ConfigurationPage(QWidget):
    """
    Configuration page, which includes all related Qt widgets to interact for
    searching and importing resources
    """
    def __init__(self) -> None:
        super().__init__()
        self.setObjectName("configurationPage")

        self._page_vertical_layout: QVBoxLayout = QVBoxLayout(self)
        self._page_vertical_layout.setObjectName("pageVerticalLayout")
        self._page_vertical_layout.setGeometry(QRect(10, 20, 900, 560))

        self._notification_group: QWidget = QWidget(self)
        self._notification_label: QLabel = QLabel(self._notification_group)
        size_policy = QSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
        size_policy.setHeightForWidth(self._notification_label.sizePolicy().hasHeightForWidth())
        self._notification_label.setObjectName("notificationLabel")
        self._notification_label.setText("Please select the root folder of your resource mapping config files")

        self._page_vertical_layout.addWidget(self._notification_group)
