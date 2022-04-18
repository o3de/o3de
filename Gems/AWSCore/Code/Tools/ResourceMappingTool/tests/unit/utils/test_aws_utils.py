"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from typing import List
from unittest import TestCase
from unittest.mock import (ANY, call, MagicMock, patch)

from model import constants
from model.basic_resource_attributes import (BasicResourceAttributes, BasicResourceAttributesBuilder)
from utils import aws_utils

import pytest
from botocore.exceptions import (CredentialRetrievalError, PartialCredentialsError)

class TestAWSUtils(TestCase):
    """
    aws utils unit test cases
    TODO: add test cases once error handling is ready
    """
    _expected_account_id: str = "1234567890"
    _expected_region: str = "aws-global"

    _expected_bucket: str = "TestBucket"
    _expected_function: str = "TestFunction"
    _expected_table: str = "TestTable"
    _expected_stack: str = "TestStack"

    _expected_bucket_resource: BasicResourceAttributes = BasicResourceAttributesBuilder() \
        .build_type(constants.AWS_RESOURCE_TYPES[constants.AWS_RESOURCE_S3_BUCKET_INDEX]) \
        .build_name_id(_expected_bucket) \
        .build()

    def setUp(self) -> None:
        session_patcher: patch = patch("boto3.session.Session")
        self.addCleanup(session_patcher.stop)
        self._mock_session: MagicMock = session_patcher.start()
        self._mock_client: MagicMock = self._mock_session.return_value.client

        aws_utils.setup_default_session("default")

    def test_get_default_account_id_return_expected_account_id(self) -> None:
        mocked_sts_client: MagicMock = self._mock_client.return_value
        mocked_sts_client.get_caller_identity.return_value = {"Account": TestAWSUtils._expected_account_id}

        actual_account_id: str = aws_utils.get_default_account_id()
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.STS_SERVICE_NAME)
        mocked_sts_client.get_caller_identity.assert_called_once()
        assert actual_account_id == TestAWSUtils._expected_account_id

    def test_get_default_account_id_raise_credential_retrieval_error(self) -> None:
        self._mock_session.return_value.client.side_effect=CredentialRetrievalError(provider="custom-process", error_msg="")
        with pytest.raises(RuntimeError) as error:
            aws_utils.get_default_account_id()
        assert str(error.value) == 'Error when retrieving credentials from custom-process: '

    def test_get_default_account_id_raise_partial_credentials_error(self) -> None:
        self._mock_session.return_value.client.side_effect=PartialCredentialsError(provider="custom-process", cred_var="access_key")
        with pytest.raises(RuntimeError) as error:
            aws_utils.get_default_account_id()
        assert str(error.value) == 'Partial credentials found in custom-process, missing: access_key'

    def test_get_default_region_return_expected_region_from_session(self) -> None:
        mocked_session = self._mock_session.return_value
        mocked_session.region_name = TestAWSUtils._expected_region

        actual_region: str = aws_utils.get_default_region()
        self._mock_session.assert_called_once()
        assert actual_region == TestAWSUtils._expected_region

    def test_get_default_region_return_expected_region_from_sts_client(self) -> None:
        mocked_session: MagicMock = self._mock_session.return_value
        mocked_session.region_name = ""
        mocked_sts_client: MagicMock = self._mock_client.return_value
        mocked_sts_client.meta.region_name = TestAWSUtils._expected_region

        actual_region: str = aws_utils.get_default_region()
        self._mock_session.assert_called_once()
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.STS_SERVICE_NAME)
        assert actual_region == TestAWSUtils._expected_region

    def test_get_default_region_return_empty(self) -> None:
        mocked_session: MagicMock = self._mock_session.return_value
        mocked_session.region_name = ""
        mocked_sts_client: MagicMock = self._mock_client.return_value
        mocked_sts_client.meta.region_name = ""

        actual_region: str = aws_utils.get_default_region()
        self._mock_session.assert_called_once()
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.STS_SERVICE_NAME)
        assert not actual_region

    def test_list_s3_buckets_return_empty_list(self) -> None:
        mocked_s3_client: MagicMock = self._mock_client.return_value
        mocked_s3_client.list_buckets.return_value = {"Buckets": {}}

        actual_buckets: List[str] = aws_utils.list_s3_buckets()
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.S3_SERVICE_NAME)
        mocked_s3_client.list_buckets.assert_called_once()
        assert not actual_buckets

    def test_list_s3_buckets_return_empty_list_with_given_region(self) -> None:
        mocked_s3_client: MagicMock = self._mock_client.return_value
        mocked_s3_client.list_buckets.return_value = {"Buckets": []}

        actual_buckets: List[str] = aws_utils.list_s3_buckets(TestAWSUtils._expected_region)
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.S3_SERVICE_NAME,
                                                  region_name=TestAWSUtils._expected_region)
        mocked_s3_client.list_buckets.assert_called_once()
        assert not actual_buckets

    def test_list_s3_buckets_return_empty_list_with_no_matching_region(self) -> None:
        expected_region: str = "us-east-1"
        mocked_s3_client: MagicMock = self._mock_client.return_value
        expected_buckets: List[str] = [f"{TestAWSUtils._expected_bucket}1", f"{TestAWSUtils._expected_bucket}2"]
        mocked_s3_client.list_buckets.return_value = {"Buckets": [{"Name": expected_buckets[0]},
                                                                  {"Name": expected_buckets[1]}]}
        mocked_s3_client.get_bucket_location.side_effect = [{"LocationConstraint": "us-east-2"},
                                                            {"LocationConstraint": "us-west-1"}]

        actual_buckets: List[str] = aws_utils.list_s3_buckets(expected_region)
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.S3_SERVICE_NAME,
                                                  region_name=expected_region)
        mocked_s3_client.list_buckets.assert_called_once()
        assert not actual_buckets

    def test_list_s3_buckets_return_expected_buckets_matching_region(self) -> None:
        expected_region: str = "us-west-2"
        mocked_s3_client: MagicMock = self._mock_client.return_value
        expected_buckets: List[str] = [f"{TestAWSUtils._expected_bucket}1", f"{TestAWSUtils._expected_bucket}2"]
        mocked_s3_client.list_buckets.return_value = {"Buckets": [{"Name": expected_buckets[0]},
                                                                  {"Name": expected_buckets[1]}]}
        mocked_s3_client.get_bucket_location.side_effect = [{"LocationConstraint": "us-west-2"},
                                                            {"LocationConstraint": "us-west-1"}]

        actual_buckets: List[str] = aws_utils.list_s3_buckets(expected_region)
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.S3_SERVICE_NAME,
                                                  region_name=expected_region)
        mocked_s3_client.list_buckets.assert_called_once()
        assert len(actual_buckets) == 1
        assert actual_buckets[0] == expected_buckets[0]

    def test_list_s3_buckets_return_expected_iad_buckets(self) -> None:
        expected_region: str = "us-east-1"
        mocked_s3_client: MagicMock = self._mock_client.return_value
        expected_buckets: List[str] = [f"{TestAWSUtils._expected_bucket}1", f"{TestAWSUtils._expected_bucket}2"]
        mocked_s3_client.list_buckets.return_value = {"Buckets": [{"Name": expected_buckets[0]},
                                                                  {"Name": expected_buckets[1]}]}
        mocked_s3_client.get_bucket_location.side_effect = [{"LocationConstraint": None},
                                                            {"LocationConstraint": "us-west-1"}]

        actual_buckets: List[str] = aws_utils.list_s3_buckets(expected_region)
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.S3_SERVICE_NAME,
                                                  region_name=expected_region)
        mocked_s3_client.list_buckets.assert_called_once()
        assert len(actual_buckets) == 1
        assert actual_buckets[0] == expected_buckets[0]

    def test_list_lambda_functions_return_empty_list(self) -> None:
        mocked_lambda_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_lambda_client.get_paginator.return_value = mocked_paginator
        mocked_iterator: MagicMock = MagicMock()
        mocked_paginator.paginate.return_value = mocked_iterator
        mocked_iterator.__iter__.return_value = [{"Functions": []}]

        actual_functions: List[str] = aws_utils.list_lambda_functions()
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.LAMBDA_SERVICE_NAME)
        mocked_lambda_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.LAMBDA_LIST_FUNCTIONS_API_NAME)
        mocked_paginator.paginate.assert_called_once_with(
            PaginationConfig={"PageSize": aws_utils._PAGINATION_PAGE_SIZE})
        assert not actual_functions

    def test_list_lambda_functions_return_expected_functions(self) -> None:
        mocked_lambda_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_lambda_client.get_paginator.return_value = mocked_paginator
        mocked_iterator: MagicMock = MagicMock()
        mocked_paginator.paginate.return_value = mocked_iterator
        expected_functions: List[str] = [f"{TestAWSUtils._expected_function}1", f"{TestAWSUtils._expected_function}2"]
        mocked_iterator.__iter__.return_value = [{"Functions": [{"FunctionName": expected_functions[0]},
                                                                {"FunctionName": expected_functions[1]}]}]

        actual_functions: List[str] = aws_utils.list_lambda_functions()
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.LAMBDA_SERVICE_NAME)
        mocked_lambda_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.LAMBDA_LIST_FUNCTIONS_API_NAME)
        mocked_paginator.paginate.assert_called_once_with(
            PaginationConfig={"PageSize": aws_utils._PAGINATION_PAGE_SIZE})
        assert actual_functions == expected_functions

    def test_list_dynamodb_tables_return_empty_list(self) -> None:
        mocked_dynamodb_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_dynamodb_client.get_paginator.return_value = mocked_paginator
        mocked_iterator: MagicMock = MagicMock()
        mocked_paginator.paginate.return_value = mocked_iterator
        mocked_iterator.__iter__.return_value = [{"TableNames": []}]

        actual_functions: List[str] = aws_utils.list_dynamodb_tables()
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.DYNAMODB_SERVICE_NAME)
        mocked_dynamodb_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.DYNAMODB_LIST_TABLES_API_NAME)
        mocked_paginator.paginate.assert_called_once_with(
            PaginationConfig={"PageSize": aws_utils._PAGINATION_PAGE_SIZE})
        assert not actual_functions

    def test_list_dynamodb_tables_return_expected_tables(self) -> None:
        mocked_dynamodb_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_dynamodb_client.get_paginator.return_value = mocked_paginator
        mocked_iterator: MagicMock = MagicMock()
        mocked_paginator.paginate.return_value = mocked_iterator
        expected_tables: List[str] = [f"{TestAWSUtils._expected_table}1", f"{TestAWSUtils._expected_table}2"]
        mocked_iterator.__iter__.return_value = [{"TableNames": expected_tables}]

        actual_tables: List[str] = aws_utils.list_dynamodb_tables()
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.DYNAMODB_SERVICE_NAME)
        mocked_dynamodb_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.DYNAMODB_LIST_TABLES_API_NAME)
        mocked_paginator.paginate.assert_called_once_with(
            PaginationConfig={"PageSize": aws_utils._PAGINATION_PAGE_SIZE})
        assert actual_tables == expected_tables

    def test_list_cloudformation_stacks_return_empty_list(self) -> None:
        mocked_cloudformation_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_cloudformation_client.get_paginator.return_value = mocked_paginator
        mocked_iterator: MagicMock = MagicMock()
        mocked_paginator.paginate.return_value = mocked_iterator
        mocked_iterator.__iter__.return_value = [{"StackSummaries": []}]

        actual_stacks: List[str] = aws_utils.list_cloudformation_stacks()
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.CLOUDFORMATION_SERVICE_NAME)
        mocked_cloudformation_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.CLOUDFORMATION_LIST_STACKS_API_NAME)
        mocked_paginator.paginate.assert_called_once_with(
            StackStatusFilter=aws_utils.AWSConstants.CLOUDFORMATION_STACKS_STATUS_FILTERS)
        assert not actual_stacks

    def test_list_cloudformation_stacks_return_empty_list_with_given_region(self) -> None:
        mocked_cloudformation_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_cloudformation_client.get_paginator.return_value = mocked_paginator
        mocked_iterator: MagicMock = MagicMock()
        mocked_paginator.paginate.return_value = mocked_iterator
        mocked_iterator.__iter__.return_value = [{"StackSummaries": []}]

        actual_stacks: List[str] = aws_utils.list_cloudformation_stacks(TestAWSUtils._expected_region)
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.CLOUDFORMATION_SERVICE_NAME,
                                                  region_name=TestAWSUtils._expected_region)
        mocked_cloudformation_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.CLOUDFORMATION_LIST_STACKS_API_NAME)
        mocked_paginator.paginate.assert_called_once_with(
            StackStatusFilter=aws_utils.AWSConstants.CLOUDFORMATION_STACKS_STATUS_FILTERS)
        assert not actual_stacks

    def test_list_cloudformation_stacks_return_expected_stacks(self) -> None:
        mocked_cloudformation_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_cloudformation_client.get_paginator.return_value = mocked_paginator
        mocked_iterator: MagicMock = MagicMock()
        mocked_paginator.paginate.return_value = mocked_iterator
        expected_stacks: List[str] = [f"{TestAWSUtils._expected_stack}1", f"{TestAWSUtils._expected_stack}2"]
        mocked_iterator.__iter__.return_value = [{"StackSummaries": [{"StackName": expected_stacks[0]},
                                                                     {"StackName": expected_stacks[1]}]}]

        actual_stacks: List[str] = aws_utils.list_cloudformation_stacks()
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.CLOUDFORMATION_SERVICE_NAME)
        mocked_cloudformation_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.CLOUDFORMATION_LIST_STACKS_API_NAME)
        mocked_paginator.paginate.assert_called_once_with(
            StackStatusFilter=aws_utils.AWSConstants.CLOUDFORMATION_STACKS_STATUS_FILTERS)
        assert actual_stacks == expected_stacks

    def test_list_cloudformation_stack_resources_return_empty_list(self) -> None:
        mocked_cloudformation_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_cloudformation_client.get_paginator.return_value = mocked_paginator
        mocked_iterator: MagicMock = MagicMock()
        mocked_iterator.resume_token = None
        mocked_paginator.paginate.return_value = mocked_iterator
        mocked_iterator.__iter__.return_value = [{"StackResourceSummaries": []}]

        actual_stack_resources: List[BasicResourceAttributes] = \
            aws_utils.list_cloudformation_stack_resources(TestAWSUtils._expected_stack)
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.CLOUDFORMATION_SERVICE_NAME)
        mocked_cloudformation_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.CLOUDFORMATION_LIST_STACK_RESOURCES_API_NAME)
        mocked_paginator.paginate.assert_called_once_with(StackName=TestAWSUtils._expected_stack, PaginationConfig=ANY)
        assert not actual_stack_resources

    def test_list_cloudformation_stack_resources_return_empty_list_with_given_region(self) -> None:
        mocked_cloudformation_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_cloudformation_client.get_paginator.return_value = mocked_paginator
        mocked_iterator: MagicMock = MagicMock()
        mocked_iterator.resume_token = None
        mocked_paginator.paginate.return_value = mocked_iterator
        mocked_iterator.__iter__.return_value = [{"StackResourceSummaries": []}]

        actual_stack_resources: List[BasicResourceAttributes] = \
            aws_utils.list_cloudformation_stack_resources(TestAWSUtils._expected_stack, TestAWSUtils._expected_region)
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.CLOUDFORMATION_SERVICE_NAME,
                                                  region_name=TestAWSUtils._expected_region)
        mocked_cloudformation_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.CLOUDFORMATION_LIST_STACK_RESOURCES_API_NAME)
        mocked_paginator.paginate.assert_called_once_with(StackName=TestAWSUtils._expected_stack, PaginationConfig=ANY)
        assert not actual_stack_resources

    def test_list_cloudformation_stack_resources_return_empty_list_when_resource_has_invalid_attributes(self) -> None:
        mocked_cloudformation_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_cloudformation_client.get_paginator.return_value = mocked_paginator
        mocked_iterator: MagicMock = MagicMock()
        mocked_iterator.resume_token = None
        mocked_paginator.paginate.return_value = mocked_iterator
        mocked_iterator.__iter__.return_value = [{"StackResourceSummaries": [
            {"DummyAttribute": "DummyValue"}]}]

        actual_stack_resources: List[BasicResourceAttributes] = \
            aws_utils.list_cloudformation_stack_resources(TestAWSUtils._expected_stack, TestAWSUtils._expected_region)
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.CLOUDFORMATION_SERVICE_NAME,
                                                  region_name=TestAWSUtils._expected_region)
        mocked_cloudformation_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.CLOUDFORMATION_LIST_STACK_RESOURCES_API_NAME)
        mocked_paginator.paginate.assert_called_once_with(StackName=TestAWSUtils._expected_stack, PaginationConfig=ANY)
        assert not actual_stack_resources

    def test_list_cloudformation_stack_resources_return_expected_stack_resources(self) -> None:
        mocked_cloudformation_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_cloudformation_client.get_paginator.return_value = mocked_paginator
        mocked_iterator: MagicMock = MagicMock()
        mocked_iterator.resume_token = None
        mocked_iterator.__iter__.return_value = [{"StackResourceSummaries": [
            {"ResourceType": TestAWSUtils._expected_bucket_resource.type,
             "PhysicalResourceId": TestAWSUtils._expected_bucket_resource.name_id}]}]
        mocked_paginator.paginate.return_value = mocked_iterator

        actual_stack_resources: List[BasicResourceAttributes] = aws_utils.list_cloudformation_stack_resources(
            TestAWSUtils._expected_stack)
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.CLOUDFORMATION_SERVICE_NAME)
        mocked_cloudformation_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.CLOUDFORMATION_LIST_STACK_RESOURCES_API_NAME)
        mocked_paginator.paginate.assert_called_once_with(StackName=TestAWSUtils._expected_stack, PaginationConfig=ANY)
        assert actual_stack_resources == [TestAWSUtils._expected_bucket_resource]

    def test_list_cloudformation_stack_resources_return_expected_stack_resources_with_separate_iterator(self) -> None:
        mocked_cloudformation_client: MagicMock = self._mock_client.return_value
        mocked_paginator: MagicMock = MagicMock()
        mocked_cloudformation_client.get_paginator.return_value = mocked_paginator
        mocked_iterator1: MagicMock = MagicMock()
        expected_starting_token: str = "starting_token"
        mocked_iterator1.resume_token = expected_starting_token
        mocked_iterator1.__iter__.return_value = [{"StackResourceSummaries": [
            {"ResourceType": TestAWSUtils._expected_bucket_resource.type,
             "PhysicalResourceId": TestAWSUtils._expected_bucket_resource.name_id}]}]
        mocked_iterator2: MagicMock = MagicMock()
        mocked_iterator2.resume_token = None
        mocked_iterator2.__iter__.return_value = [{"StackResourceSummaries": [
            {"ResourceType": TestAWSUtils._expected_bucket_resource.type,
             "PhysicalResourceId": TestAWSUtils._expected_bucket_resource.name_id}]}]
        mocked_paginator.paginate.side_effect = [mocked_iterator1, mocked_iterator2]

        actual_stack_resources: List[BasicResourceAttributes] = aws_utils.list_cloudformation_stack_resources(
            TestAWSUtils._expected_stack)
        self._mock_client.assert_called_once_with(aws_utils.AWSConstants.CLOUDFORMATION_SERVICE_NAME)
        mocked_cloudformation_client.get_paginator.assert_called_once_with(
            aws_utils.AWSConstants.CLOUDFORMATION_LIST_STACK_RESOURCES_API_NAME)
        mocked_calls: List[call] = [
            call(StackName=TestAWSUtils._expected_stack, PaginationConfig={"MaxItems": ANY, "StartingToken": None}),
            call(StackName=TestAWSUtils._expected_stack,
                 PaginationConfig={"MaxItems": ANY, "StartingToken": expected_starting_token})]
        mocked_paginator.paginate.assert_has_calls(mocked_calls)
        assert actual_stack_resources == [TestAWSUtils._expected_bucket_resource,
                                          TestAWSUtils._expected_bucket_resource]
