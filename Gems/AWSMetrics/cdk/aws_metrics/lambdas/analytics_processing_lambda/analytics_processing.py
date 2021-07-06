"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import base64
import boto3
import datetime
import json


def lambda_handler(event: dict, context: dict) -> dict:
    """
    Deliver the Kinesis Data Analytics application output to Amazon CloudWatch as metrics.
    https://docs.aws.amazon.com/kinesisanalytics/latest/dev/how-it-works-output-lambda.html
    """
    cloudwatch = boto3.client('cloudwatch')

    results = []
    metrics_records = {}
    for record in event.get('records', []):
        bytes_array = base64.b64decode(record.get('data', ''))
        record_content_str = bytes_array.decode('utf8').replace("'", '"')
        record_content = json.loads(record_content_str)

        if record_content.get('OUTPUT_TYPE', '') == 'metrics':
            metrics_records[record.get('recordId', '')] = {
                'MetricName': record_content.get('METRIC_NAME', ''),
                'Timestamp': datetime.datetime.fromtimestamp(
                    record_content.get('METRIC_TIMESTAMP', 0) / 1e3),
                'Value': record_content.get('METRIC_UNIT_VALUE_INT', 0),
                'Unit': record_content.get('METRIC_UNIT', '')
            }

            # Each PutMetricsData request is limited to no more than 20 different metrics.
            # https://docs.aws.amazon.com/AmazonCloudWatch/latest/APIReference/API_PutMetricData.html
            if len(metrics_records.keys()) == 20:
                results += send_batched_metrics(cloudwatch, metrics_records)
                metrics_records = {}
        else:
            # Ignore the output type which we don't care about.
            results.append({
                'recordId': record.get('recordId', ''),
                'result': 'Ok'
            })

    if len(metrics_records.keys()) > 0:
        results += send_batched_metrics(cloudwatch, metrics_records)

    return {'records': results}


def send_batched_metrics(client, metrics_records):
    try:
        client.put_metric_data(
            MetricData=[record_data for record_id, record_data in metrics_records.items()],
            Namespace='AWSMetrics'
        )
    except Exception as e:
        # Kinesis Data Analytics application expects the result string to be either Ok or DeliveryFailed.
        print(f'Failed to deliver record to Amazon CloudWatch: {str(e)}')

        # Kinesis Data Analytics application will retry the failed submission.
        return [{
            'recordId': record_id,
            'result': 'DeliveryFailed'} for record_id in metrics_records]

    # Records are sent successfully to Amazon Cloud Watch.
    return [{
            'recordId': record_id,
            'result': 'Ok'} for record_id in metrics_records]
