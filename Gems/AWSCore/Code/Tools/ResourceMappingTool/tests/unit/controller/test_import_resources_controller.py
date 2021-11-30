"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from typing import List
from unittest import TestCase
from unittest.mock import (call, MagicMock, patch)

from controller.import_resources_controller import ImportResourcesController
from model import constants
from model.basic_resource_attributes import (BasicResourceAttributes, BasicResourceAttributesBuilder)
from view.import_resources_page import ImportResourcesPageConstants


class TestImportResourcesController(TestCase):
    """
    ImportResourcesController unit test cases
    TODO: add test cases once error handling is ready
    """
    _expected_account_id: str = "1234567890"
    _expected_region: str = "aws-global"

    _expected_lambda_name: str = "TestLambda"
    _expected_lambda_resource: BasicResourceAttributes = BasicResourceAttributesBuilder() \
        .build_type(constants.AWS_RESOURCE_TYPES[constants.AWS_RESOURCE_LAMBDA_FUNCTION_INDEX]) \
        .build_name_id(_expected_lambda_name) \
        .build_region(_expected_region) \
        .build_account_id(_expected_account_id) \
        .build()

    _expected_cfn_stack_name: str = "TestStack"
    _expected_cfn_stack_resource: BasicResourceAttributes = BasicResourceAttributesBuilder() \
        .build_type(constants.AWS_RESOURCE_CLOUDFORMATION_STACK_TYPE) \
        .build_name_id(_expected_cfn_stack_name) \
        .build_account_id(_expected_account_id) \
        .build_region(_expected_region) \
        .build()

    def setUp(self) -> None:
        configuration_manager_patcher: patch = patch("controller.import_resources_controller.ConfigurationManager")
        self.addCleanup(configuration_manager_patcher.stop)
        self._mock_configuration_manager: MagicMock = configuration_manager_patcher.start()

        view_manager_patcher: patch = patch("controller.import_resources_controller.ViewManager")
        self.addCleanup(view_manager_patcher.stop)
        self._mock_view_manager: MagicMock = view_manager_patcher.start()

        self._mocked_configuration_manager: MagicMock = self._mock_configuration_manager.get_instance.return_value
        self._mocked_configuration_manager.configuration.account_id = TestImportResourcesController._expected_account_id
        self._mocked_configuration_manager.configuration.region = TestImportResourcesController._expected_region

        self._mocked_view_manager: MagicMock = self._mock_view_manager.get_instance.return_value
        self._mocked_import_resources_page: MagicMock = self._mocked_view_manager.get_import_resources_page.return_value
        self._mocked_tree_view: MagicMock = self._mocked_import_resources_page.tree_view
        self._mocked_proxy_model: MagicMock = self._mocked_tree_view.resource_proxy_model

        self._test_import_resources_controller: ImportResourcesController = ImportResourcesController()
        self._test_import_resources_controller.add_import_resources_sender = MagicMock()
        self._test_import_resources_controller.set_notification_frame_text_sender = MagicMock()
        self._test_import_resources_controller.setup()

    def test_reset_page_resetting_page_with_expected_state(self) -> None:
        self._test_import_resources_controller.reset_page()

        self._mocked_tree_view.reset_view.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_once_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)
        self._mocked_import_resources_page.typed_resources_combobox.setCurrentIndex.assert_called_once_with(-1)
        assert self._mocked_import_resources_page.search_version is None

    def test_setup_with_behavior_connected(self) -> None:
        self._mocked_import_resources_page.back_button.clicked.connect.assert_called_once_with(
            self._test_import_resources_controller._back_to_view_edit_page)
        self._mocked_import_resources_page.search_filter_input.returnPressed.connect.assert_called_once_with(
            self._test_import_resources_controller._filter_based_on_search_text)
        self._mocked_import_resources_page.typed_resources_search_button.clicked.connect.assert_called_once_with(
            self._test_import_resources_controller._start_search_resources_async)
        self._mocked_import_resources_page.typed_resources_import_button.clicked.connect.assert_called_once_with(
            self._test_import_resources_controller._import_resources)
        self._mocked_import_resources_page.cfn_stacks_search_button.clicked.connect.assert_called_once_with(
            self._test_import_resources_controller._start_search_resources_async)
        self._mocked_import_resources_page.cfn_stacks_import_button.clicked.connect.assert_called_once_with(
            self._test_import_resources_controller._import_resources)

    def test_page_back_button_switching_view_gets_invoked_and_resetting_page_with_expected_state(self) -> None:
        mocked_call_args: call = \
            self._mocked_import_resources_page.back_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering back_button connected function
        self._mocked_view_manager.switch_to_view_edit_page.assert_called_once()
        self._mocked_tree_view.reset_view.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_once_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)
        self._mocked_import_resources_page.typed_resources_combobox.setCurrentIndex.assert_called_once_with(-1)
        assert self._mocked_import_resources_page.search_version is None

    def test_page_search_filter_input_invoke_proxy_model_with_expected_filter_text(self) -> None:
        expected_filter_text: str = "dummySearchFilter"
        self._mocked_import_resources_page.search_filter_input.text.return_value = expected_filter_text
        mocked_call_args: call = \
            self._mocked_import_resources_page.search_filter_input.returnPressed.connect.call_args[0]

        mocked_call_args[0]()  # triggering search_filter_input connected function
        self._mocked_tree_view.clear_selection()
        assert self._mocked_proxy_model.filter_text == expected_filter_text

    def test_page_search_button_post_notification_when_search_version_is_unexpected(self) -> None:
        self._mocked_import_resources_page.search_version = "dummySearchVersion"
        mocked_call_args: call = \
            self._mocked_import_resources_page.typed_resources_search_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering search_button connected function
        self._test_import_resources_controller.set_notification_frame_text_sender.emit.assert_called_once()
        self._mocked_tree_view.reset_view.assert_not_called()
        self._mocked_import_resources_page.set_current_main_view_index.assert_not_called()

    @patch("controller.import_resources_controller.ThreadManager")
    def test_page_cfn_stacks_search_button_switch_page_with_expected_index_in_sync_process(
            self, mock_thread_manager: MagicMock) -> None:
        self._mocked_import_resources_page.search_version = constants.SEARCH_CFN_STACKS_VERSION
        mocked_call_args: call = \
            self._mocked_import_resources_page.cfn_stacks_search_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering cfn_stacks_search_button connected function
        self._mocked_tree_view.reset_view.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_once_with(
            ImportResourcesPageConstants.NOTIFICATION_PAGE_INDEX)
        mock_thread_manager.get_instance.return_value.start.assert_called_once()

    @patch("controller.import_resources_controller.ThreadManager")
    @patch("controller.import_resources_controller.aws_utils")
    def test_page_cfn_stacks_search_button_no_resource_loaded_when_search_cfn_stacks_returns_empty_in_async_process(
            self, mock_aws_utils: MagicMock, mock_thread_manager: MagicMock) -> None:
        self._mocked_import_resources_page.search_version = constants.SEARCH_CFN_STACKS_VERSION
        mocked_call_args: call = \
            self._mocked_import_resources_page.cfn_stacks_search_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering cfn_stacks_search_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        mock_aws_utils.list_cloudformation_stacks.return_value = []
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        mock_aws_utils.list_cloudformation_stacks.assert_called_once_with(
            TestImportResourcesController._expected_region)
        mock_aws_utils.list_cloudformation_stack_resources.assert_not_called()
        self._mocked_proxy_model.load_resource.assert_not_called()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)

    @patch("controller.import_resources_controller.ThreadManager")
    @patch("controller.import_resources_controller.aws_utils")
    def test_page_cfn_stacks_search_button_post_notification_when_search_cfn_stacks_raise_exception_in_async_process(
            self, mock_aws_utils: MagicMock, mock_thread_manager: MagicMock) -> None:
        self._mocked_import_resources_page.search_version = constants.SEARCH_CFN_STACKS_VERSION
        mocked_call_args: call = \
            self._mocked_import_resources_page.cfn_stacks_search_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering cfn_stacks_search_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        mock_aws_utils.list_cloudformation_stacks.side_effect = RuntimeError("dummyException")
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        mock_aws_utils.list_cloudformation_stacks.assert_called_once_with(
            TestImportResourcesController._expected_region)
        mock_aws_utils.list_cloudformation_stack_resources.assert_not_called()
        self._test_import_resources_controller.set_notification_frame_text_sender.emit.assert_called_once()
        self._mocked_proxy_model.load_resource.assert_not_called()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)

    @patch("controller.import_resources_controller.ThreadManager")
    @patch("controller.import_resources_controller.aws_utils")
    def test_page_cfn_stacks_search_button_post_notification_when_search_cfn_resources_raise_exception_in_async_process(
            self, mock_aws_utils: MagicMock, mock_thread_manager: MagicMock) -> None:
        self._mocked_import_resources_page.search_version = constants.SEARCH_CFN_STACKS_VERSION
        mocked_call_args: call = \
            self._mocked_import_resources_page.cfn_stacks_search_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering cfn_stacks_search_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        mock_aws_utils.list_cloudformation_stacks.return_value = \
            [TestImportResourcesController._expected_cfn_stack_name]
        mock_aws_utils.list_cloudformation_stack_resources.side_effect = RuntimeError("dummyException")
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        mock_aws_utils.list_cloudformation_stacks.assert_called_once_with(
            TestImportResourcesController._expected_region)
        mock_aws_utils.list_cloudformation_stack_resources.assert_called_once_with(
            TestImportResourcesController._expected_cfn_stack_name, TestImportResourcesController._expected_region)
        self._test_import_resources_controller.set_notification_frame_text_sender.emit.assert_called_once()
        self._mocked_proxy_model.load_resource.assert_not_called()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)

    @patch("controller.import_resources_controller.ThreadManager")
    @patch("controller.import_resources_controller.aws_utils")
    def test_page_cfn_stacks_search_button_no_resource_loaded_when_no_resource_found_under_cfn_stack_in_async_process(
            self, mock_aws_utils: MagicMock, mock_thread_manager: MagicMock) -> None:
        self._mocked_import_resources_page.search_version = constants.SEARCH_CFN_STACKS_VERSION
        mocked_call_args: call = \
            self._mocked_import_resources_page.cfn_stacks_search_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering cfn_stacks_search_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        mock_aws_utils.list_cloudformation_stacks.return_value = \
            [TestImportResourcesController._expected_cfn_stack_name]
        mock_aws_utils.list_cloudformation_stack_resources.return_value = []
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        mock_aws_utils.list_cloudformation_stacks.assert_called_once_with(
            TestImportResourcesController._expected_region)
        mock_aws_utils.list_cloudformation_stack_resources.assert_called_once_with(
            TestImportResourcesController._expected_cfn_stack_name, TestImportResourcesController._expected_region)
        self._mocked_proxy_model.load_resource.assert_not_called()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)

    @patch("controller.import_resources_controller.ThreadManager")
    @patch("controller.import_resources_controller.aws_utils")
    def test_page_cfn_stacks_search_button_expected_resource_loaded_when_resource_found_under_cfn_stack_in_async_process(
            self, mock_aws_utils: MagicMock, mock_thread_manager: MagicMock) -> None:
        self._mocked_import_resources_page.search_version = constants.SEARCH_CFN_STACKS_VERSION
        mocked_call_args: call = \
            self._mocked_import_resources_page.cfn_stacks_search_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering cfn_stacks_search_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        mock_aws_utils.list_cloudformation_stacks.return_value = \
            [TestImportResourcesController._expected_cfn_stack_name]
        mock_aws_utils.list_cloudformation_stack_resources.return_value = \
            [TestImportResourcesController._expected_lambda_resource]
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        mock_aws_utils.list_cloudformation_stacks.assert_called_once_with(
            TestImportResourcesController._expected_region)
        mock_aws_utils.list_cloudformation_stack_resources.assert_called_once_with(
            TestImportResourcesController._expected_cfn_stack_name, TestImportResourcesController._expected_region)
        expected_mocked_calls: List[call] = [call(TestImportResourcesController._expected_cfn_stack_resource),
                                             call(TestImportResourcesController._expected_lambda_resource)]
        self._mocked_proxy_model.load_resource.assert_has_calls(expected_mocked_calls)
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)

    def test_page_cfn_stacks_import_button_nothing_happened_when_selected_resources_are_empty(self) -> None:
        self._mocked_proxy_model.deduplicate_selected_import_resources.return_value = []
        mocked_call_args: call = \
            self._mocked_import_resources_page.cfn_stacks_import_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering cfn_stacks_import_button connected function
        self._test_import_resources_controller.add_import_resources_sender.emit.assert_not_called()
        self._test_import_resources_controller.set_notification_frame_text_sender.emit.assert_called_once()

    def test_page_cfn_stacks_import_button_emit_signal_with_expected_resources_and_switch_to_expected_page(self) -> None:
        self._mocked_proxy_model.deduplicate_selected_import_resources.return_value = \
            [TestImportResourcesController._expected_lambda_resource]
        mocked_call_args: call = \
            self._mocked_import_resources_page.cfn_stacks_import_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering cfn_stacks_import_button connected function
        self._test_import_resources_controller.add_import_resources_sender.emit.assert_called_once_with(
            [TestImportResourcesController._expected_lambda_resource])
        self._mocked_view_manager.switch_to_view_edit_page.assert_called_once()
        self._mocked_tree_view.reset_view.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_once_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)
        self._mocked_import_resources_page.typed_resources_combobox.setCurrentIndex.assert_called_once_with(-1)
        assert self._mocked_import_resources_page.search_version is None

    @patch("controller.import_resources_controller.ThreadManager")
    def test_page_typed_resources_search_button_switch_page_with_expected_index_in_sync_process(
            self, mock_thread_manager: MagicMock) -> None:
        self._mocked_import_resources_page.search_version = constants.SEARCH_TYPED_RESOURCES_VERSION
        mocked_call_args: call = \
            self._mocked_import_resources_page.typed_resources_search_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering typed_resources_search_button connected function
        self._mocked_tree_view.reset_view.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_once_with(
            ImportResourcesPageConstants.NOTIFICATION_PAGE_INDEX)
        mock_thread_manager.get_instance.return_value.start.assert_called_once()

    @patch("controller.import_resources_controller.ThreadManager")
    @patch("controller.import_resources_controller.aws_utils")
    def test_page_typed_resources_search_button_no_resource_loaded_when_search_lambda_returns_empty_in_async_process(
            self, mock_aws_utils: MagicMock, mock_thread_manager: MagicMock) -> None:
        self._mocked_import_resources_page.search_version = constants.SEARCH_TYPED_RESOURCES_VERSION
        mocked_call_args: call = \
            self._mocked_import_resources_page.typed_resources_search_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering typed_resources_search_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        self._mocked_import_resources_page.typed_resources_combobox.currentIndex.return_value = \
            constants.AWS_RESOURCE_LAMBDA_FUNCTION_INDEX
        mock_aws_utils.list_lambda_functions.return_value = []
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        mock_aws_utils.list_lambda_functions.assert_called_once_with(
            TestImportResourcesController._expected_region)
        self._mocked_proxy_model.load_resource.assert_not_called()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)

    @patch("controller.import_resources_controller.ThreadManager")
    @patch("controller.import_resources_controller.aws_utils")
    def test_page_typed_resources_search_button_no_resource_loaded_when_search_lambda_raise_exception_in_async_process(
            self, mock_aws_utils: MagicMock, mock_thread_manager: MagicMock) -> None:
        self._mocked_import_resources_page.search_version = constants.SEARCH_TYPED_RESOURCES_VERSION
        mocked_call_args: call = \
            self._mocked_import_resources_page.typed_resources_search_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering typed_resources_search_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        self._mocked_import_resources_page.typed_resources_combobox.currentIndex.return_value = \
            constants.AWS_RESOURCE_LAMBDA_FUNCTION_INDEX
        mock_aws_utils.list_lambda_functions.side_effect = RuntimeError("dummyException")
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        mock_aws_utils.list_lambda_functions.assert_called_once_with(
            TestImportResourcesController._expected_region)
        self._test_import_resources_controller.set_notification_frame_text_sender.emit.assert_called_once()
        self._mocked_proxy_model.load_resource.assert_not_called()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)

    @patch("controller.import_resources_controller.ThreadManager")
    @patch("controller.import_resources_controller.aws_utils")
    def test_page_typed_resources_search_button_no_resource_loaded_when_resource_type_is_unexpected_in_async_process(
            self, mock_aws_utils: MagicMock, mock_thread_manager: MagicMock) -> None:
        self._mocked_import_resources_page.search_version = constants.SEARCH_TYPED_RESOURCES_VERSION
        mocked_call_args: call = \
            self._mocked_import_resources_page.typed_resources_search_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering typed_resources_search_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        self._mocked_import_resources_page.typed_resources_combobox.currentIndex.return_value = -1
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        self._test_import_resources_controller.set_notification_frame_text_sender.emit.assert_called_once()
        self._mocked_proxy_model.load_resource.assert_not_called()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)

    @patch("controller.import_resources_controller.ThreadManager")
    @patch("controller.import_resources_controller.aws_utils")
    def test_page_typed_resources_search_button_expected_resource_loaded_when_found_lambda_in_async_process(
            self, mock_aws_utils: MagicMock, mock_thread_manager: MagicMock) -> None:
        self._mocked_import_resources_page.search_version = constants.SEARCH_TYPED_RESOURCES_VERSION
        mocked_call_args: call = \
            self._mocked_import_resources_page.typed_resources_search_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering typed_resources_search_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        self._mocked_import_resources_page.typed_resources_combobox.currentIndex.return_value = \
            constants.AWS_RESOURCE_LAMBDA_FUNCTION_INDEX
        mock_aws_utils.list_lambda_functions.return_value = [TestImportResourcesController._expected_lambda_name]
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        mock_aws_utils.list_lambda_functions.assert_called_once_with(
            TestImportResourcesController._expected_region)
        self._mocked_proxy_model.load_resource.assert_called_once_with(
            TestImportResourcesController._expected_lambda_resource)
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)

    def test_page_typed_resources_import_button_nothing_happened_when_selected_resources_are_empty(self) -> None:
        self._mocked_proxy_model.deduplicate_selected_import_resources.return_value = []
        mocked_call_args: call = \
            self._mocked_import_resources_page.typed_resources_import_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering typed_resources_import_button connected function
        self._test_import_resources_controller.add_import_resources_sender.emit.assert_not_called()
        self._test_import_resources_controller.set_notification_frame_text_sender.emit.assert_called_once()

    def test_page_typed_resources_import_button_emit_signal_with_expected_resources_and_switch_to_expected_page(self) -> None:
        self._mocked_proxy_model.deduplicate_selected_import_resources.return_value = \
            [TestImportResourcesController._expected_lambda_resource]
        mocked_call_args: call = \
            self._mocked_import_resources_page.typed_resources_import_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering typed_resources_import_button connected function
        self._test_import_resources_controller.add_import_resources_sender.emit.assert_called_once_with(
            [TestImportResourcesController._expected_lambda_resource])
        self._mocked_view_manager.switch_to_view_edit_page.assert_called_once()
        self._mocked_tree_view.reset_view.assert_called_once()
        self._mocked_import_resources_page.set_current_main_view_index.assert_called_once_with(
            ImportResourcesPageConstants.TREE_VIEW_PAGE_INDEX)
        self._mocked_import_resources_page.typed_resources_combobox.setCurrentIndex.assert_called_once_with(-1)
        assert self._mocked_import_resources_page.search_version is None
