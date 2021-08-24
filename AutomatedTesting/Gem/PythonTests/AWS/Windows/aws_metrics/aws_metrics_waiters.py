"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import botocore.client
import logging

from datetime import timedelta
from AWS.common.custom_waiter import CustomWaiter, WaitState

logging.getLogger('boto').setLevel(logging.CRITICAL)


class KinesisAnalyticsApplicationUpdatedWaiter(CustomWaiter):
    """
    Subclass of the base custom waiter class.
    Wait for the Kinesis analytics application being updated to a specific status.
    """
    def __init__(self, client: botocore.client, status: str):
        """
        Initialize the waiter.

        :param client: Boto3 client to use.
        :param status: Expected status.
        """
        super().__init__(
            'KinesisAnalyticsApplicationUpdated',
            'DescribeApplication',
            'ApplicationDetail.ApplicationStatus',
            {status: WaitState.SUCCESS},
            client)

    def wait(self, application_name: str):
        """
        Wait for the expected status.

        :param application_name: Name of the Kinesis analytics application.
        """
        self._wait(ApplicationName=application_name)


class GlueCrawlerReadyWaiter(CustomWaiter):
    """
    Subclass of the base custom waiter class.
    Wait for the Glue crawler to finish its processing. Return when the crawler is in the "Stopping" status
    to avoid wasting too much time in the automation tests on its shutdown process.
    """
    def __init__(self, client: botocore.client):
        """
        Initialize the waiter.

        :param client: Boto3 client to use.
        """
        super().__init__(
            'GlueCrawlerReady',
            'GetCrawler',
            'Crawler.State',
            {'STOPPING': WaitState.SUCCESS},
            client)

    def wait(self, crawler_name):
        """
        Wait for the expected status.

        :param crawler_name: Name of the Glue crawler.
        """
        self._wait(Name=crawler_name)


class DataLakeMetricsDeliveredWaiter(CustomWaiter):
    """
    Subclass of the base custom waiter class.
    Wait for the expected directory being created in the S3 bucket.
    """
    def __init__(self, client: botocore.client):
        """
        Initialize the waiter.

        :param client: Boto3 client to use.
        """
        super().__init__(
            'DataLakeMetricsDelivered',
            'ListObjectsV2',
            'KeyCount > `0`',
            {True: WaitState.SUCCESS},
            client)

    def wait(self, bucket_name, prefix):
        """
        Wait for the expected directory being created.

        :param bucket_name: Name of the S3 bucket.
        :param prefix: Name of the expected directory prefix.
        """
        self._wait(Bucket=bucket_name, Prefix=prefix)


class CloudWatchMetricsDeliveredWaiter(CustomWaiter):
    """
    Subclass of the base custom waiter class.
    Wait for the expected metrics being delivered to CloudWatch.
    """
    def __init__(self, client: botocore.client):
        """
        Initialize the waiter.

        :param client: Boto3 client to use.
        """
        super().__init__(
            'CloudWatchMetricsDelivered',
            'GetMetricStatistics',
            'length(Datapoints) > `0`',
            {True: WaitState.SUCCESS},
            client)

    def wait(self, namespace, metrics_name, dimensions, start_time):
        """
        Wait for the expected metrics being delivered.

        :param namespace: Namespace of the metrics.
        :param metrics_name: Name of the metrics.
        :param dimensions: Dimensions of the metrics.
        :param start_time: Start time for generating the metrics.
        """
        self._wait(
            Namespace=namespace,
            MetricName=metrics_name,
            Dimensions=dimensions,
            StartTime=start_time,
            EndTime=start_time + timedelta(0, self.timeout),
            Period=60,
            Statistics=[
                'SampleCount'
            ],
            Unit='Count'
        )
