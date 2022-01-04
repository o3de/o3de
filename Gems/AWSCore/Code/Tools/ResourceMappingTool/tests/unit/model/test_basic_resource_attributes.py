"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from unittest import TestCase
from model.basic_resource_attributes import (BasicResourceAttributes, BasicResourceAttributesBuilder)

class TestBasicResourceAttributes(TestCase):
    """
    BasicResourceAttributes unit test cases
    """

    def setUp(self) -> None:
        testResourceAttributes: BasicResourceAttributes = BasicResourceAttributes()
        testResourceAttributes.region = "us-east-1"
        testResourceAttributes.type = "AWS::S3::Bucket"
        testResourceAttributes.account_id = "123456789012"
        testResourceAttributes.name_id = "my-o3de-bucket-in-us-east-1"

        self._test_basic_resource_attributes = testResourceAttributes
    
    def test_is_valid(self) -> None:
        assert self._test_basic_resource_attributes.is_valid() == True

    def test_is_valid_no_accountid_ok(self) -> None:
        self._test_basic_resource_attributes.account_id = ""
        assert self._test_basic_resource_attributes.is_valid() == True

    def test_is_valid_no_type_invalid(self) -> None:
        self._test_basic_resource_attributes.type = ""
        assert self._test_basic_resource_attributes.is_valid() == False

    def test_is_valid_no_nameid_invalid(self) -> None:
        self._test_basic_resource_attributes.name_id = ""
        assert self._test_basic_resource_attributes.is_valid() == False

    def test_is_valid_no_region_invalid(self) -> None:
        self._test_basic_resource_attributes.region = ""
        assert self._test_basic_resource_attributes.is_valid() == False
