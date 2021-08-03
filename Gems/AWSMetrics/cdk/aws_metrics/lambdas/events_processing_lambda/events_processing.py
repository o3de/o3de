"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
