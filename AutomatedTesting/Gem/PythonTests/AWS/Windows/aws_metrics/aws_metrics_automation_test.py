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

# fixture imports
from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor
from .aws_metrics_utils import aws_metrics_utils
from .aws_metrics_custom_thread import AWSMetricsThread

AWS_METRICS_FEATURE_NAME = 'AWSMetrics'
GAME_LOG_NAME = 'Game.log'
CONTEXT_VARIABLE = ['-c', 'batch_processing=true']

logger = logging.getLogger(__name__)


def setup(launcher: pytest.fixture,
          asset_processor: pytest.fixture) -> pytest.fixture:
    """
    Set up the resource mapping configuration and start the log monitor.
    :param launcher: Client launcher for running the test level.
    :param asset_processor: asset_processor fixture.
    :return log monitor object, metrics file path and the metrics stack name.
    """
    asset_processor.start()
    asset_processor.wait_for_idle()

    file_to_monitor = os.path.join(launcher.workspace.paths.project_log(), GAME_LOG_NAME)

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


def query_metrics_from_s3(aws_metrics_utils: pytest.fixture, stack_name: str) -> None:
    """
    Verify that the metrics events are delivered to the S3 bucket and can be queried.
    aws_metrics_utils: aws_metrics_utils fixture.
    stack_name: name of the CloudFormation stack.
    """
    analytics_bucket_name = aws_metrics_utils.get_analytics_bucket_name(stack_name)
    aws_metrics_utils.verify_s3_delivery(analytics_bucket_name)
    logger.info('Metrics are sent to S3.')

    aws_metrics_utils.run_glue_crawler(f'{stack_name}-EventsCrawler')
    aws_metrics_utils.run_named_queries(f'{stack_name}-AthenaWorkGroup')
    logger.info('Query metrics from S3 successfully.')

    # Empty the S3 bucket. S3 buckets can only be deleted successfully when it doesn't contain any object.
    aws_metrics_utils.empty_batch_analytics_bucket(analytics_bucket_name)


def verify_operational_metrics(aws_metrics_utils: pytest.fixture, stack_name: str, start_time: datetime) -> None:
    """
    Verify that operational health metrics are delivered to CloudWatch.
    aws_metrics_utils: aws_metrics_utils fixture.
    stack_name: name of the CloudFormation stack.
    start_time: Time when the game launcher starts.
    """
    aws_metrics_utils.verify_cloud_watch_delivery(
        'AWS/Lambda',
        'Invocations',
        [{'Name': 'FunctionName',
          'Value': f'{stack_name}-AnalyticsProcessingLambdaName'}],
        start_time)
    logger.info('AnalyticsProcessingLambda metrics are sent to CloudWatch.')

    aws_metrics_utils.verify_cloud_watch_delivery(
        'AWS/Lambda',
        'Invocations',
        [{'Name': 'FunctionName',
          'Value': f'{stack_name}-EventsProcessingLambda'}],
        start_time)
    logger.info('EventsProcessingLambda metrics are sent to CloudWatch.')


def start_kinesis_analytics_application(aws_metrics_utils: pytest.fixture, stack_name: str) -> None:
    """
    Start the Kinesis analytics application for real-time analytics.
    aws_metrics_utils: aws_metrics_utils fixture.
    stack_name: name of the CloudFormation stack.
    """
    analytics_application_name = f'{stack_name}-AnalyticsApplication'
    aws_metrics_utils.start_kinesis_data_analytics_application(analytics_application_name)

@pytest.mark.SUITE_periodic
@pytest.mark.usefixtures('automatic_process_killer')
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['AWS/Metrics'])
@pytest.mark.parametrize('feature_name', [AWS_METRICS_FEATURE_NAME])
@pytest.mark.usefixtures('resource_mappings')
@pytest.mark.parametrize('resource_mappings_filename', ['default_aws_resource_mappings.json'])
@pytest.mark.usefixtures('aws_credentials')
@pytest.mark.parametrize('profile_name', ['AWSAutomationTest'])
@pytest.mark.parametrize('region_name', ['us-west-2'])
@pytest.mark.parametrize('assume_role_arn', ['arn:aws:iam::645075835648:role/o3de-automation-tests'])
@pytest.mark.usefixtures('cdk')
@pytest.mark.parametrize('session_name', ['o3de-Automation-session'])
@pytest.mark.parametrize('deployment_params', [CONTEXT_VARIABLE])
class TestAWSMetricsWindows(object):
    """
    Test class to verify the real-time and batch analytics for metrics.
    """

    @pytest.mark.parametrize('destroy_stacks_on_teardown', [False])
    def test_realtime_and_batch_analytics(self,
                                          level: str,
                                          launcher: pytest.fixture,
                                          asset_processor: pytest.fixture,
                                          workspace: pytest.fixture,
                                          aws_utils: pytest.fixture,
                                          cdk: pytest.fixture,
                                          aws_metrics_utils: pytest.fixture):
        """
        Verify that the metrics events are sent to CloudWatch and S3 for analytics.
        """
        # Start Kinesis analytics application on a separate thread to avoid blocking the test.
        kinesis_analytics_application_thread = AWSMetricsThread(target=start_kinesis_analytics_application,
                                                                args=(aws_metrics_utils, cdk.stacks[0]))
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

        # Run time-consuming verifications on separate threads to avoid blocking the test.
        verification_threads = list()
        verification_threads.append(
            AWSMetricsThread(target=query_metrics_from_s3, args=(aws_metrics_utils, cdk.stacks[0])))
        verification_threads.append(
            AWSMetricsThread(target=verify_operational_metrics, args=(aws_metrics_utils, cdk.stacks[0], start_time)))
        for thread in verification_threads:
            thread.start()
        for thread in verification_threads:
            thread.join()

    @pytest.mark.parametrize('destroy_stacks_on_teardown', [True])
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
