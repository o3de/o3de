"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import os
import pytest
import typing
from datetime import datetime

import ly_test_tools.log.log_monitor

from AWS.common import constants
from .aws_metrics_custom_thread import AWSMetricsThread

# fixture imports
from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor
from .aws_metrics_utils import aws_metrics_utils

AWS_METRICS_FEATURE_NAME = 'AWSMetrics'

logger = logging.getLogger(__name__)


def setup(launcher: pytest.fixture,
          asset_processor: pytest.fixture) -> pytest.fixture:
    """
    Set up the resource mapping configuration and start the log monitor.
    :param launcher: Client launcher for running the test level.
    :param asset_processor: asset_processor fixture.
    :return log monitor object.
    """
    asset_processor.start()
    asset_processor.wait_for_idle()

    file_to_monitor = os.path.join(launcher.workspace.paths.project_log(), constants.GAME_LOG_NAME)

    # Initialize the log monitor.
    log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

    return log_monitor


def monitor_metrics_submission(log_monitor: pytest.fixture) -> None:
    """
    Monitor the messages and notifications for submitting metrics.
    :param log_monitor: Log monitor to check the log messages.
    """
    expected_lines = [
        '(Script) - Submitted metrics without buffer.',
        '(Script) - Submitted metrics with buffer.',
        '(Script) - Flushed the buffered metrics.',
        '(Script) - Metrics is sent successfully.'
    ]

    unexpected_lines = [
        '(Script) - Failed to submit metrics without buffer.',
        '(Script) - Failed to submit metrics with buffer.',
        '(Script) - Failed to send metrics.'
    ]

    result = log_monitor.monitor_log_for_lines(
        expected_lines=expected_lines,
        unexpected_lines=unexpected_lines,
        halt_on_unexpected=True)

    # Assert the log monitor detected expected lines and did not detect any unexpected lines.
    assert result, (
        f'Log monitoring failed. Used expected_lines values: {expected_lines} & '
        f'unexpected_lines values: {unexpected_lines}')


def query_metrics_from_s3(aws_metrics_utils: pytest.fixture, resource_mappings: pytest.fixture) -> None:
    """
    Verify that the metrics events are delivered to the S3 bucket and can be queried.
    :param aws_metrics_utils: aws_metrics_utils fixture.
    :param resource_mappings: resource_mappings fixture.
    """
    aws_metrics_utils.verify_s3_delivery(
        resource_mappings.get_resource_name_id('AWSMetrics.AnalyticsBucketName')
    )
    logger.info('Metrics are sent to S3.')

    aws_metrics_utils.run_glue_crawler(
        resource_mappings.get_resource_name_id('AWSMetrics.EventsCrawlerName'))

    # Remove the events_json table if exists so that the sample query can create a table with the same name.
    aws_metrics_utils.delete_table(resource_mappings.get_resource_name_id('AWSMetrics.EventDatabaseName'), 'events_json')
    aws_metrics_utils.run_named_queries(resource_mappings.get_resource_name_id('AWSMetrics.AthenaWorkGroupName'))
    logger.info('Query metrics from S3 successfully.')


def verify_operational_metrics(aws_metrics_utils: pytest.fixture,
                               resource_mappings: pytest.fixture, start_time: datetime) -> None:
    """
    Verify that operational health metrics are delivered to CloudWatch.
    :param aws_metrics_utils: aws_metrics_utils fixture.
    :param resource_mappings: resource_mappings fixture.
    :param start_time: Time when the game launcher starts.
    """
    aws_metrics_utils.verify_cloud_watch_delivery(
        'AWS/Lambda',
        'Invocations',
        [{'Name': 'FunctionName',
          'Value': resource_mappings.get_resource_name_id('AWSMetrics.AnalyticsProcessingLambdaName')}],
        start_time)
    logger.info('AnalyticsProcessingLambda metrics are sent to CloudWatch.')

    aws_metrics_utils.verify_cloud_watch_delivery(
        'AWS/Lambda',
        'Invocations',
        [{'Name': 'FunctionName',
          'Value': resource_mappings.get_resource_name_id('AWSMetrics.EventProcessingLambdaName')}],
        start_time)
    logger.info('EventsProcessingLambda metrics are sent to CloudWatch.')


def update_kinesis_analytics_application_status(aws_metrics_utils: pytest.fixture,
                                                resource_mappings: pytest.fixture, start_application: bool) -> None:
    """
    Update the Kinesis analytics application to start or stop it.
    :param aws_metrics_utils: aws_metrics_utils fixture.
    :param resource_mappings: resource_mappings fixture.
    :param start_application: whether to start or stop the application.
    """
    if start_application:
        aws_metrics_utils.start_kinesis_data_analytics_application(
            resource_mappings.get_resource_name_id('AWSMetrics.AnalyticsApplicationName'))
    else:
        aws_metrics_utils.stop_kinesis_data_analytics_application(
            resource_mappings.get_resource_name_id('AWSMetrics.AnalyticsApplicationName'))

@pytest.mark.SUITE_awsi
@pytest.mark.usefixtures('automatic_process_killer')
@pytest.mark.usefixtures('aws_credentials')
@pytest.mark.usefixtures('resource_mappings')
@pytest.mark.parametrize('assume_role_arn', [constants.ASSUME_ROLE_ARN])
@pytest.mark.parametrize('feature_name', [AWS_METRICS_FEATURE_NAME])
@pytest.mark.parametrize('profile_name', ['AWSAutomationTest'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('region_name', [constants.AWS_REGION])
@pytest.mark.parametrize('resource_mappings_filename', [constants.AWS_RESOURCE_MAPPING_FILE_NAME])
@pytest.mark.parametrize('session_name', [constants.SESSION_NAME])
@pytest.mark.parametrize('stacks', [[f'{constants.AWS_PROJECT_NAME}-{AWS_METRICS_FEATURE_NAME}-{constants.AWS_REGION}']])
class TestAWSMetricsWindows(object):
    """
    Test class to verify the real-time and batch analytics for metrics.
    """
    @pytest.mark.parametrize('level', ['AWS/Metrics'])
    def test_realtime_and_batch_analytics(self,
                                          level: str,
                                          launcher: pytest.fixture,
                                          asset_processor: pytest.fixture,
                                          workspace: pytest.fixture,
                                          aws_utils: pytest.fixture,
                                          resource_mappings: pytest.fixture,
                                          aws_metrics_utils: pytest.fixture):
        """
        Verify that the metrics events are sent to CloudWatch and S3 for analytics.
        """
        # Start Kinesis analytics application on a separate thread to avoid blocking the test.
        kinesis_analytics_application_thread = AWSMetricsThread(target=update_kinesis_analytics_application_status,
                                                                args=(aws_metrics_utils, resource_mappings, True))
        kinesis_analytics_application_thread.start()

        log_monitor = setup(launcher, asset_processor)

        # Kinesis analytics application needs to be in the running state before we start the game launcher.
        kinesis_analytics_application_thread.join()
        launcher.args = ['+LoadLevel', level]
        launcher.args.extend(['-rhi=null'])
        start_time = datetime.utcnow()
        with launcher.start(launch_ap=False):
            monitor_metrics_submission(log_monitor)

            # Verify that real-time analytics metrics are delivered to CloudWatch.
            aws_metrics_utils.verify_cloud_watch_delivery(
                AWS_METRICS_FEATURE_NAME,
                'TotalLogins',
                [],
                start_time)
            logger.info('Real-time metrics are sent to CloudWatch.')

        # Run time-consuming operations on separate threads to avoid blocking the test.
        operational_threads = list()
        operational_threads.append(
            AWSMetricsThread(target=query_metrics_from_s3,
                             args=(aws_metrics_utils, resource_mappings)))
        operational_threads.append(
            AWSMetricsThread(target=verify_operational_metrics,
                             args=(aws_metrics_utils, resource_mappings, start_time)))
        operational_threads.append(
            AWSMetricsThread(target=update_kinesis_analytics_application_status,
                             args=(aws_metrics_utils, resource_mappings, False)))
        for thread in operational_threads:
            thread.start()
        for thread in operational_threads:
            thread.join()

    @pytest.mark.parametrize('level', ['AWS/Metrics'])
    def test_unauthorized_user_request_rejected(self,
                                                level: str,
                                                launcher: pytest.fixture,
                                                asset_processor: pytest.fixture,
                                                workspace: pytest.fixture):
        """
        Verify that unauthorized users cannot send metrics events to the AWS backed backend.
        """
        log_monitor = setup(launcher, asset_processor)

        # Set invalid AWS credentials.
        launcher.args = ['+LoadLevel', level, '+cl_awsAccessKey', 'AKIAIOSFODNN7EXAMPLE',
                         '+cl_awsSecretKey', 'wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY']
        launcher.args.extend(['-rhi=null'])

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - Failed to send metrics.'],
                unexpected_lines=['(Script) - Metrics is sent successfully.'],
                halt_on_unexpected=True)
            assert result, 'Metrics events are sent successfully by unauthorized user'
            logger.info('Unauthorized user is rejected to send metrics.')

    def test_clean_up_s3_bucket(self,
                                aws_utils: pytest.fixture,
                                resource_mappings: pytest.fixture,
                                aws_metrics_utils: pytest.fixture):
        """
        Clear the analytics bucket objects so that the S3 bucket can be destroyed during tear down.
        """
        aws_metrics_utils.empty_bucket(
            resource_mappings.get_resource_name_id('AWSMetrics.AnalyticsBucketName'))
