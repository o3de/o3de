"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import json
import logging
import re
from typing import (Dict, List, Set)

from model import error_messages
from model.resource_mapping_attributes import (ResourceMappingAttributes, ResourceMappingAttributesBuilder,
                                               ResourceMappingAttributesStatus)

"""
Json Utils provide related functions to read/write/serialize/deserialize json format
"""
logger = logging.getLogger(__name__)

# resource mapping json content constants
_RESOURCE_MAPPING_JSON_KEY_NAME: str = "AWSResourceMappings"
_RESOURCE_MAPPING_TYPE_JSON_KEY_NAME: str = "Type"
_RESOURCE_MAPPING_NAMEID_JSON_KEY_NAME: str = "Name/ID"
_RESOURCE_MAPPING_REGION_JSON_KEY_NAME: str = "Region"
_RESOURCE_MAPPING_VERSION_JSON_KEY_NAME: str = "Version"
_RESOURCE_MAPPING_JSON_FORMAT_VERSION: str = "1.1.0"
RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME: str = "AccountId"
RESOURCE_MAPPING_ACCOUNTID_TEMPLATE_VALUE: str = "EMPTY"

# resource mapping json schema constants
_RESOURCE_MAPPING_SCHEMA_ACCOUNTID_KEY_NAME: str = "AccountIdString"
_RESOURCE_MAPPING_SCHEMA_REGION_KEY_NAME: str = "RegionString"
_RESOURCE_MAPPING_SCHEMA_REQUIRED_PROPERTIES_KEY_NAME: str = "required"
_RESOURCE_MAPPING_SCHEMA_PROPERTIES_KEY_NAME: str = "properties"
_RESOURCE_MAPPING_SCHEMA_PATTERN_PROPERTIES_KEY_NAME: str = "patternProperties"
_RESOURCE_MAPPING_SCHEMA_PROPERTY_PATTERN_KEY_NAME: str = "pattern"
_RESOURCE_MAPPING_SCHEMA: Dict[str, any] = {}
_RESOURCE_MAPPING_SCHEMA_REQUIRED_ROOT_PROPERTIES: List[str] = []
_RESOURCE_MAPPING_SCHEMA_REQUIRED_RESOURCE_PROPERTIES: List[str] = []
_RESOURCE_MAPPING_SCHEMA_ACCOUNTID_PATTERN: str = ""
_RESOURCE_MAPPING_SCHEMA_REGION_PATTERN: str = ""
_RESOURCE_MAPPING_SCHEMA_VERSION_PATTERN: str = ""

def _add_validation_error_message(errors: Dict[int, List[str]], row: int, error_message: str) -> None:
    if row in errors.keys():
        errors[row].append(error_message)
    else:
        errors[row] = [error_message]


def _validate_json_dict_unique_keys(json_dict_pairs: any) -> any:
    duplicated_keys: Set[str] = set()
    unique_json_dict: Dict[str, any] = {}
    key: str
    value: any
    for key, value in json_dict_pairs:
        if key in unique_json_dict:
            duplicated_keys.add(key)
        else:
            unique_json_dict[key] = value

    if duplicated_keys:
        raise ValueError(error_messages.INVALID_FORMAT_DUPLICATED_KEY_IN_JSON_ERROR_MESSAGE.format(
            list(duplicated_keys), f"root/{_RESOURCE_MAPPING_JSON_KEY_NAME}"))

    return unique_json_dict


def _validate_required_key_in_json_dict(json_dict: Dict[str, any], json_dict_name: str, key: str) -> None:
    if key not in json_dict.keys():
        raise KeyError(error_messages.INVALID_FORMAT_MISSING_REQUIRED_KEY_ERROR_MESSAGE.format(key, json_dict_name))


def convert_resources_to_json_dict(resources: List[ResourceMappingAttributes],
                                   old_json_dict: Dict[str, any]) -> Dict[str, any]:
    default_account_id: str = old_json_dict[RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME]
    default_region: str = old_json_dict[_RESOURCE_MAPPING_REGION_JSON_KEY_NAME]
    json_dict: Dict[str, any] = {RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME: default_account_id,
                                 _RESOURCE_MAPPING_REGION_JSON_KEY_NAME: default_region,
                                 _RESOURCE_MAPPING_VERSION_JSON_KEY_NAME: _RESOURCE_MAPPING_JSON_FORMAT_VERSION,
                                 _RESOURCE_MAPPING_JSON_KEY_NAME: {}}
    json_resources = {}
    resource: ResourceMappingAttributes
    for resource in resources:
        json_resource_attributes = {_RESOURCE_MAPPING_TYPE_JSON_KEY_NAME: resource.type,
                                    _RESOURCE_MAPPING_NAMEID_JSON_KEY_NAME: resource.name_id}
        if not resource.account_id == default_account_id:
            json_resource_attributes[RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME] = resource.account_id

        if not resource.region == default_region:
            json_resource_attributes[_RESOURCE_MAPPING_REGION_JSON_KEY_NAME] = resource.region

        json_resources[resource.key_name] = json_resource_attributes

    json_dict[_RESOURCE_MAPPING_JSON_KEY_NAME].update(json_resources)
    return json_dict


def convert_json_dict_to_resources(json_dict: Dict[str, any]) -> List[ResourceMappingAttributes]:
    default_account_id: str = json_dict[RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME]
    default_region: str = json_dict[_RESOURCE_MAPPING_REGION_JSON_KEY_NAME]

    resources: List[ResourceMappingAttributes] = []
    json_resources: Dict[str, any] = json_dict[_RESOURCE_MAPPING_JSON_KEY_NAME]
    if json_resources:
        resource_key: str
        resource_value: Dict[str, str]
        for resource_key, resource_value in json_resources.items():
            resource_builder = ResourceMappingAttributesBuilder() \
                .build_key_name(resource_key) \
                .build_type(resource_value[_RESOURCE_MAPPING_TYPE_JSON_KEY_NAME]) \
                .build_name_id(resource_value[_RESOURCE_MAPPING_NAMEID_JSON_KEY_NAME])

            if RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME in resource_value.keys():
                resource_builder.build_account_id(resource_value[RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME])
            else:
                resource_builder.build_account_id(default_account_id)

            if _RESOURCE_MAPPING_REGION_JSON_KEY_NAME in resource_value.keys():
                resource_builder.build_region(resource_value[_RESOURCE_MAPPING_REGION_JSON_KEY_NAME])
            else:
                resource_builder.build_region(default_region)

            resource_builder.build_status(
                ResourceMappingAttributesStatus(ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE,
                                                [ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE]))
            resources.append(resource_builder.build())
    return resources


def load_resource_mapping_json_schema(schema_path: str) -> None:
    global _RESOURCE_MAPPING_SCHEMA, _RESOURCE_MAPPING_SCHEMA_REQUIRED_ROOT_PROPERTIES, _RESOURCE_MAPPING_SCHEMA_REQUIRED_RESOURCE_PROPERTIES,\
        _RESOURCE_MAPPING_SCHEMA_ACCOUNTID_PATTERN, _RESOURCE_MAPPING_SCHEMA_REGION_PATTERN, _RESOURCE_MAPPING_SCHEMA_VERSION_PATTERN

    if not _RESOURCE_MAPPING_SCHEMA:
        # assume schema should be in correct format, and manually load expected pattern; tool will log error if schema is invalid
        _RESOURCE_MAPPING_SCHEMA = read_from_json_file(schema_path)
        _RESOURCE_MAPPING_SCHEMA_REQUIRED_ROOT_PROPERTIES = _RESOURCE_MAPPING_SCHEMA[_RESOURCE_MAPPING_SCHEMA_REQUIRED_PROPERTIES_KEY_NAME]
        schema_properties: Dict[str, any] = _RESOURCE_MAPPING_SCHEMA[_RESOURCE_MAPPING_SCHEMA_PROPERTIES_KEY_NAME]
        schema_pattern_properties: Dict[str, any] = \
            schema_properties[_RESOURCE_MAPPING_JSON_KEY_NAME][_RESOURCE_MAPPING_SCHEMA_PATTERN_PROPERTIES_KEY_NAME]
        _RESOURCE_MAPPING_SCHEMA_REQUIRED_RESOURCE_PROPERTIES = list(schema_pattern_properties.values())[0][_RESOURCE_MAPPING_SCHEMA_REQUIRED_PROPERTIES_KEY_NAME]
        _RESOURCE_MAPPING_SCHEMA_ACCOUNTID_PATTERN = \
            _RESOURCE_MAPPING_SCHEMA[_RESOURCE_MAPPING_SCHEMA_ACCOUNTID_KEY_NAME][_RESOURCE_MAPPING_SCHEMA_PROPERTY_PATTERN_KEY_NAME]
        _RESOURCE_MAPPING_SCHEMA_REGION_PATTERN = \
            _RESOURCE_MAPPING_SCHEMA[_RESOURCE_MAPPING_SCHEMA_REGION_KEY_NAME][_RESOURCE_MAPPING_SCHEMA_PROPERTY_PATTERN_KEY_NAME]
        _RESOURCE_MAPPING_SCHEMA_VERSION_PATTERN = \
            schema_properties[_RESOURCE_MAPPING_VERSION_JSON_KEY_NAME][_RESOURCE_MAPPING_SCHEMA_PROPERTY_PATTERN_KEY_NAME]


def read_from_json_file(file_name: str) -> Dict[str, any]:
    try:
        json_dict: Dict[str, any] = {}
        with open(file_name, 'r') as in_file:
            json_dict = json.load(in_file, object_pairs_hook=_validate_json_dict_unique_keys)
        return json_dict
    except json.decoder.JSONDecodeError as e:
        raw_messages: List[str] = str(e).split(":")
        if raw_messages:
            raise ValueError(error_messages.INVALID_JSON_FORMAT_FILE_WITH_POSITION_ERROR_MESSAGE.format(
                raw_messages[-1]))
        else:
            raise ValueError(error_messages.INVALID_JSON_FORMAT_FILE_ERROR_MESSAGE)
    except FileNotFoundError:
        raise FileNotFoundError(error_messages.FILE_NOT_FOUND_ERROR_MESSAGE.format(file_name))


def validate_resources_according_to_json_schema(resources: List[ResourceMappingAttributes]) -> Dict[int, List[str]]:
    invalid_resources: Dict[int, List[str]] = {}

    success_status_unique_keys: Set[str] = set()
    other_status_unique_keys: Set[str] = set()
    other_status_duplicated_keys: Set[str] = set()
    resource: ResourceMappingAttributes
    for resource in resources:
        if resource.status.value == ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE:
            success_status_unique_keys.add(resource.key_name)
        else:
            if resource.key_name in other_status_unique_keys:
                other_status_duplicated_keys.add(resource.key_name)
            else:
                other_status_unique_keys.add(resource.key_name)

    row_count: int = 0
    for resource in resources:
        if not resource.is_valid():
            _add_validation_error_message(
                invalid_resources, row_count, error_messages.INVALID_FORMAT_EMPTY_ATTRIBUTE_ERROR_MESSAGE)

        # if resource status is success, skip validation check
        if (not resource.status.value == ResourceMappingAttributesStatus.SUCCESS_STATUS_VALUE) \
                and (resource.key_name in success_status_unique_keys
                     or resource.key_name in other_status_duplicated_keys):
            _add_validation_error_message(
                invalid_resources, row_count,
                error_messages.INVALID_FORMAT_DUPLICATED_KEY_ERROR_MESSAGE.format(resource.key_name))

        if not re.match(_RESOURCE_MAPPING_SCHEMA_ACCOUNTID_PATTERN, resource.account_id):
            _add_validation_error_message(
                invalid_resources, row_count,
                error_messages.INVALID_FORMAT_UNEXPECTED_VALUE_IN_TABLE_ERROR_MESSAGE.format(
                    resource.account_id,
                    RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME,
                    _RESOURCE_MAPPING_SCHEMA_ACCOUNTID_PATTERN))

        if not re.match(_RESOURCE_MAPPING_SCHEMA_REGION_PATTERN, resource.region):
            _add_validation_error_message(
                invalid_resources, row_count,
                error_messages.INVALID_FORMAT_UNEXPECTED_VALUE_IN_TABLE_ERROR_MESSAGE.format(
                    resource.region, _RESOURCE_MAPPING_REGION_JSON_KEY_NAME,
                    _RESOURCE_MAPPING_SCHEMA_REGION_PATTERN))

        row_count += 1

    return invalid_resources


def validate_json_dict_according_to_json_schema(json_dict: Dict[str, any]) -> None:
    # The reason we keep this manual json schema validation is python missing supportive feature in default libs
    # When it is ready, we should be able to replace this with straightforward lib function call
    root_property: str
    for root_property in _RESOURCE_MAPPING_SCHEMA_REQUIRED_ROOT_PROPERTIES:
        _validate_required_key_in_json_dict(json_dict, "root", root_property)

    if not re.match(_RESOURCE_MAPPING_SCHEMA_ACCOUNTID_PATTERN, json_dict[RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME]):
        raise ValueError(error_messages.INVALID_FORMAT_UNEXPECTED_VALUE_IN_FILE_ERROR_MESSAGE.format(
            json_dict[RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME],
            f"root/{RESOURCE_MAPPING_ACCOUNTID_JSON_KEY_NAME}",
            _RESOURCE_MAPPING_SCHEMA_ACCOUNTID_PATTERN))

    if not re.match(_RESOURCE_MAPPING_SCHEMA_REGION_PATTERN, json_dict[_RESOURCE_MAPPING_REGION_JSON_KEY_NAME]):
        raise ValueError(error_messages.INVALID_FORMAT_UNEXPECTED_VALUE_IN_FILE_ERROR_MESSAGE.format(
            json_dict[_RESOURCE_MAPPING_REGION_JSON_KEY_NAME],
            f"root/{_RESOURCE_MAPPING_REGION_JSON_KEY_NAME}",
            _RESOURCE_MAPPING_SCHEMA_REGION_PATTERN))

    json_resources: Dict[str, any] = json_dict[_RESOURCE_MAPPING_JSON_KEY_NAME]
    if json_resources:
        resource_key: str
        resource_value: Dict[str, str]
        for resource_key, resource_value in json_resources.items():
            resource_property: str
            for resource_property in _RESOURCE_MAPPING_SCHEMA_REQUIRED_RESOURCE_PROPERTIES:
                _validate_required_key_in_json_dict(resource_value, resource_key, resource_property)


def write_into_json_file(file_name: str, json_dict: Dict[str, any]) -> None:
    with open(file_name, "w") as out_file:
        json.dump(json_dict, out_file, indent=4, sort_keys=True)
