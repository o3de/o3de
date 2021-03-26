"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""


def lambda_handler(event: dict, context: dict):
    """
    Return the original data without any modification. This lambda can be modified for the custom transformation:
    https://docs.aws.amazon.com/firehose/latest/dev/data-transformation.html
    """
    results = []

    for record in event.get('records', []):
        results.append({
            'recordId': record.get('recordId', ''),
            'result': 'Ok',
            'data': record.get('data', '')
        })

    return {'records': results}
