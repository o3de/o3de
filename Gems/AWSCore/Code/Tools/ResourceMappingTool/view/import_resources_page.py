"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from typing import List
from PySide2.QtCore import (QItemSelectionModel, QModelIndex)
from PySide2.QtGui import (QFont, QIcon, QPixmap)
from PySide2.QtWidgets import (QAbstractItemView, QComboBox, QFrame, QHBoxLayout,  QHeaderView, QLabel, QLayout,
                               QLineEdit, QPushButton, QSizePolicy, QSpacerItem, QStackedWidget, QTreeView,
                               QVBoxLayout, QWidget)

from model import (constants, notification_label_text, view_size_constants)
from model.resource_proxy_model import ResourceProxyModel
from model.resource_tree_model import (ResourceTreeConstants, ResourceTreeModel, ResourceTreeNode)
from view.common_view_components import (NotificationFrame, SearchFilterLineEdit)


class ImportResourcesPageConstants(object):
    NOTIFICATION_PAGE_INDEX = 0
    TREE_VIEW_PAGE_INDEX = 1


class ResourceTreeView(QTreeView):
    """
    Custom QTreeView, which maintains custom resource tree model to show
    resources with a tree view
    """
    def __init__(self, has_child: bool = True, parent: QWidget = None) -> None:
        super(ResourceTreeView, self).__init__(parent)
        self._has_child: bool = has_child
        self._resource_proxy_model: ResourceProxyModel = ResourceProxyModel()
        self._resource_proxy_model.setSourceModel(ResourceTreeModel(has_child))
        self.setModel(self._resource_proxy_model)
        self.clicked.connect(self._auto_select_child_index)

    def _auto_select_child_index(self) -> None:
        if not self._has_child:
            return  # return directly when source model has no children

        indices: List[QModelIndex] = self.selectedIndexes()
        if not indices:
            return

        model_index: QModelIndex
        for model_index in indices:
            source_index = self._resource_proxy_model.mapToSource(model_index)
            node: ResourceTreeNode = source_index.internalPointer()
            if not node.is_root_node():
                continue
            child_row: int
            for child_row in range(0, len(node.children)):
                self.selectionModel().select(model_index.child(child_row, 0),
                                             QItemSelectionModel.Rows | QItemSelectionModel.Select)

    @property
    def has_child(self) -> bool:
        return self._has_child

    @has_child.setter
    def has_child(self, new_has_child: bool) -> None:
        self._has_child = new_has_child
        self._resource_proxy_model.sourceModel().has_child = new_has_child

    @property
    def resource_proxy_model(self) -> ResourceProxyModel:
        return self._resource_proxy_model

    def clear_selection(self) -> None:
        """Clear tree view selection"""
        self.selectionModel().clearSelection()
        self.selectionModel().clear()

    def reset_view(self) -> None:
        """Reset tree view selection and internal model"""
        self.clear_selection()
        self._resource_proxy_model.reset()


class ImportResourcesPage(QWidget):
    """
    Import resources page, which includes all related Qt widgets to interact for
    searching and importing resources
    """
    def __init__(self) -> None:
        super().__init__()
        self.setGeometry(0, 0,
                         view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                         view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_HEIGHT)

        self._search_version: str = ""

        page_vertical_layout: QVBoxLayout = QVBoxLayout(self)
        page_vertical_layout.setSizeConstraint(QLayout.SetMinimumSize)
        page_vertical_layout.setMargin(0)

        self._setup_header_area()
        page_vertical_layout.addWidget(self._header_area)
        header_area_separator: QFrame = QFrame(self)
        header_area_separator.setObjectName("SeparatorLine")
        header_area_separator.setFrameShape(QFrame.HLine)
        header_area_separator.setLineWidth(view_size_constants.HEADER_AREA_SEPARATOR_WIDTH)
        page_vertical_layout.addWidget(header_area_separator)

        self._notification_frame: NotificationFrame = \
            NotificationFrame(self, QPixmap(":/stylesheet/img/logging/information.svg"), "", True)
        page_vertical_layout.addWidget(self._notification_frame)

        self._setup_search_area()
        page_vertical_layout.addWidget(self._search_area)

        self._setup_view_area()
        page_vertical_layout.addWidget(self._stacked_pages)

    def _setup_header_area(self) -> None:
        self._header_area: QWidget = QWidget(self)
        self._header_area.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Minimum)
        self._header_area.setMinimumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                         view_size_constants.IMPORT_RESOURCES_PAGE_HEADER_AREA_HEIGHT)

        header_area_layout: QHBoxLayout = QHBoxLayout(self._header_area)
        header_area_layout.setSizeConstraint(QLayout.SetMinimumSize)
        header_area_layout.setContentsMargins(
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_TOPBOTTOM + view_size_constants.IMPORT_RESOURCES_PAGE_MARGIN_TOPBOTTOM,
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_TOPBOTTOM)

        self._back_button: QPushButton = QPushButton(self._header_area)
        self._back_button.setObjectName("Secondary")
        self._back_button.setText(f"   {notification_label_text.IMPORT_RESOURCES_PAGE_BACK_TEXT}")
        self._back_button.setIcon(QIcon(":/Breadcrumb/img/UI20/Breadcrumb/arrow_left-default.svg"))
        self._back_button.setMinimumSize(view_size_constants.BACK_BUTTON_WIDTH,
                                         view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        header_area_layout.addWidget(self._back_button)

        header_area_spacer: QSpacerItem = QSpacerItem(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                                      view_size_constants.INTERACTION_COMPONENT_HEIGHT,
                                                      QSizePolicy.Expanding, QSizePolicy.Minimum)
        header_area_layout.addItem(header_area_spacer)

    def _setup_search_area(self) -> None:
        self._search_area: QWidget = QWidget(self)
        self._search_area.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Minimum)
        self._search_area.setMinimumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                         view_size_constants.IMPORT_RESOURCES_PAGE_SEARCH_AREA_HEIGHT)

        search_area_layout: QHBoxLayout = QHBoxLayout(self._search_area)
        search_area_layout.setSizeConstraint(QLayout.SetMinimumSize)
        search_area_layout.setContentsMargins(view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
                                              view_size_constants.IMPORT_RESOURCES_PAGE_MARGIN_TOPBOTTOM,
                                              view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
                                              view_size_constants.IMPORT_RESOURCES_PAGE_MARGIN_TOPBOTTOM)

        self._setup_typed_resources_search_group()
        search_area_layout.addWidget(self._typed_resources_search_group)

        self._setup_cfn_stacks_search_group()
        search_area_layout.addWidget(self._cfn_stacks_search_group)

        search_area_spacer: QSpacerItem = QSpacerItem(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                                      view_size_constants.INTERACTION_COMPONENT_HEIGHT,
                                                      QSizePolicy.Expanding, QSizePolicy.Minimum)
        search_area_layout.addItem(search_area_spacer)

        self._search_filter_line_edit: SearchFilterLineEdit = SearchFilterLineEdit(
            self._search_area, notification_label_text.IMPORT_RESOURCES_PAGE_SEARCH_PLACEHOLDER_TEXT)
        self._search_filter_line_edit.setMinimumSize(
            view_size_constants.IMPORT_RESOURCES_PAGE_SEARCH_FILTER_WIDGET_WIDTH,
            view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        search_area_layout.addWidget(self._search_filter_line_edit)

    def _setup_typed_resources_search_group(self) -> None:
        self._typed_resources_search_group: QWidget = QWidget(self._search_area)
        typed_resources_search_group_layout: QHBoxLayout = QHBoxLayout(self._typed_resources_search_group)
        typed_resources_search_group_layout.setSizeConstraint(QLayout.SetMinimumSize)
        typed_resources_search_group_layout.setContentsMargins(0, 0, 0, 0)

        typed_resources_label: QLabel = QLabel(self._typed_resources_search_group)
        typed_resources_label.setText(notification_label_text.IMPORT_RESOURCES_PAGE_AWS_SEARCH_TYPE_TEXT)
        typed_resources_label_font: QFont = QFont()
        typed_resources_label_font.setBold(True)
        typed_resources_label.setFont(typed_resources_label_font)
        typed_resources_label.setMinimumSize(view_size_constants.TYPED_RESOURCES_LABEL_WIDTH,
                                             view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        typed_resources_search_group_layout.addWidget(typed_resources_label)

        self._typed_resources_combobox: QComboBox = QComboBox(self._typed_resources_search_group)
        self._typed_resources_combobox.setLineEdit(QLineEdit())
        self._typed_resources_combobox.lineEdit().setPlaceholderText(
            notification_label_text.IMPORT_RESOURCES_PAGE_AWS_SEARCH_TYPE_PLACEHOLDER_TEXT)
        self._typed_resources_combobox.lineEdit().setReadOnly(True)
        self._typed_resources_combobox.setMinimumSize(view_size_constants.TYPED_RESOURCES_COMBOBOX_WIDTH,
                                                      view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        self._typed_resources_combobox.addItems(constants.AWS_RESOURCE_TYPES)
        self._typed_resources_combobox.setCurrentIndex(-1)
        typed_resources_search_group_layout.addWidget(self._typed_resources_combobox)

        typed_resources_search_group_separator_label: QLabel = QLabel(self._typed_resources_search_group)
        typed_resources_search_group_separator_label.setPixmap(QPixmap(":/stylesheet/img/separator.png"))
        typed_resources_search_group_layout.addWidget(typed_resources_search_group_separator_label)

        self._typed_resources_search_button: QPushButton = QPushButton(self._typed_resources_search_group)
        self._typed_resources_search_button.setObjectName("Secondary")
        self._typed_resources_search_button.setText(notification_label_text.IMPORT_RESOURCES_PAGE_SEARCH_TEXT)
        self._typed_resources_search_button.setMinimumSize(view_size_constants.SEARCH_BUTTON_WIDTH,
                                                           view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        typed_resources_search_group_layout.addWidget(self._typed_resources_search_button)

        self._typed_resources_import_button = QPushButton(self._typed_resources_search_group)
        self._typed_resources_import_button.setObjectName("Primary")
        self._typed_resources_import_button.setText(notification_label_text.IMPORT_RESOURCES_PAGE_IMPORT_TEXT)
        self._typed_resources_import_button.setMinimumSize(view_size_constants.IMPORT_BUTTON_WIDTH,
                                                           view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        typed_resources_search_group_layout.addWidget(self._typed_resources_import_button)

        self._typed_resources_search_group.setVisible(False)

    def _setup_cfn_stacks_search_group(self) -> None:
        self._cfn_stacks_search_group: QWidget = QWidget(self._search_area)
        cfn_stacks_search_group_layout: QHBoxLayout = QHBoxLayout(self._cfn_stacks_search_group)
        cfn_stacks_search_group_layout.setSizeConstraint(QLayout.SetMinimumSize)
        cfn_stacks_search_group_layout.setContentsMargins(0, 0, 0, 0)

        self._cfn_stacks_search_button: QPushButton = QPushButton(self._cfn_stacks_search_group)
        self._cfn_stacks_search_button.setObjectName("Secondary")
        self._cfn_stacks_search_button.setText(notification_label_text.IMPORT_RESOURCES_PAGE_SEARCH_TEXT)
        self._cfn_stacks_search_button.setMinimumSize(view_size_constants.SEARCH_BUTTON_WIDTH,
                                                      view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        cfn_stacks_search_group_layout.addWidget(self._cfn_stacks_search_button)

        self._cfn_stacks_import_button: QPushButton = QPushButton(self._cfn_stacks_search_group)
        self._cfn_stacks_import_button.setObjectName("Primary")
        self._cfn_stacks_import_button.setText(notification_label_text.IMPORT_RESOURCES_PAGE_IMPORT_TEXT)
        self._cfn_stacks_import_button.setMinimumSize(view_size_constants.IMPORT_BUTTON_WIDTH,
                                                      view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        cfn_stacks_search_group_layout.addWidget(self._cfn_stacks_import_button)
        self._cfn_stacks_search_group.setVisible(False)

    def _setup_view_area(self) -> None:
        self._stacked_pages: QStackedWidget = QStackedWidget(self)
        self._stacked_pages.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self._stacked_pages.setMaximumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                           view_size_constants.IMPORT_RESOURCES_PAGE_VIEW_AREA_HEIGHT)
        self._stacked_pages.setContentsMargins(view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT, 0,
                                               view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT, 0)

        notification_page_frame: QFrame = \
            NotificationFrame(self, QPixmap(":/stylesheet/img/32x32/info.png"),
                              notification_label_text.NOTIFICATION_LOADING_MESSAGE, False)
        notification_page_frame.setFixedSize(
            (view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH
                - 2 * view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT),
            view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        notification_page_frame.setVisible(True)
        
        self._stacked_pages.addWidget(notification_page_frame)

        tree_view_page: QWidget = QWidget(self)
        tree_view_vertical_layout: QVBoxLayout = QVBoxLayout(tree_view_page)
        tree_view_vertical_layout.setSizeConstraint(QLayout.SetMinimumSize)
        tree_view_vertical_layout.setContentsMargins(0, 0, 0, 0)

        self._tree_view: ResourceTreeView = ResourceTreeView(True, tree_view_page)
        self._tree_view.setAlternatingRowColors(True)
        self._tree_view.setMaximumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                       view_size_constants.IMPORT_RESOURCES_PAGE_TREE_VIEW_HEIGHT)
        self._tree_view.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self._tree_view.setSortingEnabled(True)

        header: QHeaderView = self._tree_view.header()
        header.setSectionResizeMode(ResourceTreeConstants.TREE_NAME_OR_ID_COLUMN_INDEX, QHeaderView.Stretch)
        header.setMaximumSectionSize(view_size_constants.RESOURCE_STACK_HEADER_CELL_MAXIMUM)
        header.setMinimumSectionSize(view_size_constants.RESOURCE_STACK_HEADER_CELL_MINIMUM)
        tree_view_vertical_layout.addWidget(self._tree_view)

        tree_view_spacer: QSpacerItem = QSpacerItem(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                                    view_size_constants.IMPORT_RESOURCES_PAGE_FOOTER_AREA_HEIGHT,
                                                    QSizePolicy.Expanding, QSizePolicy.Fixed)
        tree_view_vertical_layout.addItem(tree_view_spacer)

        self._stacked_pages.addWidget(tree_view_page)
        self._stacked_pages.setCurrentIndex(1)

    @property
    def back_button(self) -> QPushButton:
        return self._back_button

    @property
    def tree_view(self) -> ResourceTreeView:
        return self._tree_view

    @property
    def typed_resources_combobox(self) -> QComboBox:
        return self._typed_resources_combobox
    
    @property
    def typed_resources_search_button(self) -> QPushButton:
        return self._typed_resources_search_button

    @property
    def typed_resources_import_button(self) -> QPushButton:
        return self._typed_resources_import_button

    @property
    def cfn_stacks_search_button(self) -> QPushButton:
        return self._cfn_stacks_search_button

    @property
    def cfn_stacks_import_button(self) -> QPushButton:
        return self._cfn_stacks_import_button

    @property
    def search_filter_input(self) -> QLineEdit:
        return self._search_filter_line_edit

    @property
    def search_version(self) -> str:
        return self._search_version

    @property
    def notification_frame(self) -> NotificationFrame:
        return self._notification_frame

    @search_version.setter
    def search_version(self, new_search_version: str) -> None:
        self._search_version = new_search_version
        if new_search_version == constants.SEARCH_TYPED_RESOURCES_VERSION:
            self._typed_resources_search_group.setVisible(True)
            self._cfn_stacks_search_group.setVisible(False)
            self._tree_view.has_child = False
        elif new_search_version == constants.SEARCH_CFN_STACKS_VERSION:
            self._typed_resources_search_group.setVisible(False)
            self._cfn_stacks_search_group.setVisible(True)
            self._tree_view.has_child = True
        else:
            self._typed_resources_search_group.setVisible(False)
            self._cfn_stacks_search_group.setVisible(False)

    def set_current_main_view_index(self, index: int) -> None:
        """Switch main view page based on given index"""
        self._stacked_pages.setCurrentIndex(index)
