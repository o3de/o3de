"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import copy
from typing import (Dict, List)
from unittest import TestCase
from unittest.mock import (MagicMock, mock_open, patch)

from utils import file_utils
from utils import json_utils
from model import constants
from model.resource_mapping_attributes import (ResourceMappingAttributes, ResourceMappingAttributesBuilder,
                                               ResourceMappingAttributesStatus)


class TestJsonUtils(TestCase):
    """
    json utils unit test cases
    TODO: add test cases once error handling is ready
    """
    _expected_file_name: str = "dummy.json"
    _expected_key: str = "TestBucketKey"
    _expected_bucket_type: str = constants.AWS_RESOURCE_TYPES[constants.AWS_RESOURCE_S3_BUCKET_INDEX]
    _expected_bucket_name: str = "TestBucketName"
    _expected_account_id: str = "123456789012"
    _expected_invalid_account_id: str = "12345"
    _expected_region: str = "us-west-2"
    _expected_invalid_region: str = "dummy-region"

    _expected_bucket_resource_mapping: ResourceMappingAttributes = ResourceMappingAttributesBuilder() \
        .build_key_name(_expected_key) \
        .build_type(_expected_bucket_type) \
        .build_name_id(_expected_bucket_name) \
        .build_account_id(_expected_account_id) \
        .build_region(_expected_region) \
        .build()

    _expected_json_dict: Dict[str, any] = {
        json_utils._RESOURCE_MAPPING_JSON_KEY_NAME: {_expected_key: {
            json_utils._RESOURCE_MAPPING_TYPE_JSON_KEY_NAME: _expected_bucket_type,
            json_utils._RESOURCE_MAPPING_NAMEID_JSON_KEY_NAME: _expected_bucket_name
        }},
        json_utils.RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME: _expected_account_id,
        json_utils._RESOURCE_MAPPING_REGION_JSON_KEY_NAME: _expected_region,
        json_utils._RESOURCE_MAPPING_VERSION_JSON_KEY_NAME: json_utils._RESOURCE_MAPPING_JSON_FORMAT_VERSION
    }

    def setUp(self) -> None:
        schema_path: str = file_utils.join_path(file_utils.get_parent_directory_path(__file__, 4),
                                                'resource_mapping_schema.json')
        json_utils.load_resource_mapping_json_schema(schema_path)

        self._mock_open = mock_open()
        open_patcher: patch = patch("utils.json_utils.open", self._mock_open)
        self.addCleanup(open_patcher.stop)
        open_patcher.start()

        json_dump_patcher: patch = patch("json.dump")
        self.addCleanup(json_dump_patcher.stop)
        self._mock_json_dump: MagicMock = json_dump_patcher.start()

        json_load_patcher: patch = patch("json.load")
        self.addCleanup(json_load_patcher.stop)
        self._mock_json_load: MagicMock = json_load_patcher.start()

    def test_convert_resources_to_json_dict_return_expected_json_dict(self) -> None:
        old_json_dict: Dict[str, any] = {
            json_utils.RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME: TestJsonUtils._expected_account_id,
            json_utils._RESOURCE_MAPPING_REGION_JSON_KEY_NAME: TestJsonUtils._expected_region
        }
        actual_json_dict: Dict[str, any] = \
            json_utils.convert_resources_to_json_dict([TestJsonUtils._expected_bucket_resource_mapping], old_json_dict)
        assert actual_json_dict == TestJsonUtils._expected_json_dict

    def test_convert_json_dict_to_resources_return_expected_resources(self) -> None:
        actual_resources: List[ResourceMappingAttributes] = \
            json_utils.convert_json_dict_to_resources(TestJsonUtils._expected_json_dict)
        assert actual_resources == [TestJsonUtils._expected_bucket_resource_mapping]

    def test_read_from_json_file_return_expected_json_dict(self) -> None:
        mocked_open: MagicMock = MagicMock()
        self._mock_open.return_value.__enter__.return_value = mocked_open
        expected_json_dict: Dict[str, any] = {}
        self._mock_json_load.return_value = expected_json_dict

        actual_json_dict: Dict[str, any] = json_utils.read_from_json_file(TestJsonUtils._expected_file_name)
        self._mock_open.assert_called_once_with(TestJsonUtils._expected_file_name, "r")
        self._mock_json_load.assert_called_once_with(mocked_open,
                                                     object_pairs_hook=json_utils._validate_json_dict_unique_keys)
        assert actual_json_dict == expected_json_dict

    def test_validate_json_dict_according_to_json_schema_raise_error_when_json_dict_has_no_version(self) -> None:
        invalid_json_dict: Dict[str, any] = copy.deepcopy(TestJsonUtils._expected_json_dict)
        invalid_json_dict.pop(json_utils._RESOURCE_MAPPING_VERSION_JSON_KEY_NAME)
        self.assertRaises(KeyError, json_utils.validate_json_dict_according_to_json_schema, invalid_json_dict)

    def test_validate_json_dict_according_to_json_schema_raise_error_when_json_dict_has_no_resource_mappings(self) -> None:
        invalid_json_dict: Dict[str, any] = copy.deepcopy(TestJsonUtils._expected_json_dict)
        invalid_json_dict.pop(json_utils._RESOURCE_MAPPING_JSON_KEY_NAME)
        self.assertRaises(KeyError, json_utils.validate_json_dict_according_to_json_schema, invalid_json_dict)

    def test_validate_json_dict_according_to_json_schema_raise_error_when_json_dict_has_no_accountid(self) -> None:
        invalid_json_dict: Dict[str, any] = copy.deepcopy(TestJsonUtils._expected_json_dict)
        invalid_json_dict.pop(json_utils.RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME)
        self.assertRaises(KeyError, json_utils.validate_json_dict_according_to_json_schema, invalid_json_dict)

    def test_validate_json_dict_according_to_json_schema_raise_error_when_json_dict_has_empty_accountid(self) -> None:
        valid_json_dict: Dict[str, any] = copy.deepcopy(TestJsonUtils._expected_json_dict)
        valid_json_dict[json_utils.RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME] = ''
        json_utils.validate_json_dict_according_to_json_schema(valid_json_dict)

    def test_validate_json_dict_according_to_json_schema_pass_when_json_dict_has_template_accountid(self) -> None:
        valid_json_dict: Dict[str, any] = copy.deepcopy(TestJsonUtils._expected_json_dict)
        valid_json_dict[json_utils.RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME] = \
            json_utils.RESOURCE_MAPPING_ACCOUNTID_TEMPLATE_VALUE
        json_utils.validate_json_dict_according_to_json_schema(valid_json_dict)

    def test_validate_json_dict_according_to_json_schema_raise_error_when_json_dict_has_invalid_accountid(self) -> None:
        invalid_json_dict: Dict[str, any] = copy.deepcopy(TestJsonUtils._expected_json_dict)
        invalid_json_dict[json_utils.RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME] = \
            TestJsonUtils._expected_invalid_account_id
        self.assertRaises(ValueError, json_utils.validate_json_dict_according_to_json_schema, invalid_json_dict)

    def test_validate_json_dict_according_to_json_schema_raise_error_when_json_dict_has_no_region(self) -> None:
        invalid_json_dict: Dict[str, any] = copy.deepcopy(TestJsonUtils._expected_json_dict)
        invalid_json_dict.pop(json_utils._RESOURCE_MAPPING_REGION_JSON_KEY_NAME)
        self.assertRaises(KeyError, json_utils.validate_json_dict_according_to_json_schema, invalid_json_dict)

    def test_validate_json_dict_according_to_json_schema_raise_error_when_json_dict_has_invalid_region(self) -> None:
        invalid_json_dict: Dict[str, any] = copy.deepcopy(TestJsonUtils._expected_json_dict)
        invalid_json_dict[json_utils._RESOURCE_MAPPING_REGION_JSON_KEY_NAME] = \
            TestJsonUtils._expected_invalid_region
        self.assertRaises(ValueError, json_utils.validate_json_dict_according_to_json_schema, invalid_json_dict)

    def test_validate_json_dict_according_to_json_schema_raise_error_when_json_dict_resource_has_no_type(self) -> None:
        invalid_json_dict: Dict[str, any] = copy.deepcopy(TestJsonUtils._expected_json_dict)
        invalid_json_dict[json_utils._RESOURCE_MAPPING_JSON_KEY_NAME][TestJsonUtils._expected_key].pop(
            json_utils._RESOURCE_MAPPING_TYPE_JSON_KEY_NAME)
        self.assertRaises(KeyError, json_utils.validate_json_dict_according_to_json_schema, invalid_json_dict)

    def test_validate_json_dict_according_to_json_schema_raise_error_when_json_dict_resource_has_no_nameid(self) -> None:
        invalid_json_dict: Dict[str, any] = copy.deepcopy(TestJsonUtils._expected_json_dict)
        invalid_json_dict[json_utils._RESOURCE_MAPPING_JSON_KEY_NAME][TestJsonUtils._expected_key].pop(
            json_utils._RESOURCE_MAPPING_NAMEID_JSON_KEY_NAME)
        self.assertRaises(KeyError, json_utils.validate_json_dict_according_to_json_schema, invalid_json_dict)

    def test_validate_resources_according_to_json_schema_return_expected_rows_when_resource_is_invalid(self) -> None:
        invalid_bucket_resource_mapping: ResourceMappingAttributes = TestJsonUtils._expected_bucket_resource_mapping
        invalid_bucket_resource_mapping.key_name = ""
        actual_invalid_rows: Dict[int, List[str]] = \
            json_utils.validate_resources_according_to_json_schema([invalid_bucket_resource_mapping])
        assert list(actual_invalid_rows.keys()) == [0]

    def test_validate_resources_according_to_json_schema_return_empty_when_resource_is_valid(self) -> None:
        actual_invalid_rows: Dict[int, List[str]] = \
            json_utils.validate_resources_according_to_json_schema([TestJsonUtils._expected_bucket_resource_mapping])
        assert list(actual_invalid_rows.keys()) == []

    def test_validate_resources_according_to_json_schema_return_expected_rows_when_non_succeed_resource_has_same_key_as_succeed_one(self) -> None:
        succeed_bucket_resource_mapping: ResourceMappingAttributes = \
            copy.deepcopy(TestJsonUtils._expected_bucket_resource_mapping)
        succeed_bucket_resource_mapping.status = \
            ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE)
        non_succeed_bucket_resource_mapping: ResourceMappingAttributes = \
            copy.deepcopy(TestJsonUtils._expected_bucket_resource_mapping)
        non_succeed_bucket_resource_mapping.status = \
            ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.MODIFIED_STATUS_VALUE)
        actual_invalid_rows: Dict[int, List[str]] = \
            json_utils.validate_resources_according_to_json_schema([
                succeed_bucket_resource_mapping, non_succeed_bucket_resource_mapping])
        assert list(actual_invalid_rows.keys()) == [1]

    def test_validate_resources_according_to_json_schema_return_expected_rows_when_non_succeed_resources_have_duplicated_key(self) -> None:
        actual_invalid_rows: Dict[int, List[str]] = \
            json_utils.validate_resources_according_to_json_schema([
                TestJsonUtils._expected_bucket_resource_mapping, TestJsonUtils._expected_bucket_resource_mapping])
        assert list(actual_invalid_rows.keys()) == [0, 1]

    def test_validate_resources_according_to_json_schema_return_expected_rows_when_resource_has_invalid_accountid(self) -> None:
        invalid_bucket_resource_mapping: ResourceMappingAttributes = \
            copy.deepcopy(TestJsonUtils._expected_bucket_resource_mapping)
        invalid_bucket_resource_mapping.account_id = TestJsonUtils._expected_invalid_account_id
        actual_invalid_rows: Dict[int, List[str]] = \
            json_utils.validate_resources_according_to_json_schema([invalid_bucket_resource_mapping])
        assert list(actual_invalid_rows.keys()) == [0]

    def test_validate_resources_according_to_json_schema_return_expected_rows_when_resource_has_invalid_region(self) -> None:
        invalid_bucket_resource_mapping: ResourceMappingAttributes = \
            copy.deepcopy(TestJsonUtils._expected_bucket_resource_mapping)
        invalid_bucket_resource_mapping.region = TestJsonUtils._expected_invalid_region
        actual_invalid_rows: Dict[int, List[str]] = \
            json_utils.validate_resources_according_to_json_schema([invalid_bucket_resource_mapping])
        assert list(actual_invalid_rows.keys()) == [0]

    def test_write_into_json_file_succeed(self) -> None:
        mocked_open: MagicMock = MagicMock()
        self._mock_open.return_value.__enter__.return_value = mocked_open

        json_utils.write_into_json_file(TestJsonUtils._expected_file_name, TestJsonUtils._expected_json_dict)
        self._mock_open.assert_called_once_with(TestJsonUtils._expected_file_name, "w")
        self._mock_json_dump.assert_called_once_with(
            TestJsonUtils._expected_json_dict, mocked_open, indent=4, sort_keys=True)
