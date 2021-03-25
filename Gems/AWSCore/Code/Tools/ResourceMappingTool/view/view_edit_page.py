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
from PySide2.QtCore import (QAbstractItemModel, QModelIndex, QObject, QRect, QSize, Qt)
from PySide2.QtGui import (QFont, QFontMetrics)
from PySide2.QtWidgets import (QAbstractItemDelegate, QAbstractItemView, QComboBox, QHeaderView, QLabel, QLineEdit,
                               QPushButton, QStackedWidget, QStyledItemDelegate, QStyleOptionViewItem,
                               QTableView, QVBoxLayout, QWidget)

from model import (constants, notification_label_text, view_size_constants)
from model.resource_proxy_model import ResourceProxyModel
from model.resource_table_model import (ResourceTableConstants, ResourceTableModel)
from view.common_view_components import (NotificationFrame, NotificationPrompt, SearchFilterWidget)


class ViewEditPageConstants(object):
    NOTIFICATION_PAGE_INDEX = 0
    TABLE_VIEW_PAGE_INDEX = 1

    NOTIFICATION_LOADING_TEXT: str = "Loading..."
    NOTIFICATION_SELECT_CONFIG_FILE_TEXT: str = "Please select the Config file you would like to view and modify..."


class ComboboxItemDelegate(QStyledItemDelegate):
    """
    Custom item delegate to present individual item as a combobox while
    editing
    """
    def __init__(self, parent: QObject, combobox_values: List[str]) -> None:
        super(ComboboxItemDelegate, self).__init__(parent)
        self._combobox_values: List[str] = combobox_values

    def createEditor(self, parent: QWidget, option: QStyleOptionViewItem, index: QModelIndex) -> QWidget:
        """QStyledItemDelegate override"""
        current_text: str = index.data()
        combobox: QComboBox = QComboBox(parent)
        combobox_index: int = 0
        combobox_value: str
        for combobox_value in self._combobox_values:
            combobox.addItem(combobox_value)
            combobox.setItemData(combobox_index, combobox_value, Qt.ToolTipRole)
            if combobox_value == current_text:
                combobox.setCurrentIndex(combobox_index)
            combobox_index += 1
        if not current_text:  # if current text is empty, set combobox to default index
            combobox.setCurrentIndex(-1)

        combobox.currentIndexChanged.connect(self._on_current_index_changed)
        return combobox

    def setModelData(self, editor: QWidget, model: QAbstractItemModel, index: QModelIndex) -> None:
        """QStyledItemDelegate override"""
        model.setData(index, editor.currentText(), Qt.EditRole)

    def _on_current_index_changed(self):
        editor: QWidget = self.sender()
        self.commitData.emit(editor)
        self.closeEditor.emit(editor, QAbstractItemDelegate.NoHint)


class ResourceTableView(QTableView):
    """
    Custom QTableView, which maintains custom resource table model to show
    resources with a table view
    """
    def __init__(self, parent: QWidget = None) -> None:
        super(ResourceTableView, self).__init__(parent)
        self._resource_proxy_model: ResourceProxyModel = ResourceProxyModel()
        self._resource_proxy_model.setSourceModel(ResourceTableModel())
        self.setModel(self._resource_proxy_model)

        self.setItemDelegateForColumn(ResourceTableConstants.TABLE_REGION_COLUMN_INDEX,
                                      ComboboxItemDelegate(self, constants.AWS_RESOURCE_REGIONS))

    @property
    def resource_proxy_model(self) -> ResourceProxyModel:
        return self._resource_proxy_model

    def clear_selection(self) -> None:
        """Clear table view selection"""
        self.selectionModel().clearSelection()
        self.selectionModel().clear()

    def reset_view(self) -> None:
        """Reset table view selection and internal model"""
        self.clear_selection()
        self._resource_proxy_model.reset()


class ViewEditPage(QWidget):
    """
    View edit page, which includes all related Qt widgets to interact for
    viewing and editing resources
    """
    def __init__(self) -> None:
        super().__init__()
        self.setObjectName("ViewEditPage")
        self.setGeometry(0, 0,
                         view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                         view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_HEIGHT)
        
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

        self._setup_center_area()
        page_vertical_layout.addWidget(self._stacked_pages)

        self._setup_footer_area()
        page_vertical_layout.addWidget(self._footer_area)

    def _setup_header_area(self) -> None:
        self._header_area: QWidget = QWidget(self)
        self._header_area.setObjectName("HeaderArea")
        self._header_area.setMinimumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                         view_size_constants.VIEW_EDIT_PAGE_HEADER_FOOTER_AREA_HEIGHT)
        
        config_file_label: QLabel = QLabel(self._header_area)
        config_file_label.setObjectName("ConfigFileLabel")
        config_file_label.setText("Config File:")
        config_file_label.setGeometry(view_size_constants.CONFIG_FILE_LABEL_X,
                                      view_size_constants.VIEW_EDIT_PAGE_WIDGET_SUBCOMPONENT_Y,
                                      view_size_constants.CONFIG_FILE_LABEL_WIDTH,
                                      view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        
        self._config_file_combobox: QComboBox = QComboBox(self._header_area)
        self._config_file_combobox.setObjectName("ConfigFileCombobox")
        self._config_file_combobox.setLineEdit(QLineEdit())
        self._config_file_combobox.lineEdit().setPlaceholderText(f"Found 0 config file")
        self._config_file_combobox.lineEdit().setReadOnly(True)
        self._config_file_combobox.setGeometry(view_size_constants.CONFIG_FILE_COMBOBOX_X,
                                               view_size_constants.VIEW_EDIT_PAGE_WIDGET_SUBCOMPONENT_Y,
                                               view_size_constants.CONFIG_FILE_COMBOBOX_WIDTH,
                                               view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        self._config_file_combobox.setCurrentIndex(-1)
        
        config_location_label: QLabel = QLabel(self._header_area)
        config_location_label.setObjectName("ConfigLocationLabel")
        config_location_label.setText("Config Location:")
        config_location_label.setGeometry(view_size_constants.CONFIG_LOCATION_LABEL_X,
                                          view_size_constants.VIEW_EDIT_PAGE_WIDGET_SUBCOMPONENT_Y,
                                          view_size_constants.CONFIG_LOCATION_LABEL_WIDTH,
                                          view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        
        self._config_location_text: QLabel = QLabel(self._header_area)
        self._config_location_text.setObjectName("ConfigLocationText")
        self._config_location_text.setToolTip("None config directory given")
        self._config_location_text.setGeometry(view_size_constants.CONFIG_LOCATION_TEXT_X,
                                               view_size_constants.VIEW_EDIT_PAGE_WIDGET_SUBCOMPONENT_Y,
                                               view_size_constants.CONFIG_LOCATION_TEXT_WIDTH,
                                               view_size_constants.INTERACTION_COMPONENT_HEIGHT)

        # TODO: unicode set up for now, will replace with icon
        self._config_location_button: QPushButton = QPushButton(self._header_area)
        self._config_location_button.setObjectName("ConfigLocationButton")
        self._config_location_button.setText("📂")
        font: QFont = self._config_location_button.font()
        font.setPointSize(view_size_constants.ICON_FRONT_SIZE)
        self._config_location_button.setFont(font)
        self._config_location_button.setGeometry(view_size_constants.CONFIG_LOCATION_BUTTON_X,
                                                 view_size_constants.VIEW_EDIT_PAGE_WIDGET_SUBCOMPONENT_Y,
                                                 view_size_constants.INTERACTION_COMPONENT_HEIGHT,
                                                 view_size_constants.INTERACTION_COMPONENT_HEIGHT)

    def _setup_center_area(self) -> None:
        self._stacked_pages: QStackedWidget = QStackedWidget(self)
        self._stacked_pages.setObjectName("StackedPages")
        self._stacked_pages.setMaximumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                           view_size_constants.VIEW_EDIT_PAGE_CENTER_AREA_HEIGHT)

        self._setup_notification_page()
        self._stacked_pages.addWidget(self._notification_page)

        self._setup_table_view_page()
        self._stacked_pages.addWidget(self._table_view_page)

    def _setup_notification_page(self) -> None:
        self._notification_page: QWidget = QWidget()
        self._notification_page.setObjectName("NotificationPage")

        self._notification_page_label: QLabel = QLabel(self._notification_page)
        self._notification_page_label.setObjectName("NotificationLabel")
        self._notification_page_label.setText("Loading ...")
        self._notification_page_label.setGeometry(0, 0,
                                                  view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                                  view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        self._notification_page_label.setAlignment(Qt.AlignCenter)

        self._notification_page_prompt: QWidget = \
            NotificationPrompt(self._notification_page,
                               QRect(0, 0,
                                     view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                     view_size_constants.NOTIFICATION_PAGE_PROMPT_HEIGHT),
                               QSize(view_size_constants.NOTIFICATION_PAGE_PROMPT_BUTTON_WIDTH,
                                     view_size_constants.INTERACTION_COMPONENT_HEIGHT))

    def _setup_table_view_page(self) -> None:
        self._table_view_page: QWidget = QWidget()
        self._table_view_page.setObjectName("TableViewPage")

        table_view_vertical_layout: QVBoxLayout = QVBoxLayout(self._table_view_page)
        table_view_vertical_layout.setObjectName("TableViewVerticalLayout")
        table_view_vertical_layout.setSpacing(0)
        table_view_vertical_layout.setContentsMargins(0, 0, 0, 0)

        interaction_area: QWidget = QWidget(self._table_view_page)
        interaction_area.setObjectName("InteractionArea")
        interaction_area.setMinimumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                        view_size_constants.TABLE_VIEW_PAGE_INTERACTION_AREA_HEIGHT)

        self._add_row_button: QPushButton = QPushButton(interaction_area)
        self._add_row_button.setObjectName("AddRowButton")
        self._add_row_button.setText("Add Row")
        self._add_row_button.setGeometry(view_size_constants.ADD_ROW_BUTTON_X, 0,
                                         view_size_constants.ADD_ROW_BUTTON_WIDTH,
                                         view_size_constants.INTERACTION_COMPONENT_HEIGHT)

        self._delete_row_button: QPushButton = QPushButton(interaction_area)
        self._delete_row_button.setObjectName("DeleteRowButton")
        self._delete_row_button.setText("Delete Row")
        self._delete_row_button.setGeometry(view_size_constants.DELETE_ROW_BUTTON_X, 0,
                                            view_size_constants.DELETE_ROW_BUTTON_WIDTH,
                                            view_size_constants.INTERACTION_COMPONENT_HEIGHT)

        self._import_resources_label: QLabel = QLabel(interaction_area)
        self._import_resources_label.setObjectName("ImportResourcesLabel")
        self._import_resources_label.setText("Import")
        self._import_resources_label.setGeometry(view_size_constants.IMPORT_RESOURCES_LABEL_X, 0,
                                                 view_size_constants.IMPORT_RESOURCES_LABEL_WIDTH,
                                                 view_size_constants.INTERACTION_COMPONENT_HEIGHT)

        self._import_resources_combobox: QComboBox = QComboBox(interaction_area)
        self._import_resources_combobox.setObjectName("ImportResourcesCombobox")
        self._import_resources_combobox.setLineEdit(QLineEdit())
        self._import_resources_combobox.lineEdit().setPlaceholderText("Please Select")
        self._import_resources_combobox.lineEdit().setReadOnly(True)
        self._import_resources_combobox.setGeometry(view_size_constants.IMPORT_RESOURCES_COMBOBOX_X, 0,
                                                    view_size_constants.IMPORT_RESOURCES_COMBOBOX_WIDTH,
                                                    view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        self._import_resources_combobox.addItem(constants.SEARCH_TYPED_RESOURCES_VERSION)
        self._import_resources_combobox.addItem(constants.SEARCH_CFN_STACKS_VERSION)
        self._import_resources_combobox.setCurrentIndex(-1)

        self._search_filter_widget: SearchFilterWidget = \
            SearchFilterWidget(interaction_area, notification_label_text.VIEW_EDIT_PAGE_SEARCH_PLACEHOLDER_TEXT,
                               QRect(view_size_constants.TABLE_VIEW_PAGE_SEARCH_FILTER_WIDGET_X, 0,
                                     view_size_constants.INTERACTION_COMPONENT_HEIGHT,
                                     view_size_constants.INTERACTION_COMPONENT_HEIGHT),
                               view_size_constants.TABLE_VIEW_PAGE_SEARCH_FILTER_WIDGET_WIDTH)

        table_view_vertical_layout.addWidget(interaction_area)

        self._table_view: ResourceTableView = ResourceTableView(self._table_view_page)
        self._table_view.setObjectName("TableView")
        self._table_view.setMaximumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                        view_size_constants.TABLE_VIEW_PAGE_TABLE_VIEW_AREA_HEIGHT)
        self._table_view.setSelectionBehavior(QAbstractItemView.SelectRows)
        self._table_view.setSortingEnabled(True)

        horizontal_header: QHeaderView = self._table_view.horizontalHeader()
        horizontal_header.setSectionResizeMode(ResourceTableConstants.TABLE_STATE_COLUMN_INDEX,
                                               QHeaderView.Stretch)
        horizontal_header.setMaximumSectionSize(view_size_constants.RESOURCE_TABLE_HEADER_CELL_MAXIMUM)
        horizontal_header.setMinimumSectionSize(view_size_constants.RESOURCE_TABLE_HEADER_CELL_MINIMUM)
        horizontal_header.resizeSection(ResourceTableConstants.TABLE_KEYNAME_COLUMN_INDEX,
                                        view_size_constants.RESOURCE_TABLE_KEYNAME_COLUMN_WIDTH)
        horizontal_header.resizeSection(ResourceTableConstants.TABLE_TYPE_COLUMN_INDEX,
                                        view_size_constants.RESOURCE_TABLE_TYPE_COLUMN_WIDTH)
        horizontal_header.resizeSection(ResourceTableConstants.TABLE_NAME_OR_ID_COLUMN_INDEX,
                                        view_size_constants.RESOURCE_TABLE_NAME_OR_ID_COLUMN_WIDTH)
        horizontal_header.resizeSection(ResourceTableConstants.TABLE_ACCOUNTID_COLUMN_INDEX,
                                        view_size_constants.RESOURCE_TABLE_ACCOUNTID_COLUMN_WIDTH)
        horizontal_header.resizeSection(ResourceTableConstants.TABLE_REGION_COLUMN_INDEX,
                                        view_size_constants.RESOURCE_TABLE_REGION_COLUMN_WIDTH)

        table_view_vertical_layout.addWidget(self._table_view)

    def _setup_footer_area(self) -> None:
        self._footer_area: QWidget = QWidget(self)
        self._footer_area.setObjectName("FooterArea")
        self._footer_area.setMinimumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                         view_size_constants.VIEW_EDIT_PAGE_HEADER_FOOTER_AREA_HEIGHT)
        
        self._save_changes_button: QPushButton = QPushButton(self._footer_area)
        self._save_changes_button.setObjectName("SaveChangesButton")
        self._save_changes_button.setText("Save Changes")
        self._save_changes_button.setGeometry(view_size_constants.SAVE_CHANGES_BUTTON_X,
                                              view_size_constants.VIEW_EDIT_PAGE_WIDGET_SUBCOMPONENT_Y,
                                              view_size_constants.SAVE_CHANGES_BUTTON_WIDTH,
                                              view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        
        self._cancel_button: QPushButton = QPushButton(self._footer_area)
        self._cancel_button.setObjectName("CancelButton")
        self._cancel_button.setText("Cancel")
        self._cancel_button.setGeometry(view_size_constants.CANCEL_BUTTON_X,
                                        view_size_constants.VIEW_EDIT_PAGE_WIDGET_SUBCOMPONENT_Y,
                                        view_size_constants.CANCEL_BUTTON_WIDTH,
                                        view_size_constants.INTERACTION_COMPONENT_HEIGHT)

    @property
    def add_row_button(self) -> QPushButton:
        return self._add_row_button

    @property
    def cancel_button(self) -> QPushButton:
        return self._cancel_button

    @property
    def config_file_combobox(self) -> QComboBox:
        return self._config_file_combobox

    @property
    def config_location_button(self) -> QPushButton:
        return self._config_location_button

    @property
    def delete_row_button(self) -> QPushButton:
        return self._delete_row_button

    @property
    def import_resources_combobox(self) -> QComboBox:
        return self._import_resources_combobox
    
    @property
    def save_changes_button(self) -> QPushButton:
        return self._save_changes_button
    
    @property
    def table_view(self) -> ResourceTableView:
        return self._table_view

    @property
    def search_filter_input(self) -> QLineEdit:
        return self._search_filter_widget.search_filter_input

    @property
    def notification_prompt_create_new_button(self) -> QPushButton:
        return self._notification_page_prompt.create_new_button

    @property
    def notification_prompt_rescan_button(self) -> QPushButton:
        return self._notification_page_prompt.rescan_button

    def set_current_main_view_index(self, index: int) -> None:
        """Switch main view page based on given index"""
        if index == ViewEditPageConstants.NOTIFICATION_PAGE_INDEX:
            self._save_changes_button.setVisible(False)
        else:
            self._save_changes_button.setVisible(True)

        self._stacked_pages.setCurrentIndex(index)

    def set_config_files(self, config_files: List[str]) -> None:
        """Set config file list in combo box"""
        self._config_file_combobox.blockSignals(True)
        self._config_file_combobox.clear()

        config_index: int = 0
        config_file: str
        for config_file in config_files:
            self._config_file_combobox.addItem(config_file)
            self._config_file_combobox.setItemData(config_index, config_file, Qt.ToolTipRole)
            config_index += 1

        self._config_file_combobox.lineEdit().setPlaceholderText(f"Found {len(config_files)} config files")
        self._config_file_combobox.setCurrentIndex(-1)

        if config_files:
            self._notification_page_label.setVisible(True)
            self._notification_page_prompt.set_visible(False)
        else:
            self._notification_page_label.setVisible(False)
            self._notification_page_prompt.set_visible(True)

        self._config_file_combobox.blockSignals(False)

    def set_config_location(self, config_location: str) -> None:
        """Set config file location in text label"""
        self._config_location_text.clear()

        metrics: QFontMetrics = QFontMetrics(self._config_location_text.font())
        elided_text: str = metrics.elidedText(config_location, Qt.ElideMiddle, self._config_location_text.width())
        self._config_location_text.setText(elided_text)
        self._config_location_text.setToolTip(config_location)

    def set_notification_page_text(self, text: str) -> None:
        self._notification_page_label.setText(text)

    def hide_notification_frame(self) -> None:
        self._notification_frame.hide()

    def set_notification_frame_text(self, text: str) -> None:
        self._notification_frame.set_text(text)

    def set_table_view_page_interactions_enabled(self, enabled: bool) -> None:
        self._table_view_page.setEnabled(enabled)
        self._save_changes_button.setEnabled(enabled)
