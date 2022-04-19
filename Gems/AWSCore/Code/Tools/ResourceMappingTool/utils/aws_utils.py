"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import boto3
from botocore.paginate import (PageIterator, Paginator)
from botocore.client import BaseClient
from botocore.exceptions import (BotoCoreError, ClientError, ConfigNotFound, NoCredentialsError, ProfileNotFound)
from typing import Dict, List

from model import error_messages
from model.basic_resource_attributes import (BasicResourceAttributes, BasicResourceAttributesBuilder)

"""
AWS Utils provide related functions to interact with aws service, e.g. getting
aws account, region, resources, etc.
"""

_PAGINATION_MAX_ITEMS: int = 10
_PAGINATION_PAGE_SIZE: int = 10

default_session: boto3.session.Session = None


class AWSConstants(object):
    CLOUDFORMATION_SERVICE_NAME: str = "cloudformation"
    CLOUDFORMATION_LIST_STACKS_API_NAME: str = "list_stacks"
    CLOUDFORMATION_LIST_STACK_RESOURCES_API_NAME: str = "list_stack_resources"
    CLOUDFORMATION_STACKS_STATUS_FILTERS: List[str] = ["CREATE_COMPLETE", "ROLLBACK_COMPLETE", "UPDATE_COMPLETE",
                                                       "IMPORT_COMPLETE", "IMPORT_ROLLBACK_COMPLETE"]

    LAMBDA_SERVICE_NAME: str = "lambda"
    LAMBDA_LIST_FUNCTIONS_API_NAME: str = "list_functions"

    DYNAMODB_SERVICE_NAME: str = "dynamodb"
    DYNAMODB_LIST_TABLES_API_NAME: str = "list_tables"

    STS_SERVICE_NAME: str = "sts"
    S3_SERVICE_NAME: str = "s3"


def _close_client_connection(client: BaseClient) -> None:
    session: boto3.session.Session = client._endpoint.http_session
    managers: List[object] = [session._manager, *session._proxy_managers.values()]
    for manager in managers:
        manager.clear()


def _initialize_boto3_aws_client(service: str, region: str = "") -> BaseClient:
    try:
        if region:
            boto3_client: BaseClient = default_session.client(service, region_name=region)
        else:
            boto3_client: BaseClient = default_session.client(service)
    except BotoCoreError as error:
        raise RuntimeError(error)

    boto3_client.meta.events.register(
        f"after-call.{service}.*", lambda **kwargs: _close_client_connection(boto3_client)
    )
    return boto3_client


def setup_default_session(profile: str) -> None:
    try:
        global default_session
        default_session = boto3.session.Session(profile_name=profile)
    except (ConfigNotFound, ProfileNotFound) as error:
        raise RuntimeError(error)


def get_default_account_id() -> str:
    sts_client: BaseClient = _initialize_boto3_aws_client(AWSConstants.STS_SERVICE_NAME)
    try:
        return sts_client.get_caller_identity()["Account"]
    except ClientError as error:
        raise RuntimeError(error_messages.AWS_SERVICE_REQUEST_CLIENT_ERROR_MESSAGE.format(
            "get_caller_identity", error.response['Error']['Code'], error.response['Error']['Message']))
    except NoCredentialsError as error:
        raise RuntimeError(error)


def get_default_region() -> str:
    region: str = default_session.region_name
    if region:
        return region
    
    sts_client: BaseClient = _initialize_boto3_aws_client(AWSConstants.STS_SERVICE_NAME)
    region = sts_client.meta.region_name
    if region:
        return region

    return ""


def list_s3_buckets(region: str = "") -> List[str]:
    s3_client: BaseClient = _initialize_boto3_aws_client(AWSConstants.S3_SERVICE_NAME, region)
    try:
        response: Dict[str, any] = s3_client.list_buckets()
    except ClientError as error:
        raise RuntimeError(error_messages.AWS_SERVICE_REQUEST_CLIENT_ERROR_MESSAGE.format(
            "list_buckets", error.response['Error']['Code'], error.response['Error']['Message']))

    bucket_names: List[str] = []
    bucket: Dict[str, any]
    for bucket in response["Buckets"]:
        try:
            bucket_name: str = bucket["Name"]
            location_response: Dict[str, any] = s3_client.get_bucket_location(Bucket=bucket_name)
            # Buckets in Region us-east-1 have a LocationConstraint of null .
            # https://boto3.amazonaws.com/v1/documentation/api/latest/reference/services/s3.html#S3.Client.get_bucket_location
            if ((location_response["LocationConstraint"] == region) or
                    (not location_response["LocationConstraint"] and region == "us-east-1")):
                bucket_names.append(bucket_name)
        except ClientError as error:
            raise RuntimeError(error_messages.AWS_SERVICE_REQUEST_CLIENT_ERROR_MESSAGE.format(
                "get_bucket_location", error.response['Error']['Code'], error.response['Error']['Message']))
    return bucket_names


def list_lambda_functions(region: str = "") -> List[str]:
    lambda_client: BaseClient = _initialize_boto3_aws_client(AWSConstants.LAMBDA_SERVICE_NAME, region)
    try:
        lambda_paginator: Paginator = lambda_client.get_paginator(AWSConstants.LAMBDA_LIST_FUNCTIONS_API_NAME)
        iterator: PageIterator = lambda_paginator.paginate(PaginationConfig={"PageSize": _PAGINATION_PAGE_SIZE})
        function_names: List[str] = []
        page: Dict[str, any]
        for page in iterator:
            function: Dict[str, any]
            for function in page["Functions"]:
                function_names.append(function["FunctionName"])
        return function_names
    except ClientError as error:
        raise RuntimeError(error_messages.AWS_SERVICE_REQUEST_CLIENT_ERROR_MESSAGE.format(
            AWSConstants.LAMBDA_LIST_FUNCTIONS_API_NAME,
            error.response['Error']['Code'], error.response['Error']['Message']))


def list_dynamodb_tables(region: str = "") -> List[str]:
    dynamodb_client: BaseClient = _initialize_boto3_aws_client(AWSConstants.DYNAMODB_SERVICE_NAME, region)
    try:
        dynamodb_paginator: Paginator = dynamodb_client.get_paginator(AWSConstants.DYNAMODB_LIST_TABLES_API_NAME)
        iterator: PageIterator = dynamodb_paginator.paginate(PaginationConfig={"PageSize": _PAGINATION_PAGE_SIZE})
        table_names: List[str] = []
        page: Dict[str, any]
        for page in iterator:
            table_names.extend(page["TableNames"])
        return table_names
    except ClientError as error:
        raise RuntimeError(error_messages.AWS_SERVICE_REQUEST_CLIENT_ERROR_MESSAGE.format(
            AWSConstants.DYNAMODB_LIST_TABLES_API_NAME,
            error.response['Error']['Code'], error.response['Error']['Message']))


def list_cloudformation_stacks(region: str = "") -> List[str]:
    cfn_client: BaseClient = _initialize_boto3_aws_client(AWSConstants.CLOUDFORMATION_SERVICE_NAME, region)
    try:
        cfn_paginator: Paginator = cfn_client.get_paginator(AWSConstants.CLOUDFORMATION_LIST_STACKS_API_NAME)
        iterator: PageIterator = \
            cfn_paginator.paginate(StackStatusFilter=AWSConstants.CLOUDFORMATION_STACKS_STATUS_FILTERS)
        stack_names: List[str] = []
        page: Dict[str, any]
        for page in iterator:
            stack: Dict[str, any]
            for stack in page["StackSummaries"]:
                stack_names.append(stack["StackName"])
        return stack_names
    except ClientError as error:
        raise RuntimeError(error_messages.AWS_SERVICE_REQUEST_CLIENT_ERROR_MESSAGE.format(
            AWSConstants.CLOUDFORMATION_LIST_STACKS_API_NAME,
            error.response['Error']['Code'], error.response['Error']['Message']))


def list_cloudformation_stack_resources(stack_name, region=None) -> List[BasicResourceAttributes]:
    cfn_client: BaseClient = _initialize_boto3_aws_client(AWSConstants.CLOUDFORMATION_SERVICE_NAME, region)
    try:
        cfn_paginator: Paginator = cfn_client.get_paginator(AWSConstants.CLOUDFORMATION_LIST_STACK_RESOURCES_API_NAME)
        resource_type_and_name: List[BasicResourceAttributes] = []

        starting_token: str = None
        while True:
            # list cloudformation stack resources with starting token and max items. StartingToken is used to mark
            # the starting point of the request; if None, it means request from start. MaxItems is used to define the
            # total number of resources requested in the page iterator.
            iterator: PageIterator = \
                cfn_paginator.paginate(StackName=stack_name,
                                       PaginationConfig={"MaxItems": _PAGINATION_MAX_ITEMS,
                                                         "StartingToken": starting_token})
            page: Dict[str, any]
            for page in iterator:
                # iterate through page iterator to fetch all resources
                resource: Dict[str, any]
                for resource in page["StackResourceSummaries"]:
                    if "ResourceType" in resource.keys() and "PhysicalResourceId" in resource.keys():
                        resource_type_and_name.append(BasicResourceAttributesBuilder()
                                                      .build_type(resource["ResourceType"])
                                                      .build_name_id(resource["PhysicalResourceId"])
                                                      .build())
            if iterator.resume_token is None:
                # when resume token is none, it means there is no more resources left
                break
            else:
                # setting starting token to resume token, then next request will query with proper starting point
                starting_token = iterator.resume_token
        return resource_type_and_name
    except ClientError as error:
        raise RuntimeError(error_messages.AWS_SERVICE_REQUEST_CLIENT_ERROR_MESSAGE.format(
            AWSConstants.CLOUDFORMATION_LIST_STACK_RESOURCES_API_NAME,
            error.response['Error']['Code'], error.response['Error']['Message']))
