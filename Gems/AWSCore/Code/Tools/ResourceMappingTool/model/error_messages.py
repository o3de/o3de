"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

VIEW_EDIT_PAGE_SAVING_FAILED_WITH_INVALID_ROW_ERROR_MESSAGE: str = \
    "Row {} have errors. Please correct errors or delete the row to proceed."
VIEW_EDIT_PAGE_READ_FROM_JSON_FAILED_WITH_UNEXPECTED_FILE_ERROR_MESSAGE: str = \
    "Internal error: unexpected config file {}."

IMPORT_RESOURCES_PAGE_SEARCH_VERSION_ERROR_MESSAGE: str = "Internal error: unexpected import resources search version."
IMPORT_RESOURCES_PAGE_RESOURCE_TYPE_ERROR_MESSAGE: str = "Unexpected import AWS resource type."
IMPORT_RESOURCES_PAGE_NO_RESOURCES_SELECTED_ERROR_MESSAGE: str = "No resource selected for importing."

SINGLETON_OBJECT_ERROR_MESSAGE: str = "{} needs to be an singleton object."

INVALID_JSON_FORMAT_FILE_ERROR_MESSAGE: str = "Failed to read json config file, " \
                                              "please double check your file content"
INVALID_JSON_FORMAT_FILE_WITH_POSITION_ERROR_MESSAGE: str = "Failed to read json config file, " \
                                                            "please double check your file content at {}"

FILE_NOT_FOUND_ERROR_MESSAGE: str = "No such file or directory: {}"

AWS_SERVICE_REQUEST_CLIENT_ERROR_MESSAGE: str = "{} request call failed with client error [{}]: {}"

INVALID_FORMAT_EMPTY_ATTRIBUTE_ERROR_MESSAGE: str = "Invalid resource mapping entry, no empty attribute is allowed."
INVALID_FORMAT_MISSING_REQUIRED_KEY_ERROR_MESSAGE: str = "Missing required key \'{}\' under \'{}\'"
INVALID_FORMAT_DUPLICATED_KEY_ERROR_MESSAGE: str = "Found duplicated key \'{}\', please update with a new key name."
INVALID_FORMAT_DUPLICATED_KEY_IN_JSON_ERROR_MESSAGE: str = "Found duplicated key {} under \'{}\', " \
                                                           "please update with new key name."
INVALID_FORMAT_UNEXPECTED_VALUE_IN_TABLE_ERROR_MESSAGE: str = \
    "Found unexpected value \'{}\' at column \'{}\', please follow expected pattern {}"
INVALID_FORMAT_UNEXPECTED_VALUE_IN_FILE_ERROR_MESSAGE: str = \
    "Found unexpected value \'{}\' at \'{}\', please follow expected pattern {}"
