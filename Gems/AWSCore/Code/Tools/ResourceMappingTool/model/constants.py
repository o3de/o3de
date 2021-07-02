"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from typing import List

# AWS resource types related constants
AWS_RESOURCE_CLOUDFORMATION_STACK_TYPE: str = "AWS::Cloudformation::Stack"
AWS_RESOURCE_TYPES: List[str] = ["AWS::Lambda::Function", "AWS::DynamoDB::Table", "AWS::S3::Bucket"]
AWS_RESOURCE_LAMBDA_FUNCTION_INDEX: int = 0
AWS_RESOURCE_DYNAMODB_TABLE_INDEX: int = 1
AWS_RESOURCE_S3_BUCKET_INDEX: int = 2

# AWS regions 
AWS_RESOURCE_REGIONS: List[str] = ["us-east-2", "us-east-1", "us-west-1", "us-west-2", "af-south-1",
                                   "ap-east-1", "ap-south-1", "ap-northeast-3", "ap-northeast-2", "ap-southeast-1",
                                   "ap-southeast-2", "ap-northeast-1", "ca-central-1", "cn-north-1", "cn-northwest-1",
                                   "eu-central-1", "eu-west-1", "eu-west-2", "eu-south-1", "eu-west-3",
                                   "eu-north-1", "me-south-1", "sa-east-1"]

# Default client&server config file name
RESOURCE_MAPPING_CONFIG_FILE_NAME_SUFFIX: str = "_aws_resource_mappings.json"
RESOURCE_MAPPING_DEFAULT_CONFIG_FILE_NAME: str = "default" + RESOURCE_MAPPING_CONFIG_FILE_NAME_SUFFIX

# View related constants
SEARCH_TYPED_RESOURCES_VERSION: str = "Import AWS Resources"
SEARCH_CFN_STACKS_VERSION: str = "Import CFN Stacks"
