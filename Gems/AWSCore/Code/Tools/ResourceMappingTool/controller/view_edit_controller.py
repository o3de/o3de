"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
from PySide2.QtCore import (QCoreApplication, QModelIndex, QObject, Signal, Slot)
from PySide2.QtWidgets import QFileDialog
from typing import (Dict, List)

from manager.configuration_manager import ConfigurationManager
from manager.thread_manager import ThreadManager
from manager.view_manager import ViewManager
from model import (constants, error_messages, notification_label_text)
from model.configuration import Configuration
from model.basic_resource_attributes import BasicResourceAttributes
from model.resource_mapping_attributes import (ResourceMappingAttributes, ResourceMappingAttributesBuilder,
                                               ResourceMappingAttributesStatus)
from model.resource_proxy_model import ResourceProxyModel
from multithread.worker import FunctionWorker
from utils import file_utils
from utils import json_utils
from view.view_edit_page import (ResourceTableView, ViewEditPage, ViewEditPageConstants)

logger = logging.getLogger(__name__)


class ViewEditController(QObject):
    set_notification_frame_text_sender: Signal = Signal(str)
    set_notification_page_frame_text_sender: Signal = Signal(str)

    """
    ViewEditController is the place to bind ViewEdit view with its
    corresponding behavior
    """
    def __init__(self) -> None:
        super(ViewEditController, self).__init__()
        # Initialize in memory dict to store content reading from json file
        self._config_file_json_source: Dict[str, any] = {}
        # Initialize manager references
        self._configuration_manager: ConfigurationManager = ConfigurationManager.get_instance()
        self._view_manager: ViewManager = ViewManager.get_instance()
        # Initialize view and model related references
        self._view_edit_page: ViewEditPage = self._view_manager.get_view_edit_page()
        self.set_notification_frame_text_sender.connect(
            self._view_edit_page.notification_frame.set_frame_text_receiver)
        self.set_notification_page_frame_text_sender.connect(
            self._view_edit_page.notification_page_frame.set_frame_text_receiver)
        self._table_view: ResourceTableView = self._view_edit_page.table_view
        self._proxy_model: ResourceProxyModel = self._table_view.resource_proxy_model

    def _add_table_row(self) -> None:
        configuration: Configuration = self._configuration_manager.configuration
        resource_builder: ResourceMappingAttributesBuilder = ResourceMappingAttributesBuilder() \
            .build_account_id(configuration.account_id) \
            .build_region(configuration.region) \
            .build_status(
                ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.MODIFIED_STATUS_VALUE,
                                                [ResourceMappingAttributesStatus.MODIFIED_STATUS_DESCRIPTION]))
        self._proxy_model.add_resource(resource_builder.build())

    def _cancel(self) -> None:
        QCoreApplication.instance().quit()

    def _convert_and_write_to_json(self, config_file_name: str) -> bool:
        # convert model resources into json dict
        json_dict: Dict[str, any] = \
            json_utils.convert_resources_to_json_dict(self._proxy_model.get_resources(), self._config_file_json_source)

        if json_dict == self._config_file_json_source:
            # skip because no difference found against existing json file
            return True

        # try to write in memory json content into json file
        try:
            configuration: Configuration = self._configuration_manager.configuration
            config_file_full_path: str = file_utils.join_path(configuration.config_directory, config_file_name)
            json_utils.write_into_json_file(config_file_full_path, json_dict)
            self._config_file_json_source = json_dict
            return True
        except IOError as e:
            logger.exception(e)
            self.set_notification_frame_text_sender.emit(str(e))
            return False

    def _convert_and_load_into_model(self, config_file_name: str) -> None:
        # convert json file into resources
        configuration: Configuration = self._configuration_manager.configuration
        resources: List[ResourceMappingAttributes] = []
        try:
            config_file_full_path: str = file_utils.join_path(configuration.config_directory, config_file_name)
            self._config_file_json_source = json_utils.read_from_json_file(config_file_full_path)
            json_utils.validate_json_dict_according_to_json_schema(self._config_file_json_source)
            resources = json_utils.convert_json_dict_to_resources(self._config_file_json_source)
        except (IOError, ValueError, KeyError) as e:
            logger.exception(e)
            self.set_notification_frame_text_sender.emit(str(e))
            self._view_edit_page.set_table_view_page_interactions_enabled(False)

        # load resources into model
        resource: ResourceMappingAttributes
        for resource in resources:
            self._proxy_model.load_resource(resource)

    def _delete_table_row(self) -> None:
        indices: List[QModelIndex] = self._table_view.selectedIndexes()
        self._proxy_model.remove_resources(indices)

    def _filter_based_on_search_text(self):
        self._table_view.clear_selection()
        self._proxy_model.filter_text = self._view_edit_page.search_filter_input.text()

    def _list_config_files_callback(self, config_files: List[str]) -> None:
        if config_files:
            self._configuration_manager.configuration.config_files = config_files
            self._view_edit_page.set_config_files(config_files)
        else:
            self._configuration_manager.configuration.config_files = []
            self._view_edit_page.set_config_files([])

    def _open_file_dialog(self) -> None:
        configuration: Configuration = self._configuration_manager.configuration
        current_config_directory: str = configuration.config_directory
        new_config_directory = \
            QFileDialog.getExistingDirectory(None, "Config Location",
                                             current_config_directory, QFileDialog.ShowDirsOnly)
        if not new_config_directory:
            return

        if not current_config_directory == new_config_directory:
            try:
                configuration.config_directory = new_config_directory
                self._start_search_config_files_async(new_config_directory)
            except RuntimeError as e:
                logger.exception(e)
                self.set_notification_frame_text_sender.emit(str(e))

    def _rescan_config_directory(self) -> None:
        configuration: Configuration = self._configuration_manager.configuration
        config_files: List[str] = []
        try:
            config_files = file_utils.find_files_with_suffix_under_directory(
                configuration.config_directory, constants.RESOURCE_MAPPING_CONFIG_FILE_NAME_SUFFIX)
        except FileNotFoundError as e:
            logger.exception(e)
            self.set_notification_frame_text_sender.emit(str(e))
            return

        self._configuration_manager.configuration.config_files = config_files
        self._view_edit_page.set_config_files(config_files)

    def _reset_page(self) -> None:
        self._view_edit_page.notification_frame.setVisible(False)
        self._view_edit_page.set_table_view_page_interactions_enabled(True)

    def _save_changes(self) -> None:
        if not self._validate_resources_and_post_notification():
            # there is invalid resources, stop saving
            return

        config_file: str = self._view_edit_page.config_file_combobox.currentText()
        if not self._convert_and_write_to_json(config_file):
            # failed to convert/write into json file, stop saving
            return

        self._proxy_model.override_all_resources_status(
            ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE,
                                            [ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE]))
        self.set_notification_frame_text_sender.emit(
            notification_label_text.VIEW_EDIT_PAGE_SAVING_SUCCEED_MESSAGE.format(config_file))

    def _search_complete_callback(self) -> None:
        self._reset_page()

    def _start_search_config_files_async(self, config_directory: str) -> None:
        self.set_notification_page_frame_text_sender.emit(notification_label_text.NOTIFICATION_LOADING_MESSAGE)
        self._view_edit_page.set_current_main_view_index(ViewEditPageConstants.NOTIFICATION_PAGE_INDEX)
        self._config_file_json_source.clear()
        self._table_view.reset_view()
        self._view_edit_page.set_config_location(config_directory)

        async_worker: FunctionWorker = FunctionWorker(self._search_config_files_callback, config_directory)
        async_worker.signals.result.connect(self._list_config_files_callback)
        async_worker.signals.finished.connect(self._search_complete_callback)
        ThreadManager.get_instance().start(async_worker)

    def _search_config_files_callback(self, config_directory: str) -> List[str]:
        try:
            return file_utils.find_files_with_suffix_under_directory(
                config_directory, constants.RESOURCE_MAPPING_CONFIG_FILE_NAME_SUFFIX)
        except FileNotFoundError as e:
            logger.exception(e)
            self.set_notification_frame_text_sender.emit(str(e))

    def _select_config_file(self) -> None:
        if self._view_edit_page.config_file_combobox.currentIndex() == -1:
            return  # return when combobox index is default -1

        self._reset_page()
        config_file_name: str = self._view_edit_page.config_file_combobox.currentText()
        configuration: Configuration = self._configuration_manager.configuration
        if config_file_name in configuration.config_files:  # sanity check config file name
            self._table_view.reset_view()
            self._convert_and_load_into_model(config_file_name)
            self._proxy_model.emit_source_model_layout_changed()
            if not self._validate_resources_and_post_notification():
                self._proxy_model.emit_source_model_layout_changed()
        else:
            self._view_edit_page.set_table_view_page_interactions_enabled(False)
            self.set_notification_frame_text_sender.emit(
                error_messages.VIEW_EDIT_PAGE_READ_FROM_JSON_FAILED_WITH_UNEXPECTED_FILE_ERROR_MESSAGE.format(config_file_name))
        self._view_edit_page.set_current_main_view_index(ViewEditPageConstants.TABLE_VIEW_PAGE_INDEX)

    def _setup_page_interactions_behavior(self) -> None:
        self._view_edit_page.config_file_combobox.currentIndexChanged.connect(self._select_config_file)
        self._view_edit_page.config_location_button.clicked.connect(self._open_file_dialog)
        self._view_edit_page.add_row_button.clicked.connect(self._add_table_row)
        self._view_edit_page.delete_row_button.clicked.connect(self._delete_table_row)
        self._view_edit_page.import_resources_combobox.currentIndexChanged.connect(self._switch_to_import_resources_page)
        self._view_edit_page.save_changes_button.clicked.connect(self._save_changes)
        self._view_edit_page.search_filter_input.returnPressed.connect(self._filter_based_on_search_text)
        self._view_edit_page.cancel_button.clicked.connect(self._cancel)
        self._view_edit_page.rescan_button.clicked.connect(self._rescan_config_directory)

    def _setup_page_start_state(self) -> None:
        configuration: Configuration = self._configuration_manager.configuration
        self._view_edit_page.set_current_main_view_index(ViewEditPageConstants.NOTIFICATION_PAGE_INDEX)
        self._view_edit_page.set_config_location(configuration.config_directory)
        self._view_edit_page.set_config_files(configuration.config_files)

    def _switch_to_import_resources_page(self) -> None:
        if self._view_edit_page.import_resources_combobox.currentIndex() == -1:
            return  # return when combobox index is default -1

        import_resources_search_version: str = self._view_edit_page.import_resources_combobox.currentText()
        self._view_manager.switch_to_import_resources_page(import_resources_search_version)
        self._view_edit_page.import_resources_combobox.setCurrentIndex(-1)

    def _validate_resources_and_post_notification(self) -> bool:
        invalid_sources: Dict[int, List[str]] = \
            json_utils.validate_resources_according_to_json_schema(self._proxy_model.get_resources())
        if invalid_sources:
            invalid_row: int
            invalid_details: List[str]
            for invalid_row, invalid_details in invalid_sources.items():
                self._proxy_model.override_resource_status(
                    invalid_row,
                    ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.FAILURE_STATUS_VALUE,
                                                    invalid_details))
            
            invalid_proxy_rows: List[int] = self._proxy_model.map_from_source_rows(list(invalid_sources.keys()))
            self.set_notification_frame_text_sender.emit(
                error_messages.VIEW_EDIT_PAGE_SAVING_FAILED_WITH_INVALID_ROW_ERROR_MESSAGE.format(invalid_proxy_rows))
            return False
        return True

    @Slot(list)
    def add_import_resources_receiver(self, resources: List[BasicResourceAttributes]) -> None:
        resource: BasicResourceAttributes
        for resource in resources:
            resource_builder: ResourceMappingAttributesBuilder = ResourceMappingAttributesBuilder() \
                .build_type(resource.type) \
                .build_name_id(resource.name_id) \
                .build_account_id(resource.account_id) \
                .build_region(resource.region) \
                .build_status(
                    ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.MODIFIED_STATUS_VALUE,
                                                    [ResourceMappingAttributesStatus.MODIFIED_STATUS_DESCRIPTION]))
            self._proxy_model.add_resource(resource_builder.build())

    def setup(self) -> None:
        """Setting view edit page starting state and bind interactions with its corresponding behavior"""
        self._setup_page_interactions_behavior()
        self._setup_page_start_state()
