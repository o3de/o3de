"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import hashlib
from aws_cdk import (
    core,
    aws_cognito as cognito,
    aws_iam as iam
)

MAX_RESOURCE_NAME_LENGTH_MAPPING = {
    core.Stack.__name__: 128,
    iam.Role.__name__: 64,
    iam.ManagedPolicy.__name__: 144,
    cognito.CfnUserPoolClient.__name__: 128,
    cognito.CfnUserPool.__name__: 128,
    cognito.CfnIdentityPool.__name__: 128

}


def sanitize_resource_name(resource_name: str, resource_type: str) -> str:
    """
    Truncate the resource name if its length exceeds the limit.
    This is the best effort for sanitizing resource names based on the AWS documents since each AWS service
    has its unique restrictions. Customers can extend this function for validation or sanitization.

    :param resource_name: Original name of the resource.
    :param resource_type: Type of the resource.
    :return Sanitized resource name that can be deployed with AWS.
    """
    result = resource_name
    if not MAX_RESOURCE_NAME_LENGTH_MAPPING.get(resource_type):
        return result

    if len(resource_name) > MAX_RESOURCE_NAME_LENGTH_MAPPING[resource_type]:
        # PYTHONHASHSEED is set to "random" by default in Python 3.3 and up. Cannot use
        # the built-in hash function here since it will give a different return value in each session
        digest = "-%x" % (int(hashlib.md5(resource_name.encode('ascii', 'ignore')).hexdigest(), 16) & 0xffffffff)
        result = resource_name[:MAX_RESOURCE_NAME_LENGTH_MAPPING[resource_type] - len(digest)] + digest
    return result
