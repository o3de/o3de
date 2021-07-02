"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from typing import (Dict, List)
from PySide2.QtCore import (QAbstractItemModel, QModelIndex, QObject, Qt)
from PySide2.QtGui import (QIcon, QFont, QFontMetrics, QPainter, QPixmap)
from PySide2.QtWidgets import (QAbstractItemDelegate, QAbstractItemView, QComboBox, QFrame, QHeaderView, QHBoxLayout,
                               QLabel, QLayout, QLineEdit, QPushButton, QSizePolicy, QSpacerItem, QStackedWidget,
                               QStyledItemDelegate, QStyleOptionViewItem, QTableView, QVBoxLayout, QWidget)

from model import (constants, notification_label_text, view_size_constants)
from model.resource_mapping_attributes import ResourceMappingAttributesStatus
from model.resource_proxy_model import ResourceProxyModel
from model.resource_table_model import (ResourceTableConstants, ResourceTableModel)
from view.common_view_components import (NotificationFrame, SearchFilterLineEdit)


class ViewEditPageConstants(object):
    NOTIFICATION_PAGE_INDEX = 0
    TABLE_VIEW_PAGE_INDEX = 1


class ComboboxItemDelegate(QStyledItemDelegate):
    """
    Custom item delegate to present individual item as a combobox while
    editing
    """
    def __init__(self, parent: QObject) -> None:
        super(ComboboxItemDelegate, self).__init__(parent)
        self._combobox_values: List[str] = constants.AWS_RESOURCE_REGIONS

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


class ImageDelegate(QStyledItemDelegate):
    """
    Image delegate to draw image in table cell
    """
    def __init__(self, parent: QObject) -> None:
        super(ImageDelegate, self).__init__(parent)
        self._icons: Dict[str, QIcon] = \
            {ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE: QIcon(":/stylesheet/img/table_success.png"),
             ResourceMappingAttributesStatus.MODIFIED_STATUS_VALUE: QIcon(":/stylesheet/img/table_warning.png"),
             ResourceMappingAttributesStatus.FAILURE_STATUS_VALUE: QIcon(":/stylesheet/img/table_error.png")}

    def get_icon(self, index: QModelIndex) -> QIcon:
        current_text: str = index.data()
        if current_text in self._icons.keys():
            return self._icons[current_text]
        return None

    def paint(self, painter: QPainter, option: QStyleOptionViewItem, index: QModelIndex):
        icon: QIcon = self.get_icon(index)
        if icon:
            icon.paint(painter, option.rect, Qt.AlignCenter)


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
                                      ComboboxItemDelegate(self))
        self.setItemDelegateForColumn(ResourceTableConstants.TABLE_STATE_COLUMN_INDEX,
                                      ImageDelegate(self))

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
        self.setGeometry(0, 0,
                         view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                         view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_HEIGHT)
        
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

        self._setup_center_area()
        page_vertical_layout.addWidget(self._stacked_pages)

        self._setup_footer_area()
        page_vertical_layout.addWidget(self._footer_area)

    def _setup_header_area(self) -> None:
        self._header_area: QWidget = QWidget(self)
        self._header_area.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Minimum)
        self._header_area.setMinimumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                         view_size_constants.VIEW_EDIT_PAGE_HEADER_AREA_HEIGHT)

        header_area_layout: QHBoxLayout = QHBoxLayout(self._header_area)
        header_area_layout.setSizeConstraint(QLayout.SetMinimumSize)
        header_area_layout.setContentsMargins(
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_TOPBOTTOM + view_size_constants.VIEW_EDIT_PAGE_MARGIN_TOPBOTTOM,
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_TOPBOTTOM)

        config_file_label: QLabel = QLabel(self._header_area)
        config_file_label.setText(notification_label_text.VIEW_EDIT_PAGE_CONFIG_FILE_TEXT)
        config_file_label_font: QFont = QFont()
        config_file_label_font.setBold(True)
        config_file_label.setFont(config_file_label_font)
        config_file_label.setMinimumSize(view_size_constants.CONFIG_FILE_LABEL_WIDTH,
                                         view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        header_area_layout.addWidget(config_file_label)
        
        self._config_file_combobox: QComboBox = QComboBox(self._header_area)
        self._config_file_combobox.setLineEdit(QLineEdit())
        self._config_file_combobox.lineEdit().setPlaceholderText(
            notification_label_text.VIEW_EDIT_PAGE_CONFIG_FILES_PLACEHOLDER_TEXT.format(0))
        self._config_file_combobox.lineEdit().setReadOnly(True)
        self._config_file_combobox.setMinimumSize(view_size_constants.CONFIG_FILE_COMBOBOX_WIDTH,
                                                  view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        self._config_file_combobox.setCurrentIndex(-1)
        header_area_layout.addWidget(self._config_file_combobox)

        header_area_spacer: QSpacerItem = QSpacerItem(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                                      view_size_constants.INTERACTION_COMPONENT_HEIGHT,
                                                      QSizePolicy.Expanding, QSizePolicy.Minimum)
        header_area_layout.addItem(header_area_spacer)
        
        config_location_label: QLabel = QLabel(self._header_area)
        config_location_label.setText(notification_label_text.VIEW_EDIT_PAGE_CONFIG_LOCATION_TEXT)
        config_location_label_font: QFont = QFont()
        config_location_label_font.setBold(True)
        config_location_label.setFont(config_location_label_font)
        config_location_label.setMinimumSize(view_size_constants.CONFIG_LOCATION_LABEL_WIDTH,
                                             view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        header_area_layout.addWidget(config_location_label)
        
        self._config_location_text: QLabel = QLabel(self._header_area)
        self._config_location_text.setMinimumSize(view_size_constants.CONFIG_LOCATION_TEXT_WIDTH,
                                                  view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        header_area_layout.addWidget(self._config_location_text)

        self._config_location_button: QPushButton = QPushButton(self._header_area)
        self._config_location_button.setIcon(QIcon(":/Gallery/Folder.svg"))
        self._config_location_button.setFlat(True)
        self._config_location_button.setMinimumSize(view_size_constants.INTERACTION_COMPONENT_HEIGHT,
                                                    view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        header_area_layout.addWidget(self._config_location_button)

    def _setup_center_area(self) -> None:
        self._stacked_pages: QStackedWidget = QStackedWidget(self)
        self._stacked_pages.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self._stacked_pages.setMaximumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                           view_size_constants.VIEW_EDIT_PAGE_CENTER_AREA_HEIGHT)
        self._stacked_pages.setContentsMargins(view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT, 0,
                                               view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT, 0)

        self._setup_notification_page()
        self._stacked_pages.addWidget(self._notification_page_frame)

        self._setup_table_view_page()
        self._stacked_pages.addWidget(self._table_view_page)

    def _setup_notification_page(self) -> None:
        self._notification_page_frame: QFrame = \
            NotificationFrame(self, QPixmap(":/stylesheet/img/32x32/info.png"),
                              notification_label_text.NOTIFICATION_LOADING_MESSAGE, False)
        self._notification_page_frame.setFixedSize(
            (view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH
                - 2 * view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT),
            view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        self._notification_page_frame.setVisible(True)

    def _setup_table_view_page(self) -> None:
        self._table_view_page: QWidget = QWidget(self)
        table_view_vertical_layout: QVBoxLayout = QVBoxLayout(self._table_view_page)
        table_view_vertical_layout.setSizeConstraint(QLayout.SetMinimumSize)
        table_view_vertical_layout.setContentsMargins(0, 0, 0, 0)

        interaction_area: QWidget = QWidget(self._table_view_page)
        interaction_area.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Minimum)
        interaction_area.setMinimumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                        view_size_constants.TABLE_VIEW_PAGE_INTERACTION_AREA_HEIGHT)

        interaction_area_layout: QHBoxLayout = QHBoxLayout(interaction_area)
        interaction_area_layout.setSizeConstraint(QLayout.SetMinimumSize)
        interaction_area_layout.setContentsMargins(0, view_size_constants.VIEW_EDIT_PAGE_MARGIN_TOPBOTTOM,
                                                   0, view_size_constants.VIEW_EDIT_PAGE_MARGIN_TOPBOTTOM)

        self._add_row_button: QPushButton = QPushButton(interaction_area)
        self._add_row_button.setObjectName("Primary")
        self._add_row_button.setText(notification_label_text.VIEW_EDIT_PAGE_ADD_ROW_TEXT)
        self._add_row_button.setMinimumSize(view_size_constants.ADD_ROW_BUTTON_WIDTH,
                                            view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        interaction_area_layout.addWidget(self._add_row_button)

        self._delete_row_button: QPushButton = QPushButton(interaction_area)
        self._delete_row_button.setObjectName("Primary")
        self._delete_row_button.setText(notification_label_text.VIEW_EDIT_PAGE_DELETE_ROW_TEXT)
        self._delete_row_button.setMinimumSize(view_size_constants.DELETE_ROW_BUTTON_WIDTH,
                                               view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        interaction_area_layout.addWidget(self._delete_row_button)

        interaction_area_separator_label: QLabel = QLabel(interaction_area)
        interaction_area_separator_label.setPixmap(QPixmap(":/stylesheet/img/separator.png"))
        interaction_area_layout.addWidget(interaction_area_separator_label)

        self._import_resources_combobox: QComboBox = QComboBox(interaction_area)
        self._import_resources_combobox.setLineEdit(QLineEdit())
        self._import_resources_combobox.lineEdit().setPlaceholderText(
            notification_label_text.VIEW_EDIT_PAGE_IMPORT_RESOURCES_PLACEHOLDER_TEXT)
        self._import_resources_combobox.lineEdit().setReadOnly(True)
        self._import_resources_combobox.setMinimumSize(view_size_constants.IMPORT_RESOURCES_COMBOBOX_WIDTH,
                                                       view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        self._import_resources_combobox.addItem(constants.SEARCH_TYPED_RESOURCES_VERSION)
        self._import_resources_combobox.addItem(constants.SEARCH_CFN_STACKS_VERSION)
        self._import_resources_combobox.setCurrentIndex(-1)
        interaction_area_layout.addWidget(self._import_resources_combobox)

        interaction_area_spacer: QSpacerItem = QSpacerItem(
            view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH, view_size_constants.INTERACTION_COMPONENT_HEIGHT,
            QSizePolicy.Expanding, QSizePolicy.Minimum)
        interaction_area_layout.addItem(interaction_area_spacer)

        self._search_filter_line_edit: SearchFilterLineEdit = \
            SearchFilterLineEdit(interaction_area, notification_label_text.VIEW_EDIT_PAGE_SEARCH_PLACEHOLDER_TEXT)
        self._search_filter_line_edit.setMinimumSize(view_size_constants.TABLE_VIEW_PAGE_SEARCH_FILTER_WIDGET_WIDTH,
                                                     view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        interaction_area_layout.addWidget(self._search_filter_line_edit)
        table_view_vertical_layout.addWidget(interaction_area)

        self._table_view: ResourceTableView = ResourceTableView(self._table_view_page)
        self._table_view.setAlternatingRowColors(True)
        self._table_view.setMaximumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                        view_size_constants.TABLE_VIEW_PAGE_TABLE_VIEW_AREA_HEIGHT)
        self._table_view.setSelectionBehavior(QAbstractItemView.SelectRows)
        self._table_view.setSortingEnabled(True)
        self._table_view.verticalHeader().hide()

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
        self._footer_area.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Minimum)
        self._footer_area.setMinimumSize(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                         view_size_constants.VIEW_EDIT_PAGE_FOOTER_AREA_HEIGHT)

        footer_area_layout: QHBoxLayout = QHBoxLayout(self._footer_area)
        footer_area_layout.setSizeConstraint(QLayout.SetMinimumSize)
        footer_area_layout.setContentsMargins(
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_TOPBOTTOM,
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_LEFTRIGHT,
            view_size_constants.MAIN_PAGE_LAYOUT_MARGIN_TOPBOTTOM)

        footer_area_spacer: QSpacerItem = QSpacerItem(view_size_constants.TOOL_APPLICATION_MAIN_WINDOW_WIDTH,
                                                      view_size_constants.INTERACTION_COMPONENT_HEIGHT,
                                                      QSizePolicy.Expanding, QSizePolicy.Minimum)
        footer_area_layout.addItem(footer_area_spacer)

        self._save_changes_button: QPushButton = QPushButton(self._footer_area)
        self._save_changes_button.setObjectName("Primary")
        self._save_changes_button.setText(notification_label_text.VIEW_EDIT_PAGE_SAVE_CHANGES_TEXT)
        self._save_changes_button.setMinimumSize(view_size_constants.SAVE_CHANGES_BUTTON_WIDTH,
                                                 view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        footer_area_layout.addWidget(self._save_changes_button)

        self._create_new_button: QPushButton = QPushButton(self._footer_area)
        self._create_new_button.setObjectName("Primary")
        self._create_new_button.setText(notification_label_text.VIEW_EDIT_PAGE_CREATE_NEW_TEXT)
        self._create_new_button.setMinimumSize(view_size_constants.CREATE_NEW_BUTTON_WIDTH,
                                               view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        footer_area_layout.addWidget(self._create_new_button)

        self._rescan_button: QPushButton = QPushButton(self._footer_area)
        self._rescan_button.setObjectName("Secondary")
        self._rescan_button.setText(notification_label_text.VIEW_EDIT_PAGE_RESCAN_TEXT)
        self._rescan_button.setMinimumSize(view_size_constants.RESCAN_BUTTON_WIDTH,
                                           view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        footer_area_layout.addWidget(self._rescan_button)

        self._cancel_button: QPushButton = QPushButton(self._footer_area)
        self._cancel_button.setObjectName("Secondary")
        self._cancel_button.setText(notification_label_text.VIEW_EDIT_PAGE_CANCEL_TEXT)
        self._cancel_button.setMinimumSize(view_size_constants.CANCEL_BUTTON_WIDTH,
                                           view_size_constants.INTERACTION_COMPONENT_HEIGHT)
        footer_area_layout.addWidget(self._cancel_button)

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
        return self._search_filter_line_edit

    @property
    def create_new_button(self) -> QPushButton:
        return self._create_new_button

    @property
    def rescan_button(self) -> QPushButton:
        return self._rescan_button

    @property
    def notification_frame(self) -> NotificationFrame:
        return self._notification_frame

    @property
    def notification_page_frame(self) -> NotificationFrame:
        return self._notification_page_frame

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

        self._config_file_combobox.lineEdit().setPlaceholderText(
            notification_label_text.VIEW_EDIT_PAGE_CONFIG_FILES_PLACEHOLDER_TEXT.format(len(config_files)))
        self._config_file_combobox.setCurrentIndex(-1)

        if config_files:
            self._notification_page_frame.set_frame_text_receiver(
                notification_label_text.VIEW_EDIT_PAGE_SELECT_CONFIG_FILE_MESSAGE)
            self._create_new_button.setVisible(False)
            self._rescan_button.setVisible(False)
        else:
            self._notification_page_frame.set_frame_text_receiver(
                notification_label_text.VIEW_EDIT_PAGE_NO_CONFIG_FILE_FOUND_MESSAGE)
            self._create_new_button.setVisible(True)
            self._rescan_button.setVisible(True)

        self._config_file_combobox.blockSignals(False)

    def set_config_location(self, config_location: str) -> None:
        """Set config file location in text label"""
        self._config_location_text.clear()

        metrics: QFontMetrics = QFontMetrics(self._config_location_text.font())
        elided_text: str = metrics.elidedText(config_location, Qt.ElideMiddle, self._config_location_text.width())
        self._config_location_text.setText(elided_text)
        self._config_location_text.setToolTip(config_location)

    def set_table_view_page_interactions_enabled(self, enabled: bool) -> None:
        self._table_view_page.setEnabled(enabled)
        self._save_changes_button.setEnabled(enabled)
