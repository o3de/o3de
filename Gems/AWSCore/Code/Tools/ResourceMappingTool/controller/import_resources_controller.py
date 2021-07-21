"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
from typing import (Dict, List)
from PySide2.QtCore import (QObject, Signal)

from manager.configuration_manager import ConfigurationManager
from manager.thread_manager import ThreadManager
from manager.view_manager import ViewManager
from model import (constants, error_messages)
from model.configuration import Configuration
from model.basic_resource_attributes import (BasicResourceAttributes, BasicResourceAttributesBuilder)
from model.resource_proxy_model import ResourceProxyModel
from multithread.worker import FunctionWorker
from utils import aws_utils
from view.import_resources_page import (ImportResourcesPage, ImportResourcesPageConstants, ResourceTreeView)

logger = logging.getLogger(__name__)


class ImportResourcesController(QObject):
    add_import_resources_sender: Signal = Signal(list)
    set_notification_frame_text_sender: Signal = Signal(str)

    """
    ImportResourcesController is the place to bind ImportResource view with its
    corresponding behavior
    """

    def __init__(self) -> None:
        super(ImportResourcesController, self).__init__()
        # Initialize manager references
        self._configuration_manager: ConfigurationManager = ConfigurationManager.get_instance()
        self._view_manager: ViewManager = ViewManager.get_instance()
        # Initialize view and model related references
        self._import_resources_page: ImportResourcesPage = self._view_manager.get_import_resources_page()
        self.set_notification_frame_text_sender.connect(
            self._import_resources_page.notification_frame.set_frame_text_receiver)
        self._tree_view: ResourceTreeView = self._import_resources_page.tree_view
        self._proxy_model: ResourceProxyModel = self._tree_view.resource_proxy_model

    def _back_to_view_edit_page(self) -> None:
        self._view_manager.switch_to_view_edit_page()
        self.reset_page()

    def _filter_based_on_search_text(self):
        self._tree_view.clear_selection()
        self._proxy_model.filter_text = self._import_resources_page.search_filter_input.text()

    def _import_resources(self) -> None:
        unique_resources: List[BasicResourceAttributes] = \
            self._proxy_model.deduplicate_selected_import_resources(self._tree_view.selectedIndexes())
        if unique_resources:
            logger.debug(f"Importing selected resources: {unique_resources} ...")
            self.add_import_resources_sender.emit(unique_resources)
            self._back_to_view_edit_page()
        else:
            self.set_notification_frame_text_sender.emit(error_messages.IMPORT_RESOURCES_PAGE_NO_RESOURCES_SELECTED_ERROR_MESSAGE)

    def _start_search_resources_async(self) -> None:
        configuration: Configuration = self._configuration_manager.configuration

        async_worker: FunctionWorker
        if self._import_resources_page.search_version == constants.SEARCH_TYPED_RESOURCES_VERSION:
            async_worker = FunctionWorker(self._request_typed_resources_callback, configuration.region)
            async_worker.signals.result.connect(self._load_typed_resources_callback)
        elif self._import_resources_page.search_version == constants.SEARCH_CFN_STACKS_VERSION:
            async_worker = FunctionWorker(self._request_cfn_resources_callback, configuration.region)
            async_worker.signals.result.connect(self._load_cfn_resources_callback)
        else:
            self.set_notification_frame_text_sender.emit(error_messages.IMPORT_RESOURCES_PAGE_SEARCH_VERSION_ERROR_MESSAGE)
            return

        self._tree_view.reset_view()
        self._import_resources_page.set_current_main_view_index(ImportResourcesPageConstants.NOTIFICATION_PAGE_INDEX)
        async_worker.signals.finished.connect(self._search_complete_callback)
        ThreadManager.get_instance().start(async_worker)

    def _request_cfn_resources_callback(self, region: str) -> Dict[str, List[BasicResourceAttributes]]:
        resources: Dict[str, List[BasicResourceAttributes]] = {}
        logger.debug(f"Requesting CFN stacks with Region={region} ...")

        try:
            stack_names: List[str] = aws_utils.list_cloudformation_stacks(region)
            stack_name: str
            for stack_name in stack_names:
                logger.debug(f"Requesting CFN stack resources with StackName={stack_name}, Region={region} ...")
                resource_type_and_names: List[BasicResourceAttributes] = \
                    aws_utils.list_cloudformation_stack_resources(stack_name, region)
                resources[stack_name] = resource_type_and_names
            return resources
        except RuntimeError as e:
            self.set_notification_frame_text_sender.emit(str(e))

    def _request_typed_resources_callback(self, region: str) -> List[str]:
        resource_type_index: int = self._import_resources_page.typed_resources_combobox.currentIndex()
        resources: List[str] = []
        logger.debug(f"Requesting resources with ResourceTypeIndex={resource_type_index}, Region={region} ...")

        try:
            if resource_type_index == constants.AWS_RESOURCE_LAMBDA_FUNCTION_INDEX:
                resources = aws_utils.list_lambda_functions(region)
            elif resource_type_index == constants.AWS_RESOURCE_DYNAMODB_TABLE_INDEX:
                resources = aws_utils.list_dynamodb_tables(region)
            elif resource_type_index == constants.AWS_RESOURCE_S3_BUCKET_INDEX:
                resources = aws_utils.list_s3_buckets(region)
            else:
                self.set_notification_frame_text_sender.emit(error_messages.IMPORT_RESOURCES_PAGE_RESOURCE_TYPE_ERROR_MESSAGE)

            return resources
        except RuntimeError as e:
            self.set_notification_frame_text_sender.emit(str(e))

    def _load_cfn_resources_callback(self, resources: Dict[str, List[BasicResourceAttributes]]) -> None:
        if not resources:
            logger.debug("No resource found")
            return

        configuration: Configuration = self._configuration_manager.configuration

        stack_name: str
        resource_type_and_names: List[BasicResourceAttributes]
        for stack_name, resource_type_and_names in resources.items():
            if not resource_type_and_names:
                continue

            # loading cloudformation stack resource data into model
            logger.debug(f"Loading CFN stack {stack_name} into resource model ...")
            stack_resource_attributes = BasicResourceAttributesBuilder() \
                .build_type(constants.AWS_RESOURCE_CLOUDFORMATION_STACK_TYPE) \
                .build_name_id(stack_name) \
                .build_account_id(configuration.account_id) \
                .build_region(configuration.region) \
                .build()
            self._proxy_model.load_resource(stack_resource_attributes)

            # loading all resources data under cloudformation stack into model
            resource_entry: BasicResourceAttributes
            for resource_entry in resource_type_and_names:
                logger.debug(f"Loading resource Type={resource_entry.type}, "
                             f"NameId={resource_entry.name_id} into resource model ...")
                resource_entry.account_id = configuration.account_id
                resource_entry.region = configuration.region
                self._proxy_model.load_resource(resource_entry)

    def _load_typed_resources_callback(self, resources: List[str]) -> None:
        if not resources:
            logger.debug("No resource found")
            return

        resource_type_index: int = self._import_resources_page.typed_resources_combobox.currentIndex()
        configuration: Configuration = self._configuration_manager.configuration
        resource_name_id: str
        for resource_name_id in resources:
            logger.debug(f"Converting resource {resource_name_id} into resource model ...")
            import_resource_attributes: BasicResourceAttributes = BasicResourceAttributesBuilder() \
                .build_type(constants.AWS_RESOURCE_TYPES[resource_type_index]) \
                .build_name_id(resource_name_id) \
                .build_region(configuration.region) \
                .build_account_id(configuration.account_id) \
                .build()
            self._proxy_model.load_resource(import_resource_attributes)

    def _search_complete_callback(self) -> None:
        self._proxy_model.emit_source_model_layout_changed()
        self._import_resources_page.set_current_main_view_index(ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)

    def reset_page(self):
        """Reset import resources page to its default state"""
        self._tree_view.reset_view()
        self._import_resources_page.notification_frame.setVisible(False)
        self._import_resources_page.set_current_main_view_index(ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)
        self._import_resources_page.typed_resources_combobox.setCurrentIndex(-1)
        self._import_resources_page.search_version = None

    def setup(self):
        """Binding import resources page interactions with its corresponding behavior"""
        self._import_resources_page.back_button.clicked.connect(self._back_to_view_edit_page)
        self._import_resources_page.search_filter_input.returnPressed.connect(self._filter_based_on_search_text)
        self._import_resources_page.typed_resources_search_button.clicked.connect(self._start_search_resources_async)
        self._import_resources_page.typed_resources_import_button.clicked.connect(self._import_resources)
        self._import_resources_page.cfn_stacks_search_button.clicked.connect(self._start_search_resources_async)
        self._import_resources_page.cfn_stacks_import_button.clicked.connect(self._import_resources)
