"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import hashlib

MAX_RESOURCE_NAME_LENGTH_MAPPING = {
    'athena_work_group': 128,
    'athena_named_query': 128,
    'cloudformation_stack': 128,
    'cloudwatch_dashboard': 255,
    'cloudwatch_log_group': 512,
    'firehose_delivery_stream': 64,
    'iam_managed_policy': 144,
    'iam_role': 64,
    'kinesis_application': 128,
    'kinesis_stream': 128,
    'lambda_function': 64,
    's3_bucket': 63
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
