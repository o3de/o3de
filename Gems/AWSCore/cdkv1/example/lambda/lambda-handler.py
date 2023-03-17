"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
