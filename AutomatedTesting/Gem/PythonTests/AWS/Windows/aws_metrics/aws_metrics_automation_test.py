"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import os
import pytest
import time
import typing

from datetime import datetime
import ly_test_tools.log.log_monitor

# fixture imports
from AWS.Windows.resource_mappings.resource_mappings import resource_mappings
from AWS.Windows.cdk.cdk_utils import Cdk
from AWS.common.aws_utils import AwsUtils
from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor as asset_processor
from AWS.common.aws_credentials import aws_credentials
from .aws_metrics_utils import aws_metrics_utils

AWS_METRICS_FEATURE_NAME = 'AWSMetrics'
GAME_LOG_NAME = 'Game.log'

logger = logging.getLogger(__name__)


def setup(launcher: ly_test_tools.launchers.Launcher,
          cdk: Cdk,
          asset_processor: asset_processor,
          resource_mappings: resource_mappings,
          context_variable: str = '') -> typing.Tuple[ly_test_tools.log.log_monitor.LogMonitor, str, str]:
    """
    Set up the CDK application and start the log monitor.
    :param launcher: Client launcher for running the test level.
    :param cdk: CDK application for deploying the AWS resources.
    :param asset_processor: asset_processor fixture.
    :param resource_mappings: resource_mappings fixture.
    :param context_variable: context_variable for enable optional CDK feature.
    :return log monitor object, metrics file path and the metrics stack name.
    """
    logger.info(f'Cdk stack names:\n{cdk.list()}')
    stacks = cdk.deploy(context_variable=context_variable)
    resource_mappings.populate_output_keys(stacks)

    asset_processor.start()
    asset_processor.wait_for_idle()

    metrics_file_path = os.path.join(launcher.workspace.paths.project(), 'user',
                                     AWS_METRICS_FEATURE_NAME, 'metrics.json')
    remove_file(metrics_file_path)

    file_to_monitor = os.path.join(launcher.workspace.paths.project_log(), GAME_LOG_NAME)
    remove_file(file_to_monitor)

    # Initialize the log monitor.
    log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

    return log_monitor, metrics_file_path, stacks[0]


def monitor_metrics_submission(log_monitor: ly_test_tools.log.log_monitor.LogMonitor) -> None:
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


def remove_file(file_path: str) -> None:
    """
    Remove a local file and its directory.
    :param file_path: Path to the local file.
    """
    if os.path.exists(file_path):
        os.remove(file_path)

    file_dir = os.path.dirname(file_path)
    if os.path.exists(file_dir) and len(os.listdir(file_dir)) == 0:
        os.rmdir(file_dir)


@pytest.mark.SUITE_periodic
@pytest.mark.usefixtures('automatic_process_killer')
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['AWS/Metrics'])
@pytest.mark.parametrize('feature_name', [AWS_METRICS_FEATURE_NAME])
@pytest.mark.parametrize('resource_mappings_filename', ['default_aws_resource_mappings.json'])
@pytest.mark.parametrize('profile_name', ['AWSAutomationTest'])
@pytest.mark.parametrize('region_name', ['us-west-2'])
@pytest.mark.parametrize('assume_role_arn', ['arn:aws:iam::645075835648:role/o3de-automation-tests'])
@pytest.mark.parametrize('session_name', ['o3de-Automation-session'])
class TestAWSMetricsWindows(object):
    def test_realtime_analytics_metrics_sent_to_cloudwatch(self,
                                                           level: str,
                                                           launcher: ly_test_tools.launchers.Launcher,
                                                           asset_processor: pytest.fixture,
                                                           workspace: pytest.fixture,
                                                           aws_utils: pytest.fixture,
                                                           aws_credentials: aws_credentials,
                                                           resource_mappings: pytest.fixture,
                                                           cdk: pytest.fixture,
                                                           aws_metrics_utils: aws_metrics_utils,
                                                           ):
        """
        Tests that the submitted metrics are sent to CloudWatch for real-time analytics.
        """
        log_monitor, metrics_file_path, stack_name = setup(launcher, cdk, asset_processor, resource_mappings)

        # Start the Kinesis Data Analytics application for real-time analytics.
        analytics_application_name = f'{stack_name}-AnalyticsApplication'
        aws_metrics_utils.start_kinesis_data_analytics_application(analytics_application_name)

        launcher.args = ['+LoadLevel', level]
        launcher.args.extend(['-rhi=null'])

        with launcher.start(launch_ap=False):
            start_time = datetime.utcnow()
            monitor_metrics_submission(log_monitor)
            # Verify that operational health metrics are delivered to CloudWatch.
            aws_metrics_utils.verify_cloud_watch_delivery(
                'AWS/Lambda',
                'Invocations',
                [{'Name': 'FunctionName',
                  'Value': f'{stack_name}-AnalyticsProcessingLambdaName'}],
                start_time)
            logger.info('Operational health metrics sent to CloudWatch.')

            aws_metrics_utils.verify_cloud_watch_delivery(
                AWS_METRICS_FEATURE_NAME,
                'TotalLogins',
                [],
                start_time)
            logger.info('Real-time metrics sent to CloudWatch.')

        # Stop the Kinesis Data Analytics application.
        aws_metrics_utils.stop_kinesis_data_analytics_application(analytics_application_name)

    def test_unauthorized_user_request_rejected(self,
                                                level: str,
                                                launcher: ly_test_tools.launchers.Launcher,
                                                cdk: pytest.fixture,
                                                aws_credentials: aws_credentials,
                                                asset_processor: pytest.fixture,
                                                resource_mappings: pytest.fixture,
                                                workspace: pytest.fixture):
        """
        Tests that unauthorized users cannot send metrics events to the AWS backed backend.
        """
        log_monitor, metrics_file_path, stack_name = setup(launcher, cdk, asset_processor, resource_mappings)
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

    def test_batch_analytics_metrics_delivered_to_s3(self,
                                                            level: str,
                                                            launcher: ly_test_tools.launchers.Launcher,
                                                            cdk: pytest.fixture,
                                                            aws_credentials: aws_credentials,
                                                            asset_processor: pytest.fixture,
                                                            resource_mappings: pytest.fixture,
                                                            aws_utils: pytest.fixture,
                                                            aws_metrics_utils: aws_metrics_utils,
                                                            workspace: pytest.fixture):
        """
        Tests that the submitted metrics are sent to the data lake for batch analytics.
        """
        log_monitor, metrics_file_path, stack_name = setup(launcher, cdk, asset_processor, resource_mappings,
                                                           context_variable='batch_processing=true')

        analytics_bucket_name = aws_metrics_utils.get_analytics_bucket_name(stack_name)

        launcher.args = ['+LoadLevel', level]
        launcher.args.extend(['-rhi=null'])

        with launcher.start(launch_ap=False):
            start_time = datetime.utcnow()
            monitor_metrics_submission(log_monitor)
            # Verify that operational health metrics are delivered to CloudWatch.
            aws_metrics_utils.verify_cloud_watch_delivery(
                'AWS/Lambda',
                'Invocations',
                [{'Name': 'FunctionName',
                  'Value': f'{stack_name}-EventsProcessingLambda'}],
                start_time)
            logger.info('Operational health metrics sent to CloudWatch.')

        aws_metrics_utils.verify_s3_delivery(analytics_bucket_name)
        logger.info('Metrics sent to S3.')

        # Run the glue crawler to populate the AWS Glue Data Catalog with tables.
        aws_metrics_utils.run_glue_crawler(f'{stack_name}-EventsCrawler')
        # Run named queries on the table to verify the batch analytics.
        aws_metrics_utils.run_named_queries(f'{stack_name}-AthenaWorkGroup')
        logger.info('Query metrics from S3 successfully.')

        # Kinesis Data Firehose buffers incoming data before it delivers it to Amazon S3. Sleep for the
        # default interval (60s) to make sure that all the metrics are sent to the bucket before cleanup.
        time.sleep(60)
        # Empty the S3 bucket. S3 buckets can only be deleted successfully when it doesn't contain any object.
        aws_metrics_utils.empty_s3_bucket(analytics_bucket_name)
