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
from PySide2.QtWidgets import (QAbstractItemView, QComboBox, QHeaderView, QLabel, QLineEdit,
                               QPushButton, QStackedWidget, QTreeView, QVBoxLayout, QWidget)

from model import (constants, notification_label_text, view_size_constants)
from model.resource_proxy_model import ResourceProxyModel
from model.resource_tree_model import (ResourceTreeConstants, ResourceTreeModel, ResourceTreeNode)
from view.common_view_components import (NotificationFrame, SearchFilterWidget)


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
        self.setObjectName("ImportResourcePage")
        self.setGeometry(0, 0,
                         view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                         view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_HEIGHT)

        self._search_version: str = ""

        page_vertical_layout: QVBoxLayout = QVBoxLayout(self)
        page_vertical_layout.setObjectName("PageVerticalLayout")
        page_vertical_layout.setSpacing(0)
        page_vertical_layout.setContentsMargins(view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
                                                view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_TOPBOTTOM,
                                                view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
                                                view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_TOPBOTTOM)

        self._setup_header_area()
        page_vertical_layout.addWidget(self._header_area)

        self._notification_frame: NotificationFrame = NotificationFrame(self)
        page_vertical_layout.addWidget(self._notification_frame)

        self._setup_search_area()
        page_vertical_layout.addWidget(self._search_area)

        self._setup_view_area()
        page_vertical_layout.addWidget(self._stacked_pages)

    def _setup_header_area(self) -> None:
        self._header_area: QWidget = QWidget(self)
        self._header_area.setObjectName("HeaderArea")
        self._header_area.setMinimumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                         view_size_constants.IMPORT_RESOURCES_PAGE_HEADER_AREA_HEIGHT)

        self._back_button: QPushButton = QPushButton(self._header_area)
        self._back_button.setObjectName("BackButton")
        self._back_button.setText("Back")
        self._back_button.setGeometry(0, 0,
                                      view_size_constants.BACK_BUTTON_WIDTH,
                                      view_size_constants.INTERACTION_COMPONENT_HEIGHT)

    def _setup_search_area(self) -> None:
        self._search_area: QWidget = QWidget(self)
        self._search_area.setObjectName("SearchArea")
        self._search_area.setMinimumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                         view_size_constants.IMPORT_RESOURCES_PAGE_SEARCH_AREA_HEIGHT)

        self._typed_resources_search_group: QWidget = QWidget(self._search_area)
        self._typed_resources_search_group.setObjectName("ResourceTypeSearchGroup")

        typed_resources_label: QLabel = QLabel(self._typed_resources_search_group)
        typed_resources_label.setObjectName("ResourceTypeLabel")
        typed_resources_label.setText("AWS Resource Type")
        typed_resources_label.setGeometry(view_size_constants.TYPED_RESOURCES_LABEL_X,
                                          view_size_constants.IMPORT_RESOURCES_PAGE_WIDGET_SUBCOMPONENT_Y,
                                          view_size_constants.TYPED_RESOURCES_LABEL_WIDTH,
                                          view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        
        self._typed_resources_combobox: QComboBox = QComboBox(self._typed_resources_search_group)
        self._typed_resources_combobox.setObjectName("ResourceTypeCombobox")
        self._typed_resources_combobox.setLineEdit(QLineEdit())
        self._typed_resources_combobox.lineEdit().setPlaceholderText("Please select")
        self._typed_resources_combobox.lineEdit().setReadOnly(True)
        self._typed_resources_combobox.setGeometry(view_size_constants.TYPED_RESOURCES_COMBOBOX_X,
                                                   view_size_constants.IMPORT_RESOURCES_PAGE_WIDGET_SUBCOMPONENT_Y,
                                                   view_size_constants.TYPED_RESOURCES_COMBOBOX_WIDTH,
                                                   view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        self._typed_resources_combobox.addItems(constants.AWS_RESOURCE_TYPES)
        self._typed_resources_combobox.setCurrentIndex(-1)
        
        self._typed_resources_search_button: QPushButton = QPushButton(self._typed_resources_search_group)
        self._typed_resources_search_button.setObjectName("ResourceTypeSearchButton")
        self._typed_resources_search_button.setText("Search")
        self._typed_resources_search_button.setGeometry(view_size_constants.TYPED_RESOURCES_SEARCH_BUTTON_X,
                                                        view_size_constants.IMPORT_RESOURCES_PAGE_WIDGET_SUBCOMPONENT_Y,
                                                        view_size_constants.SEARCH_BUTTON_WIDTH,
                                                        view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        
        self._typed_resources_import_button = QPushButton(self._typed_resources_search_group)
        self._typed_resources_import_button.setObjectName("ResourceTypeImportButton")
        self._typed_resources_import_button.setText("Import")
        self._typed_resources_import_button.setGeometry(view_size_constants.TYPED_RESOURCES_IMPORT_BUTTON_X,
                                                        view_size_constants.IMPORT_RESOURCES_PAGE_WIDGET_SUBCOMPONENT_Y,
                                                        view_size_constants.IMPORT_BUTTON_WIDTH,
                                                        view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        self._typed_resources_search_group.setVisible(False)

        self._cfn_stacks_search_group: QWidget = QWidget(self._search_area)
        self._cfn_stacks_search_group.setObjectName("CfnSearchGroup")

        self._cfn_stacks_search_button: QPushButton = QPushButton(self._cfn_stacks_search_group)
        self._cfn_stacks_search_button.setObjectName("CfnSearchButton")
        self._cfn_stacks_search_button.setText("Search")
        self._cfn_stacks_search_button.setGeometry(view_size_constants.CFN_STACKS_SEARCH_BUTTON_X,
                                                   view_size_constants.IMPORT_RESOURCES_PAGE_WIDGET_SUBCOMPONENT_Y,
                                                   view_size_constants.SEARCH_BUTTON_WIDTH,
                                                   view_size_constants.INTERACTION_COMPONENT_HEIGHT)

        self._cfn_stacks_import_button: QPushButton = QPushButton(self._cfn_stacks_search_group)
        self._cfn_stacks_import_button.setObjectName("CfnImportButton")
        self._cfn_stacks_import_button.setText("Import")
        self._cfn_stacks_import_button.setGeometry(view_size_constants.CFN_STACKS_IMPORT_BUTTON_X,
                                                   view_size_constants.IMPORT_RESOURCES_PAGE_WIDGET_SUBCOMPONENT_Y,
                                                   view_size_constants.IMPORT_BUTTON_WIDTH,
                                                   view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        self._cfn_stacks_search_group.setVisible(False)

        self._search_filter_widget: SearchFilterWidget = \
            SearchFilterWidget(self._search_area, notification_label_text.IMPORT_RESOURCES_PAGE_SEARCH_PLACEHOLDER_TEXT,
                               QRect(view_size_constants.IMPORT_RESOURCES_PAGE_SEARCH_FILTER_WIDGET_X,
                                     view_size_constants.IMPORT_RESOURCES_PAGE_WIDGET_SUBCOMPONENT_Y,
                                     view_size_constants.INTERACTION_COMPONENT_HEIGHT,
                                     view_size_constants.INTERACTION_COMPONENT_HEIGHT),
                               view_size_constants.IMPORT_RESOURCES_PAGE_SEARCH_FILTER_WIDGET_WIDTH)

    def _setup_view_area(self) -> None:
        self._stacked_pages: QStackedWidget = QStackedWidget(self)
        self._stacked_pages.setObjectName("StackedPages")
        self._stacked_pages.setMaximumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                           view_size_constants.IMPORT_RESOURCES_PAGE_VIEW_AREA_HEIGHT)
        
        notification_page: QWidget = QWidget()
        notification_page.setObjectName("NotificationPage")
        notification_page_label = QLabel(notification_page)
        notification_page_label.setObjectName("NotificationLabel")
        notification_page_label.setText("Loading ...")
        notification_page_label.setGeometry(0, 0,
                                            view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                            view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        notification_page_label.setAlignment(Qt.AlignCenter)
        
        self._stacked_pages.addWidget(notification_page)

        tree_view_page: QWidget = QWidget()
        tree_view_page.setObjectName("TreeViewPage")
        self._tree_view: ResourceTreeView = ResourceTreeView(True, tree_view_page)
        self._tree_view.setObjectName("TreeView")
        self._tree_view.setGeometry(view_size_constants.IMPORT_RESOURCES_PAGE_TREE_VIEW_X, 0,
                                    view_size_constants.IMPORT_RESOURCES_PAGE_TREE_VIEW_WIDTH,
                                    view_size_constants.IMPORT_RESOURCES_PAGE_TREE_VIEW_HEIGHT)
        self._tree_view.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self._tree_view.setSortingEnabled(True)

        header: QHeaderView = self._tree_view.header()
        header.setSectionResizeMode(ResourceTreeConstants.TREE_NAME_OR_ID_COLUMN_INDEX, QHeaderView.Stretch)
        header.setMaximumSectionSize(view_size_constants.RESOURCE_STACK_HEADER_CELL_MAXIMUM)
        header.setMinimumSectionSize(view_size_constants.RESOURCE_STACK_HEADER_CELL_MINIMUM)

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
        return self._search_filter_widget.search_filter_input

    @property
    def search_version(self) -> str:
        return self._search_version

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

    def hide_notification_frame(self) -> None:
        self._notification_frame.hide()

    def set_notification_frame_text(self, text: str) -> None:
        self._notification_frame.set_text(text)
