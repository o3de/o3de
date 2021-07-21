"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import pathlib
import pytest
import typing

from datetime import datetime
from botocore.exceptions import WaiterError

from AWS.common.aws_utils import AwsUtils
from .aws_metrics_waiters import KinesisAnalyticsApplicationUpdatedWaiter, \
    CloudWatchMetricsDeliveredWaiter, DataLakeMetricsDeliveredWaiter, GlueCrawlerReadyWaiter

logging.getLogger('boto').setLevel(logging.CRITICAL)

# Expected directory and file extension for the S3 objects.
EXPECTED_S3_DIRECTORY = 'firehose_events/'
EXPECTED_S3_OBJECT_EXTENSION = '.parquet'


class AWSMetricsUtils:
    """
    Provide utils functions for the AWSMetrics gem to interact with the deployed resources.
    """

    def __init__(self, aws_utils: AwsUtils):
        self._aws_util = aws_utils

    def start_kinesis_data_analytics_application(self, application_name: str) -> None:
        """
        Start the Kenisis Data Analytics application for real-time analytics.
        :param application_name: Name of the Kenisis Data Analytics application.
        """
        input_id = self.get_kinesis_analytics_application_input_id(application_name)
        assert input_id, 'invalid Kinesis Data Analytics application input.'

        client = self._aws_util.client('kinesisanalytics')
        try:
            client.start_application(
                ApplicationName=application_name,
                InputConfigurations=[
                    {
                        'Id': input_id,
                        'InputStartingPositionConfiguration': {
                            'InputStartingPosition': 'NOW'
                        }
                    },
                ]
            )
        except client.exceptions.ResourceInUseException:
            # The application has been started.
            return

        try:
            KinesisAnalyticsApplicationUpdatedWaiter(client, 'RUNNING').wait(application_name=application_name)
        except WaiterError as e:
            assert False, f'Failed to start the Kinesis Data Analytics application: {str(e)}.'

    def get_kinesis_analytics_application_input_id(self, application_name: str) -> str:
        """
        Get the input ID for the Kenisis Data Analytics application.
        :param application_name: Name of the Kenisis Data Analytics application.
        :return: Input ID for the Kenisis Data Analytics application.
        """
        client = self._aws_util.client('kinesisanalytics')
        response = client.describe_application(
            ApplicationName=application_name
        )
        if not response:
            return ''
        input_descriptions = response.get('ApplicationDetail', {}).get('InputDescriptions', [])
        if len(input_descriptions) != 1:
            return ''

        return input_descriptions[0].get('InputId', '')

    def stop_kinesis_data_analytics_application(self, application_name: str) -> None:
        """
        Stop the Kenisis Data Analytics application.
        :param application_name: Name of the Kenisis Data Analytics application.
        """
        client = self._aws_util.client('kinesisanalytics')
        client.stop_application(
            ApplicationName=application_name
        )

        try:
            KinesisAnalyticsApplicationUpdatedWaiter(client, 'READY').wait(application_name=application_name)
        except WaiterError as e:
            assert False, f'Failed to stop the Kinesis Data Analytics application: {str(e)}.'

    def verify_cloud_watch_delivery(self, namespace: str, metrics_name: str,
                                    dimensions: typing.List[dict], start_time: datetime) -> None:
        """
        Verify that the expected metrics is delivered to CloudWatch.
        :param namespace: Namespace of the metrics.
        :param metrics_name: Name of the metrics.
        :param dimensions: Dimensions of the metrics.
        :param start_time: Start time for generating the metrics.
        """
        client = self._aws_util.client('cloudwatch')

        try:
            CloudWatchMetricsDeliveredWaiter(client).wait(
                namespace=namespace,
                metrics_name=metrics_name,
                dimensions=dimensions,
                start_time=start_time
            )
        except WaiterError as e:
            assert False, f'Failed to deliver metrics to CloudWatch: {str(e)}.'

    def verify_s3_delivery(self, analytics_bucket_name: str) -> None:
        """
        Verify that metrics are delivered to S3 for batch analytics successfully.
        :param analytics_bucket_name: Name of the deployed S3 bucket.
        """
        client = self._aws_util.client('s3')
        bucket_name = analytics_bucket_name

        try:
            DataLakeMetricsDeliveredWaiter(client).wait(bucket_name=bucket_name, prefix=EXPECTED_S3_DIRECTORY)
        except WaiterError as e:
            assert False, f'Failed to find the S3 directory for storing metrics data: {str(e)}.'

        # Check whether the data is converted to the expected data format.
        response = client.list_objects_v2(
            Bucket=bucket_name,
            Prefix=EXPECTED_S3_DIRECTORY
        )
        assert response.get('KeyCount', 0) != 0, f'Failed to deliver metrics to the S3 bucket {bucket_name}.'

        s3_objects = response.get('Contents', [])
        for s3_object in s3_objects:
            key = s3_object.get('Key', '')
            assert pathlib.Path(key).suffix == EXPECTED_S3_OBJECT_EXTENSION, \
                f'Invalid data format is found in the S3 bucket {bucket_name}'

    def run_glue_crawler(self, crawler_name: str) -> None:
        """
        Run the Glue crawler and wait for it to finish.
        :param crawler_name: Name of the Glue crawler
        """
        client = self._aws_util.client('glue')
        try:
            client.start_crawler(
                Name=crawler_name
            )
        except client.exceptions.CrawlerRunningException:
            # The crawler has already been started.
            return

        try:
            GlueCrawlerReadyWaiter(client).wait(crawler_name=crawler_name)
        except WaiterError as e:
            assert False, f'Failed to run the Glue crawler: {str(e)}.'

    def run_named_queries(self, work_group: str) -> None:
        """
        Run the named queries under the specific Athena work group.
        :param work_group: Name of the Athena work group.
        """
        client = self._aws_util.client('athena')
        # List all the named queries.
        response = client.list_named_queries(
            WorkGroup=work_group
        )
        named_query_ids = response.get('NamedQueryIds', [])

        # Run each of the queries.
        for named_query_id in named_query_ids:
            get_named_query_response = client.get_named_query(
                NamedQueryId=named_query_id
            )
            named_query = get_named_query_response.get('NamedQuery', {})

            start_query_execution_response = client.start_query_execution(
                QueryString=named_query.get('QueryString', ''),
                QueryExecutionContext={
                    'Database': named_query.get('Database', '')
                },
                WorkGroup=work_group
            )

            # Wait for the query to finish.
            state = 'RUNNING'
            while state == 'QUEUED' or state == 'RUNNING':
                get_query_execution_response = client.get_query_execution(
                    QueryExecutionId=start_query_execution_response.get('QueryExecutionId', '')
                )

                state = get_query_execution_response.get('QueryExecution', {}).get('Status', {}).get('State', '')

            assert state == 'SUCCEEDED', f'Failed to run the named query {named_query.get("Name", {})}'

    def empty_s3_bucket(self, bucket_name: str) -> None:
        """
        Empty the S3 bucket following:
        https://boto3.amazonaws.com/v1/documentation/api/latest/guide/migrations3.html

        :param bucket_name: Name of the S3 bucket.
        """

        s3 = self._aws_util.resource('s3')
        bucket = s3.Bucket(bucket_name)

        for key in bucket.objects.all():
            key.delete()

    def get_analytics_bucket_name(self, stack_name: str) -> str:
        """
        Get the name of the deployed S3 bucket.
        :param stack_name: Name of the CloudFormation stack.
        :return: Name of the deployed S3 bucket.
        """

        client = self._aws_util.client('cloudformation')

        response = client.describe_stack_resources(
            StackName=stack_name
        )
        resources = response.get('StackResources', [])

        for resource in resources:
            if resource.get('ResourceType') == 'AWS::S3::Bucket':
                return resource.get('PhysicalResourceId', '')

        return ''


@pytest.fixture(scope='function')
def aws_metrics_utils(
        request: pytest.fixture,
        aws_utils: pytest.fixture):
    """
    Fixture for the AWS metrics util functions.
    :param request:  _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :param aws_utils: aws_utils fixture.
    """
    aws_utils_obj = AWSMetricsUtils(aws_utils)
    return aws_utils_obj
