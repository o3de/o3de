"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

Preconditions:
1) Enable the AWSCore and AWSMetrics gem for the AutomatedTesting project
2) Define AWS_METRICS_DEFAULT_ACCOUNT_ID using a valid AWS account ID with admin permissions
"""

import logging
import os
import time
import typing
import pytest

from datetime import datetime
import ly_test_tools.log.log_monitor

from common.aws_configuration import aws_configuration
from common.cdk_application import cdk_application
from common.remote_console import remote_console
from .aws_metrics_utils import AWSMetricsUtils, AWS_METRICS_DEFAULT_REGION

AWS_PROJECT_NAME = 'AWS-TEST'
AWS_METRICS_FEATURE_NAME = 'AWSMetrics'

AWS_METRICS_DEFAULT_PROFILE_NAME = 'default'
AWS_METRICS_DEFAULT_ACCOUNT_ID = ''

GAME_LOG_NAME = 'Game.log'
# Flush period for the metrics queue
QUEUE_FLUSH_PERIOD_IN_SECONDS = 60

logger = logging.getLogger(__name__)
aws_metrics_utils = AWSMetricsUtils()


def setup(launcher: ly_test_tools.launchers.Launcher,
          cdk_application: cdk_application = None,
          resource_mapping_file_path: str = None) -> typing.Tuple[ly_test_tools.log.log_monitor.LogMonitor, str]:
    """
    Set up the CDK application and start the log monitor.
    :param launcher: Client launcher for running the test level.
    :param cdk_application: CDK application for deploying the AWS resources.
    :param resource_mapping_file_path: Path to the resource Mapping file.
    """
    if cdk_application:
        cdk_application_path = os.path.join(launcher.workspace.paths.dev(), 'Gems', AWS_METRICS_FEATURE_NAME, 'cdk')
        cdk_application.deploy(cdk_application_path)

    if resource_mapping_file_path:
        aws_metrics_utils.export_rest_api_id(cdk_application.stack_name, resource_mapping_file_path)

    metrics_file_path = os.path.join(launcher.workspace.paths.project(), 'user',
                                     AWS_METRICS_FEATURE_NAME, 'metrics.json')
    aws_metrics_utils.remove_file(metrics_file_path)

    file_to_monitor = os.path.join(launcher.workspace.paths.project_log(), GAME_LOG_NAME)
    aws_metrics_utils.remove_file(file_to_monitor)

    # Initialize the log monitor.
    log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

    return log_monitor, metrics_file_path


def monitor_metrics_submission(log_monitor: ly_test_tools.log.log_monitor.LogMonitor):
    """
    Monitor the messages and notifications for submitting metrics.
    :param log_monitor: Log monitor to check the log messages.
    """
    expected_lines = [
        '(Script) - Submitted metrics without buffer.',
        '(Script) - Submitted metrics with buffer.',
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


def verify_offline_recording(metrics_file_path: str, remote_console: remote_console):
    """
    Verify that metrics can be stored to a local file and resubmitted when users disable offline recording.
    :param metrics_file_path: Path to the local metrics file.
    :param remote_console: Remote console instance to send console commands.
    """
    assert not os.path.exists(metrics_file_path), 'Local metrics file exists before offline recording is enabled.'

    remote_console.send_command('AWSMetricsSystemComponent.EnableOfflineRecording true')
    time.sleep(QUEUE_FLUSH_PERIOD_IN_SECONDS)
    assert os.path.exists(metrics_file_path), 'No metrics events are recorded locally.'

    remote_console.send_command('AWSMetricsSystemComponent.EnableOfflineRecording false submit')
    time.sleep(QUEUE_FLUSH_PERIOD_IN_SECONDS)
    assert not os.path.exists(metrics_file_path), \
        'The local metrics file is not deleted ' \
        'after disabling offline recording and resubmitting the local metrics events.'


@pytest.mark.usefixtures('automatic_process_killer')
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['AWSMetrics'])
@pytest.mark.usefixtures('cdk_application')
@pytest.mark.parametrize('aws_project_name', [AWS_PROJECT_NAME], scope='class')
@pytest.mark.parametrize('account_id', [AWS_METRICS_DEFAULT_ACCOUNT_ID], scope='class')
@pytest.mark.parametrize('region', [AWS_METRICS_DEFAULT_REGION], scope='class')
class TestAWSMetrics_Windows(object):
    @pytest.mark.parametrize('profile_name', [AWS_METRICS_DEFAULT_PROFILE_NAME])
    def test_AWSMetrics_RealTimeAnalytics_MetricsSentToCloudWatch(self,
                                                                  level: str,
                                                                  launcher: ly_test_tools.launchers.Launcher,
                                                                  aws_configuration: aws_configuration,
                                                                  cdk_application: cdk_application,
                                                                  remote_console: remote_console,
                                                                  workspace: pytest.fixture):
        """
        Tests that the submitted metrics are sent to CloudWatch for real-time analytics.
        """
        log_monitor, metrics_file_path = setup(launcher, cdk_application, aws_configuration.resource_mapping_file)

        # Start the Kinesis Data Analytics application for real-time analytics.
        analytics_application_name = f'{cdk_application.stack_name}-AnalyticsApplication'
        aws_metrics_utils.start_kenisis_data_analytics_application(analytics_application_name)

        with launcher.start():
            start_time = datetime.utcnow()
            monitor_metrics_submission(log_monitor)
            # Verify that operational health metrics are delivered to CloudWatch.
            aws_metrics_utils.verify_cloud_watch_delivery(
                'AWS/Lambda',
                'Invocations',
                [{'Name': 'FunctionName',
                  'Value': f'{cdk_application.stack_name}-AnalyticsProcessingLambda'}],
                start_time)
            aws_metrics_utils.verify_cloud_watch_delivery(
                AWS_METRICS_FEATURE_NAME,
                'TotalLogins',
                [],
                start_time)

        # Stop the Kinesis Data Analytics application.
        aws_metrics_utils.stop_kenisis_data_analytics_application(analytics_application_name)

    @pytest.mark.parametrize('profile_name', [AWS_METRICS_DEFAULT_PROFILE_NAME])
    def test_AWSMetrics_UnauthorizedUser_RequestRejected(self,
                                                         level: str,
                                                         launcher: ly_test_tools.launchers.Launcher,
                                                         aws_configuration: aws_configuration,
                                                         cdk_application: cdk_application,
                                                         workspace: pytest.fixture):
        """
        Tests that unauthorized users cannot send metrics events to the AWS backed backend.
        """
        log_monitor, metrics_file_path = setup(launcher, cdk_application, aws_configuration.resource_mapping_file)
        # Set invalid AWS credentials.
        launcher.args = ['+map', level, '+cl_awsAccessKey', 'AKIAIOSFODNN7EXAMPLE',
                         '+cl_awsSecretKey', 'wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY']

        with launcher.start():
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - Failed to send metrics.'],
                unexpected_lines=['(Script) - Metrics is sent successfully.'],
                halt_on_unexpected=True)
            assert result, 'Metrics events are sent successfully by unauthorized user'

    @pytest.mark.parametrize('profile_name', [AWS_METRICS_DEFAULT_PROFILE_NAME])
    def test_AWSMetrics_BatchAnalytics_MetricsDeliveredToS3(self,
                                                            level: str,
                                                            launcher: ly_test_tools.launchers.Launcher,
                                                            aws_configuration: aws_configuration,
                                                            cdk_application: cdk_application,
                                                            workspace: pytest.fixture):
        """
        Tests that the submitted metrics are sent to the data lake for batch analytics.
        """
        log_monitor, metrics_file_path = setup(launcher, cdk_application, aws_configuration.resource_mapping_file)

        # Enable the optional batch analytics feature.
        cdk_application.deploy(context_variable='batch_processing=true')

        analytics_bucket_name = aws_metrics_utils.get_analytics_bucket_name(cdk_application.stack_name)
        with launcher.start():
            start_time = datetime.utcnow()
            monitor_metrics_submission(log_monitor)
            # Verify that operational health metrics are delivered to CloudWatch.
            aws_metrics_utils.verify_cloud_watch_delivery(
                'AWS/Lambda',
                'Invocations',
                [{'Name': 'FunctionName',
                  'Value': f'{cdk_application.stack_name}-EventsProcessingLambda'}],
                start_time)
        aws_metrics_utils.verify_s3_delivery(analytics_bucket_name)

        # Run the glue crawler to populate the AWS Glue Data Catalog with tables.
        aws_metrics_utils.run_glue_crawler(f'{cdk_application.stack_name}-EventsCrawler')
        # Run named queries on the table to verify the batch analytics.
        aws_metrics_utils.run_named_queries(f'{cdk_application.stack_name}-AthenaWorkGroup')

        # Empty the S3 bucket. S3 buckets can only be deleted successfully when it doesn't contain any object.
        aws_metrics_utils.empty_s3_bucket(analytics_bucket_name)

    @pytest.mark.parametrize('profile_name', [AWS_METRICS_DEFAULT_PROFILE_NAME])
    def test_AWSMetrics_OfflineRecording_MetricsResubmittedAfterDisable(self,
                                                                        level: str,
                                                                        launcher: ly_test_tools.launchers.Launcher,
                                                                        aws_configuration: aws_configuration,
                                                                        cdk_application: cdk_application,
                                                                        remote_console: remote_console,
                                                                        workspace: pytest.fixture):
        """
        Tests that the offline recorded metrics events are resubmitted after offline recording is disabled.
        """
        log_monitor, metrics_file_path = setup(launcher, cdk_application, aws_configuration.resource_mapping_file)

        with launcher.start():
            remote_console.start()
            monitor_metrics_submission(log_monitor)
            verify_offline_recording(metrics_file_path, remote_console)

        aws_metrics_utils.remove_file(metrics_file_path)

    def test_AWSMetrics_Retry_MetricsResubmitted(self,
                                                 level: str,
                                                 launcher: ly_test_tools.launchers.Launcher,
                                                 remote_console: remote_console,
                                                 workspace: pytest.fixture):
        """
        Tests that failed metrics events are retried for submission.
        """
        log_monitor, metrics_file_path = setup(launcher)

        with launcher.start():
            remote_console.start()
            result = remote_console.expect_log_line('(Script) - Failed to send metrics.')
            assert result, 'Metrics events are sent successfully to the backend without valid configuration files.'

            remote_console.send_command('AWSMetricsSystemComponent.EnableOfflineRecording true')
            # The metrics queue will be flushed every minute by default.
            # Wait for the metrics queue to be flushed at least once.
            time.sleep(QUEUE_FLUSH_PERIOD_IN_SECONDS)

            # Verify that failed metrics events are retried and sent successfully.
            remote_console.send_command('AWSMetricsSystemComponent.DumpStats')
            result = remote_console.expect_log_line(
                '(AWSMetrics) - Total number of metrics events failed to be sent to the backend/local file: 0'
            )
            assert result, 'Failed metrics events are not retried and sent successfully.'

        aws_metrics_utils.remove_file(metrics_file_path)

    def test_AWSMetrics_Retry_MetricsEventsDroppedAfterMaxRetry(self,
                                                                level: str,
                                                                launcher: ly_test_tools.launchers.Launcher,
                                                                remote_console: remote_console,
                                                                workspace: pytest.fixture):
        """
        Tests that failed metrics events are dropped after maximum number of retries.
        """
        log_monitor, metrics_file_path = setup(launcher)

        with launcher.start():
            remote_console.start()
            result = remote_console.expect_log_line('(Script) - Failed to send metrics.')
            assert result, 'Metrics events are sent successfully to the backend without valid configuration files.'
            # The metrics queue will be flushed every minute by default.
            # Wait for the metrics queue to be flushed at least once.
            time.sleep(QUEUE_FLUSH_PERIOD_IN_SECONDS)

            # Verify that failed metrics events are dropped after retry.
            remote_console.send_command('AWSMetricsSystemComponent.DumpStats')
            result = log_monitor.monitor_log_for_lines(
                expected_lines=[],
                unexpected_lines=[
                    '(AWSMetrics) - Total number of metrics events failed to be sent to the backend/local file: 0',
                    '(AWSMetrics) - Total number of metrics events which failed the JSON schema validation'
                    ' or reached the maximum number of retries : 0'
                ],
                halt_on_unexpected=True)

            assert result, (
                f'Metrics events were not dropped on unsuccessful retries')

        aws_metrics_utils.remove_file(metrics_file_path)
