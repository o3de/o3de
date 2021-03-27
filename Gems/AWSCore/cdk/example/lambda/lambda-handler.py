"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""


def main(event: dict, context: dict) -> dict:
    """
    Simple lambda handler to echo back responses
    :param event:  A JSON-formatted document that contains data for a Lambda function to process.
    :param context: Provides methods and properties that provide information about the invocation,
    function, and runtime environment.
    :return: A response as json dict
    """
    print(event)

    return {
        'statusCode': 200,
        'body': event
    }
