"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from typing import (Dict, List)
from unittest import TestCase
from unittest.mock import (call, MagicMock, patch)

from controller.view_edit_controller import ViewEditController
from model import (constants, notification_label_text)
from model.basic_resource_attributes import (BasicResourceAttributes, BasicResourceAttributesBuilder,)
from model.resource_mapping_attributes import (ResourceMappingAttributes, ResourceMappingAttributesStatus)
from view.view_edit_page import ViewEditPageConstants


class TestViewEditController(TestCase):
    """
    ViewEditController unit test cases
    TODO: add test cases once error handling is ready
    """
    _expected_account_id: str = "1234567890"
    _expected_account_id_attribute_name = "AccountId"
    _expected_account_id_template_vale: str = "EMPTY"
    _expected_region: str = "aws-global"
    _expected_config_directory: str = "dummy/directory/"
    _expected_config_file_name: str = "dummy.json"
    _expected_config_file_full_path: str = f"{_expected_config_directory}{_expected_config_file_name}"
    _expected_config_files: List[str] = [_expected_config_file_name]

    _expected_resource: BasicResourceAttributes = BasicResourceAttributesBuilder() \
        .build_type("dummyType") \
        .build_name_id("dummyNameId") \
        .build_account_id("dummyAccountId") \
        .build_region("dummyRegion") \
        .build()

    def setUp(self) -> None:
        configuration_manager_patcher: patch = patch("controller.view_edit_controller.ConfigurationManager")
        self.addCleanup(configuration_manager_patcher.stop)
        self._mock_configuration_manager: MagicMock = configuration_manager_patcher.start()

        view_manager_patcher: patch = patch("controller.view_edit_controller.ViewManager")
        self.addCleanup(view_manager_patcher.stop)
        self._mock_view_manager: MagicMock = view_manager_patcher.start()

        self._mocked_configuration_manager: MagicMock = self._mock_configuration_manager.get_instance.return_value
        self._mocked_configuration_manager.configuration.config_directory = \
            TestViewEditController._expected_config_directory
        self._mocked_configuration_manager.configuration.config_files = \
            TestViewEditController._expected_config_files
        self._mocked_configuration_manager.configuration.account_id = TestViewEditController._expected_account_id
        self._mocked_configuration_manager.configuration.region = TestViewEditController._expected_region

        self._mocked_view_manager: MagicMock = self._mock_view_manager.get_instance.return_value
        self._mocked_view_edit_page: MagicMock = self._mocked_view_manager.get_view_edit_page.return_value
        self._mocked_table_view: MagicMock = self._mocked_view_edit_page.table_view
        self._mocked_proxy_model: MagicMock = self._mocked_table_view.resource_proxy_model

        self._test_view_edit_controller: ViewEditController = ViewEditController()
        self._test_view_edit_controller.set_notification_frame_text_sender = MagicMock()
        self._test_view_edit_controller.set_notification_page_frame_text_sender = MagicMock()
        self._test_view_edit_controller.setup()

    def test_add_import_resources_expected_resource_gets_loaded_into_model(self) -> None:
        self._test_view_edit_controller.add_import_resources_receiver([TestViewEditController._expected_resource])

        self._mocked_proxy_model.add_resource.assert_called_once()
        mocked_call_args: call = self._mocked_proxy_model.add_resource.call_args[0]  # mock call args index is 0
        assert len(mocked_call_args) == 1
        actual_resource: ResourceMappingAttributes = mocked_call_args[0]
        assert actual_resource.type == self._expected_resource.type
        assert actual_resource.name_id == self._expected_resource.name_id
        assert actual_resource.account_id == self._expected_resource.account_id
        assert actual_resource.region == self._expected_resource.region

    def test_setup_with_expected_state_and_behavior_connected(self) -> None:
        self._mocked_view_edit_page.set_current_main_view_index.assert_called_once_with(
            ViewEditPageConstants.NOTIFICATION_PAGE_INDEX)
        self._mocked_view_edit_page.set_config_files.assert_called_once_with(
            TestViewEditController._expected_config_files)
        self._mocked_view_edit_page.set_config_location.assert_called_once_with(
            TestViewEditController._expected_config_directory)

        self._mocked_view_edit_page.config_file_combobox.currentIndexChanged.connect.assert_called_once_with(
            self._test_view_edit_controller._select_config_file)
        self._mocked_view_edit_page.config_location_button.clicked.connect.assert_called_once_with(
            self._test_view_edit_controller._open_file_dialog)
        self._mocked_view_edit_page.add_row_button.clicked.connect.assert_called_once_with(
            self._test_view_edit_controller._add_table_row)
        self._mocked_view_edit_page.delete_row_button.clicked.connect.assert_called_once_with(
            self._test_view_edit_controller._delete_table_row)
        self._mocked_view_edit_page.import_resources_combobox.currentIndexChanged.connect.assert_called_once_with(
            self._test_view_edit_controller._switch_to_import_resources_page)
        self._mocked_view_edit_page.save_changes_button.clicked.connect.assert_called_once_with(
            self._test_view_edit_controller._save_changes)
        self._mocked_view_edit_page.search_filter_input.returnPressed.connect.assert_called_once_with(
            self._test_view_edit_controller._filter_based_on_search_text)
        self._mocked_view_edit_page.cancel_button.clicked.connect.assert_called_once_with(
            self._test_view_edit_controller._cancel)
        self._mocked_view_edit_page.rescan_button.clicked.connect.assert_called_once_with(
            self._test_view_edit_controller._rescan_config_directory)

    def test_page_config_file_combobox_nothing_happen_when_index_is_default_value(self) -> None:
        self._mocked_view_edit_page.config_file_combobox.currentIndex.return_value = -1
        mocked_call_args: call = \
            self._mocked_view_edit_page.config_file_combobox.currentIndexChanged.connect.call_args[0]

        mocked_call_args[0]()  # triggering config_file_combobox connected function
        self._mocked_view_edit_page.config_file_combobox.currentText.assert_not_called()

    @patch("controller.view_edit_controller.file_utils")
    @patch("controller.view_edit_controller.json_utils")
    def test_page_config_file_combobox_post_notification_when_selected_unexpected_config_file_name(
            self, mock_json_utils: MagicMock, mock_file_utils: MagicMock) -> None:
        self._mocked_view_edit_page.config_file_combobox.currentIndex.return_value = 0
        self._mocked_view_edit_page.config_file_combobox.currentText.return_value = "dummyConfigFileName"
        mocked_call_args: call = \
            self._mocked_view_edit_page.config_file_combobox.currentIndexChanged.connect.call_args[0]

        mocked_call_args[0]()  # triggering config_file_combobox connected function
        self._mocked_view_edit_page.config_file_combobox.currentText.assert_called_once()
        self._mocked_table_view.reset_view.assert_not_called()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_not_called()
        self._mocked_proxy_model.load_resource.assert_not_called()
        self._mocked_view_edit_page.set_table_view_page_interactions_enabled.assert_called_with(False)
        self._test_view_edit_controller.set_notification_frame_text_sender.emit.assert_called_once()
        self._mocked_view_edit_page.set_current_main_view_index.assert_called_with(
            ViewEditPageConstants.TABLE_VIEW_PAGE_INDEX)

    @patch("controller.view_edit_controller.file_utils")
    @patch("controller.view_edit_controller.json_utils")
    def test_page_config_file_combobox_no_resource_loaded_when_read_from_json_is_empty(
            self, mock_json_utils: MagicMock, mock_file_utils: MagicMock) -> None:
        self._mocked_view_edit_page.config_file_combobox.currentIndex.return_value = 0
        self._mocked_view_edit_page.config_file_combobox.currentText.return_value = \
            TestViewEditController._expected_config_file_name
        mock_file_utils.join_path.return_value = TestViewEditController._expected_config_file_full_path
        mock_json_utils.read_from_json_file.return_value = {}
        mock_json_utils.convert_json_dict_to_resources.return_value = []
        mock_json_utils.validate_resources_according_to_json_schema.return_value = []
        mocked_call_args: call = \
            self._mocked_view_edit_page.config_file_combobox.currentIndexChanged.connect.call_args[0]

        mocked_call_args[0]()  # triggering config_file_combobox connected function
        self._mocked_view_edit_page.config_file_combobox.currentText.assert_called_once()
        self._mocked_table_view.reset_view.assert_called_once()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_view_edit_page.set_current_main_view_index.assert_called_with(
            ViewEditPageConstants.TABLE_VIEW_PAGE_INDEX)

        mock_file_utils.join_path.assert_called_once_with(
            TestViewEditController._expected_config_directory, TestViewEditController._expected_config_file_name)
        mock_json_utils.read_from_json_file.assert_called_once_with(
            TestViewEditController._expected_config_file_full_path)
        mock_json_utils.validate_json_dict_according_to_json_schema.assert_called_once_with({})
        mock_json_utils.convert_json_dict_to_resources.assert_called_once_with({})
        self._mocked_proxy_model.load_resource.assert_not_called()

    @patch("controller.view_edit_controller.file_utils")
    @patch("controller.view_edit_controller.json_utils")
    def test_page_config_file_combobox_no_resource_loaded_when_validate_json_dict_raise_exception(
            self, mock_json_utils: MagicMock, mock_file_utils: MagicMock) -> None:
        self._mocked_view_edit_page.config_file_combobox.currentIndex.return_value = 0
        self._mocked_view_edit_page.config_file_combobox.currentText.return_value = \
            TestViewEditController._expected_config_file_name
        mock_file_utils.join_path.return_value = TestViewEditController._expected_config_file_full_path
        mock_json_utils.read_from_json_file.return_value = {}
        mock_json_utils.validate_json_dict_according_to_json_schema.side_effect = KeyError("dummyException")
        mock_json_utils.validate_resources_according_to_json_schema.return_value = []
        mocked_call_args: call = \
            self._mocked_view_edit_page.config_file_combobox.currentIndexChanged.connect.call_args[0]

        mocked_call_args[0]()  # triggering config_file_combobox connected function
        self._mocked_view_edit_page.config_file_combobox.currentText.assert_called_once()
        self._mocked_table_view.reset_view.assert_called_once()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_view_edit_page.set_current_main_view_index.assert_called_with(
            ViewEditPageConstants.TABLE_VIEW_PAGE_INDEX)

        mock_file_utils.join_path.assert_called_once_with(
            TestViewEditController._expected_config_directory, TestViewEditController._expected_config_file_name)
        mock_json_utils.read_from_json_file.assert_called_once_with(
            TestViewEditController._expected_config_file_full_path)
        mock_json_utils.validate_json_dict_according_to_json_schema.assert_called_once_with({})
        mock_json_utils.convert_json_dict_to_resources.assert_not_called()
        self._mocked_proxy_model.load_resource.assert_not_called()
        self._test_view_edit_controller.set_notification_frame_text_sender.emit.assert_called_once()
        self._mocked_view_edit_page.set_table_view_page_interactions_enabled.assert_called_with(False)

    @patch("controller.view_edit_controller.file_utils")
    @patch("controller.view_edit_controller.json_utils")
    def test_page_config_file_combobox_load_expected_resource_from_json_file(
            self, mock_json_utils: MagicMock, mock_file_utils: MagicMock) -> None:
        self._mocked_view_edit_page.config_file_combobox.currentIndex.return_value = 0
        self._mocked_view_edit_page.config_file_combobox.currentText.return_value = \
            TestViewEditController._expected_config_file_name
        mock_file_utils.join_path.return_value = TestViewEditController._expected_config_file_full_path
        mock_json_utils.read_from_json_file.return_value = {}
        mock_json_utils.convert_json_dict_to_resources.return_value = [TestViewEditController._expected_resource]
        mock_json_utils.validate_resources_according_to_json_schema.return_value = []
        mocked_call_args: call = \
            self._mocked_view_edit_page.config_file_combobox.currentIndexChanged.connect.call_args[0]

        mocked_call_args[0]()  # triggering config_file_combobox connected function
        self._mocked_view_edit_page.config_file_combobox.currentText.assert_called_once()
        self._mocked_table_view.reset_view.assert_called_once()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_called_once()
        self._mocked_view_edit_page.set_current_main_view_index.assert_called_with(
            ViewEditPageConstants.TABLE_VIEW_PAGE_INDEX)

        mock_file_utils.join_path.assert_called_once_with(
            TestViewEditController._expected_config_directory, TestViewEditController._expected_config_file_name)
        mock_json_utils.read_from_json_file.assert_called_once_with(
            TestViewEditController._expected_config_file_full_path)
        mock_json_utils.validate_json_dict_according_to_json_schema.assert_called_once_with({})
        mock_json_utils.convert_json_dict_to_resources.assert_called_once_with({})
        self._mocked_proxy_model.load_resource.assert_called_once_with(TestViewEditController._expected_resource)

    @patch("controller.view_edit_controller.file_utils")
    @patch("controller.view_edit_controller.json_utils")
    def test_page_config_file_combobox_load_expected_resource_with_notification_post(
            self, mock_json_utils: MagicMock, mock_file_utils: MagicMock) -> None:
        self._mocked_view_edit_page.config_file_combobox.currentIndex.return_value = 0
        self._mocked_view_edit_page.config_file_combobox.currentText.return_value = \
            TestViewEditController._expected_config_file_name
        mock_file_utils.join_path.return_value = TestViewEditController._expected_config_file_full_path
        mock_json_utils.read_from_json_file.return_value = {}
        mock_json_utils.convert_json_dict_to_resources.return_value = [TestViewEditController._expected_resource]
        mock_json_utils.validate_resources_according_to_json_schema.return_value = {0: []}
        mocked_call_args: call = \
            self._mocked_view_edit_page.config_file_combobox.currentIndexChanged.connect.call_args[0]

        mocked_call_args[0]()  # triggering config_file_combobox connected function
        self._mocked_view_edit_page.config_file_combobox.currentText.assert_called_once()
        self._mocked_table_view.reset_view.assert_called_once()
        self._mocked_proxy_model.override_resource_status.assert_called_once()
        self._test_view_edit_controller.set_notification_frame_text_sender.emit.assert_called_once()
        self._mocked_proxy_model.emit_source_model_layout_changed.assert_has_calls([call(), call()])
        self._mocked_view_edit_page.set_current_main_view_index.assert_called_with(
            ViewEditPageConstants.TABLE_VIEW_PAGE_INDEX)

        mock_file_utils.join_path.assert_called_once_with(
            TestViewEditController._expected_config_directory, TestViewEditController._expected_config_file_name)
        mock_json_utils.read_from_json_file.assert_called_once_with(
            TestViewEditController._expected_config_file_full_path)
        mock_json_utils.validate_json_dict_according_to_json_schema.assert_called_once_with({})
        mock_json_utils.convert_json_dict_to_resources.assert_called_once_with({})
        self._mocked_proxy_model.load_resource.assert_called_once_with(TestViewEditController._expected_resource)

    @patch("controller.view_edit_controller.QFileDialog")
    def test_page_config_location_button_nothing_happen_when_config_location_is_empty(
            self, mock_file_dialog: MagicMock) -> None:
        mock_file_dialog.getExistingDirectory.return_value = ""
        mocked_call_args: call = \
            self._mocked_view_edit_page.config_location_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering config_location_button connected function
        mock_file_dialog.getExistingDirectory.assert_called_once()
        assert self._mocked_configuration_manager.configuration.config_directory == \
               TestViewEditController._expected_config_directory

    @patch("controller.view_edit_controller.ThreadManager")
    @patch("controller.view_edit_controller.QFileDialog")
    def test_page_config_location_button_set_expected_new_config_directory_in_sync_process(
            self, mock_file_dialog: MagicMock, mock_thread_manager: MagicMock) -> None:
        expected_new_config_directory: str = "new/dummy/directory"
        mock_file_dialog.getExistingDirectory.return_value = expected_new_config_directory
        mocked_call_args: call = \
            self._mocked_view_edit_page.config_location_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering config_location_button connected function
        mock_file_dialog.getExistingDirectory.assert_called_once()
        self._test_view_edit_controller.set_notification_page_frame_text_sender.emit.assert_called_with(
            notification_label_text.NOTIFICATION_LOADING_MESSAGE)
        self._mocked_view_edit_page.set_current_main_view_index.assert_called_with(
            ViewEditPageConstants.NOTIFICATION_PAGE_INDEX)
        self._mocked_table_view.reset_view.assert_called_once()
        self._mocked_view_edit_page.set_config_location.assert_called_with(expected_new_config_directory)
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        assert self._mocked_configuration_manager.configuration.config_directory == expected_new_config_directory

    @patch("controller.view_edit_controller.ThreadManager")
    @patch("controller.view_edit_controller.QFileDialog")
    @patch("controller.view_edit_controller.file_utils")
    def test_page_config_location_button_set_empty_config_files_when_search_files_return_empty_in_async_process(
            self, mock_file_utils: MagicMock, mock_file_dialog: MagicMock, mock_thread_manager: MagicMock) -> None:
        expected_new_config_directory: str = "new/dummy/directory"
        mock_file_dialog.getExistingDirectory.return_value = expected_new_config_directory
        mocked_call_args: call = \
            self._mocked_view_edit_page.config_location_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering config_location_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        mock_file_utils.find_files_with_suffix_under_directory.return_value = []
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        mock_file_utils.find_files_with_suffix_under_directory.assert_called_once_with(
            expected_new_config_directory, constants.RESOURCE_MAPPING_CONFIG_FILE_NAME_SUFFIX)
        assert self._mocked_configuration_manager.configuration.config_files == []
        self._mocked_view_edit_page.set_config_files.assert_called_with([])
        self._mocked_view_edit_page.set_config_location.assert_called_with(expected_new_config_directory)
        self._test_view_edit_controller.set_notification_page_frame_text_sender.emit.assert_called_once_with(
            notification_label_text.NOTIFICATION_LOADING_MESSAGE)

    @patch("controller.view_edit_controller.ThreadManager")
    @patch("controller.view_edit_controller.QFileDialog")
    @patch("controller.view_edit_controller.file_utils")
    def test_page_config_location_button_set_empty_config_files_when_search_files_raise_exception_in_async_process(
            self, mock_file_utils: MagicMock, mock_file_dialog: MagicMock, mock_thread_manager: MagicMock) -> None:
        expected_new_config_directory: str = "new/dummy/directory"
        mock_file_dialog.getExistingDirectory.return_value = expected_new_config_directory
        mocked_call_args: call = \
            self._mocked_view_edit_page.config_location_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering config_location_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        mock_file_utils.find_files_with_suffix_under_directory.side_effect = FileNotFoundError("dummyException")
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        mock_file_utils.find_files_with_suffix_under_directory.assert_called_once_with(
            expected_new_config_directory, constants.RESOURCE_MAPPING_CONFIG_FILE_NAME_SUFFIX)
        assert self._mocked_configuration_manager.configuration.config_files == []
        self._mocked_view_edit_page.set_config_files.assert_called_with([])
        self._test_view_edit_controller.set_notification_page_frame_text_sender.emit.assert_called_once_with(
            notification_label_text.NOTIFICATION_LOADING_MESSAGE)

    @patch("controller.view_edit_controller.ThreadManager")
    @patch("controller.view_edit_controller.QFileDialog")
    @patch("controller.view_edit_controller.file_utils")
    def test_page_config_location_button_set_new_config_files_when_search_and_find_files_in_async_process(
            self, mock_file_utils: MagicMock, mock_file_dialog: MagicMock, mock_thread_manager: MagicMock) -> None:
        expected_new_config_directory: str = "new/dummy/directory"
        mock_file_dialog.getExistingDirectory.return_value = expected_new_config_directory
        mocked_call_args: call = \
            self._mocked_view_edit_page.config_location_button.clicked.connect.call_args[0]
        mocked_call_args[0]()  # triggering config_location_button connected function
        mock_thread_manager.get_instance.return_value.start.assert_called_once()
        expected_new_config_files: List[str] = ["new_dummy.json"]
        mock_file_utils.find_files_with_suffix_under_directory.return_value = expected_new_config_files
        mocked_async_call_args: call = mock_thread_manager.get_instance.return_value.start.call_args[0]
        mocked_async_call_args[0].run()  # triggering async function

        mock_file_utils.find_files_with_suffix_under_directory.assert_called_once_with(
            expected_new_config_directory, constants.RESOURCE_MAPPING_CONFIG_FILE_NAME_SUFFIX)
        assert self._mocked_configuration_manager.configuration.config_files == expected_new_config_files
        self._mocked_view_edit_page.set_config_files.assert_called_with(expected_new_config_files)
        self._test_view_edit_controller.set_notification_page_frame_text_sender.emit.assert_called_once_with(
            notification_label_text.NOTIFICATION_LOADING_MESSAGE)

    def test_page_add_row_button_expected_resource_gets_loaded_into_model(self) -> None:
        mocked_call_args: call = self._mocked_view_edit_page.add_row_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering add_row_button connected function
        self._mocked_proxy_model.add_resource.assert_called_once()
        mocked_call_args: call = self._mocked_proxy_model.add_resource.call_args[0]  # mock call args index is 0
        assert len(mocked_call_args) == 1
        actual_resource: ResourceMappingAttributes = mocked_call_args[0]
        assert actual_resource.account_id == TestViewEditController._expected_account_id
        assert actual_resource.region == TestViewEditController._expected_region

    def test_page_delete_row_button_expected_selected_indices_invoked_with_remove_resource_model_call(self) -> None:
        self._mocked_table_view.selectedIndexes.return_value = []
        mocked_call_args: call = self._mocked_view_edit_page.delete_row_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering delete_row_button connected function
        self._mocked_proxy_model.remove_resources.assert_called_once_with([])

    def test_page_import_resources_combobox_nothing_happen_when_index_is_default_value(self) -> None:
        self._mocked_view_edit_page.import_resources_combobox.currentIndex.return_value = -1
        mocked_call_args: call = \
            self._mocked_view_edit_page.import_resources_combobox.currentIndexChanged.connect.call_args[0]

        mocked_call_args[0]()  # triggering import_resources_combobox connected function
        self._mocked_view_edit_page.import_resources_combobox.currentText.assert_not_called()

    def test_page_import_resources_combobox_switch_page_with_expected_search_version(self) -> None:
        self._mocked_view_edit_page.import_resources_combobox.currentIndex.return_value = 0
        expected_search_version: str = "dummyVersion"
        self._mocked_view_edit_page.import_resources_combobox.currentText.return_value = expected_search_version
        mocked_call_args: call = \
            self._mocked_view_edit_page.import_resources_combobox.currentIndexChanged.connect.call_args[0]

        mocked_call_args[0]()  # triggering import_resources_combobox connected function
        self._mocked_view_manager.switch_to_import_resources_page.assert_called_once_with(expected_search_version)
        self._mocked_view_edit_page.import_resources_combobox.setCurrentIndex.assert_called_once_with(-1)

    @patch("controller.view_edit_controller.json_utils")
    def test_page_save_changes_button_nothing_happen_when_validation_failed(self, mock_json_utils: MagicMock) -> None:
        mock_json_utils.validate_resources_according_to_json_schema.return_value = {0: []}
        mocked_call_args: call = self._mocked_view_edit_page.save_changes_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering save_changes_button connected function
        self._mocked_proxy_model.override_resource_status.assert_called_once()
        self._test_view_edit_controller.set_notification_frame_text_sender.emit.assert_called_once()
        self._mocked_proxy_model.override_all_resources_status.assert_not_called()

    @patch("controller.view_edit_controller.json_utils")
    def test_page_save_changes_button_state_column_is_marked_as_success_when_converted_json_dict_is_same_as_original_one(
            self, mock_json_utils: MagicMock) -> None:
        self._mocked_view_edit_page.config_file_combobox.currentText.return_value = \
            TestViewEditController._expected_config_file_name
        dummy_dict: Dict[str, any] = {"dummyKey": "dummyValue"}
        self._test_view_edit_controller._config_file_json_source = dummy_dict
        mock_json_utils.validate_resources_according_to_json_schema.return_value = []
        mock_json_utils.convert_resources_to_json_dict.return_value = dummy_dict
        mocked_call_args: call = self._mocked_view_edit_page.save_changes_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering save_changes_button connected function
        mock_json_utils.convert_resources_to_json_dict.assert_called_once()
        mock_json_utils.write_into_json_file.assert_not_called()
        self._mocked_proxy_model.override_all_resources_status.assert_called_once()

    @patch("controller.view_edit_controller.file_utils")
    @patch("controller.view_edit_controller.json_utils")
    def test_page_save_changes_button_json_file_saved_and_model_state_is_updated(
            self, mock_json_utils: MagicMock, mock_file_utils: MagicMock) -> None:
        self._mocked_view_edit_page.config_file_combobox.currentText.return_value = \
            TestViewEditController._expected_config_file_name
        expected_json_dict: Dict[str, any] = {
            "dummyKey": "dummyValue"
        }
        mock_json_utils.validate_resources_according_to_json_schema.return_value = []
        mock_json_utils.convert_resources_to_json_dict.return_value = expected_json_dict
        mock_file_utils.join_path.return_value = TestViewEditController._expected_config_file_full_path
        mocked_call_args: call = self._mocked_view_edit_page.save_changes_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering save_changes_button connected function
        mock_json_utils.convert_resources_to_json_dict.assert_called_once()
        mock_json_utils.write_into_json_file.assert_called_once_with(
            TestViewEditController._expected_config_file_full_path, expected_json_dict)
        self._mocked_proxy_model.override_all_resources_status.assert_called_once_with(
            ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE,
                                            [ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE]))

    @patch("controller.view_edit_controller.file_utils")
    @patch("controller.view_edit_controller.json_utils")
    def test_page_save_changes_button_json_file_saved_and_template_account_id_unchanged(
            self, mock_json_utils: MagicMock, mock_file_utils: MagicMock) -> None:
        self._mocked_view_edit_page.config_file_combobox.currentText.return_value = \
            TestViewEditController._expected_config_file_name
        expected_json_dict: Dict[str, any] = {
            self._expected_account_id_attribute_name: self._expected_account_id_template_vale
        }
        mock_json_utils.RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME = self._expected_account_id_attribute_name
        mock_json_utils.RESOURCE_MAPPING_ACCOUNTID_TEMPLATE_VALUE = self._expected_account_id_template_vale
        mock_json_utils.validate_resources_according_to_json_schema.return_value = []
        mock_json_utils.convert_resources_to_json_dict.return_value = expected_json_dict
        mock_file_utils.join_path.return_value = TestViewEditController._expected_config_file_full_path
        mocked_call_args: call = self._mocked_view_edit_page.save_changes_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering save_changes_button connected function
        assert expected_json_dict[mock_json_utils.RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME] == self._expected_account_id_template_vale
        mock_json_utils.convert_resources_to_json_dict.assert_called_once()
        mock_json_utils.write_into_json_file.assert_called_once_with(
            TestViewEditController._expected_config_file_full_path, expected_json_dict)
        self._mocked_proxy_model.override_all_resources_status.assert_called_once_with(
            ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE,
                                            [ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE]))

    @patch("controller.view_edit_controller.file_utils")
    @patch("controller.view_edit_controller.json_utils")
    def test_page_save_changes_button_json_file_saved_and_empty_account_id_unchanged(
            self, mock_json_utils: MagicMock, mock_file_utils: MagicMock) -> None:
        self._mocked_view_edit_page.config_file_combobox.currentText.return_value = \
            TestViewEditController._expected_config_file_name
        expected_json_dict: Dict[str, any] = {
            self._expected_account_id_attribute_name: ''
        }
        mock_json_utils.RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME = self._expected_account_id_attribute_name
        mock_json_utils.validate_resources_according_to_json_schema.return_value = []
        mock_json_utils.convert_resources_to_json_dict.return_value = expected_json_dict
        mock_file_utils.join_path.return_value = TestViewEditController._expected_config_file_full_path
        mocked_call_args: call = self._mocked_view_edit_page.save_changes_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering save_changes_button connected function
        assert expected_json_dict[mock_json_utils.RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME] == ''
        mock_json_utils.convert_resources_to_json_dict.assert_called_once()
        mock_json_utils.write_into_json_file.assert_called_once_with(
            TestViewEditController._expected_config_file_full_path, expected_json_dict)
        self._mocked_proxy_model.override_all_resources_status.assert_called_once_with(
            ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE,
                                            [ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE]))

    @patch("controller.view_edit_controller.file_utils")
    @patch("controller.view_edit_controller.json_utils")
    def test_page_save_changes_button_post_notification_when_write_into_json_file_raise_exception(
            self, mock_json_utils: MagicMock, mock_file_utils: MagicMock) -> None:
        self._mocked_view_edit_page.config_file_combobox.currentText.return_value = \
            TestViewEditController._expected_config_file_name
        expected_json_dict: Dict[str, any] = {"dummyKey": "dummyValue"}
        mock_json_utils.validate_resources_according_to_json_schema.return_value = []
        mock_json_utils.convert_resources_to_json_dict.return_value = expected_json_dict
        mock_file_utils.join_path.return_value = TestViewEditController._expected_config_file_full_path
        mock_json_utils.write_into_json_file.side_effect = IOError("dummyException")
        mocked_call_args: call = self._mocked_view_edit_page.save_changes_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering save_changes_button connected function
        mock_json_utils.convert_resources_to_json_dict.assert_called_once()
        mock_json_utils.write_into_json_file.assert_called_once_with(
            TestViewEditController._expected_config_file_full_path, expected_json_dict)
        self._test_view_edit_controller.set_notification_frame_text_sender.emit.assert_called_once()
        self._mocked_proxy_model.override_all_resources_status.assert_not_called()

    def test_page_search_filter_input_invoke_proxy_model_with_expected_filter_text(self) -> None:
        expected_filter_text: str = "dummySearchFilter"
        self._mocked_view_edit_page.search_filter_input.text.return_value = expected_filter_text
        mocked_call_args: call = self._mocked_view_edit_page.search_filter_input.returnPressed.connect.call_args[0]

        mocked_call_args[0]()  # triggering search_filter_input connected function
        self._mocked_table_view.clear_selection()
        assert self._mocked_proxy_model.filter_text == expected_filter_text

    @patch("controller.view_edit_controller.QCoreApplication")
    def test_page_cancel_button_invoke_application_close(self, mock_application: MagicMock) -> None:
        mocked_call_args: call = self._mocked_view_edit_page.cancel_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering cancel_button connected function
        mock_application.instance.return_value.quit.assert_called_once()

    @patch("controller.view_edit_controller.file_utils")
    def test_page_rescan_button_post_notification_when_find_files_throw_exception(
            self, mock_file_utils: MagicMock) -> None:
        mock_file_utils.find_files_with_suffix_under_directory.side_effect = FileNotFoundError("dummy error")
        mocked_call_args: call = \
            self._mocked_view_edit_page.rescan_button.clicked.connect.call_args[0]

        mocked_call_args[0]()  # triggering rescan_button connected function
        mock_file_utils.find_files_with_suffix_under_directory.assert_called_once()
        self._test_view_edit_controller.set_notification_frame_text_sender.emit.assert_called_once()
