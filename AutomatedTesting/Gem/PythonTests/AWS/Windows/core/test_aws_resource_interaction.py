"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import logging
import time

import pytest
import ly_test_tools
import ly_test_tools.log.log_monitor
import ly_test_tools.environment.process_utils as process_utils
import ly_test_tools.o3de.asset_processor_utils as asset_processor_utils

from botocore.exceptions import ClientError
from AWS.Windows.resource_mappings.resource_mappings import resource_mappings
from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor

AWS_CORE_FEATURE_NAME = 'AWSCore'
AWS_RESOURCE_MAPPING_FILE_NAME = 'default_aws_resource_mappings.json'

process_utils.kill_processes_named("o3de", ignore_extensions=True)  # Kill ProjectManager windows

GAME_LOG_NAME = 'Game.log'

logger = logging.getLogger(__name__)


def setup(launcher: pytest.fixture, cdk: pytest.fixture, resource_mappings: pytest.fixture, asset_processor: pytest.fixture):
    asset_processor_utils.kill_asset_processor()
    logger.info(f'Cdk stack names:\n{cdk.list()}')
    stacks = cdk.deploy(additonal_params=['--all'])
    resource_mappings.populate_output_keys(stacks)
    asset_processor.start()
    asset_processor.wait_for_idle()

    file_to_monitor = os.path.join(launcher.workspace.paths.project_log(), GAME_LOG_NAME)
    log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

    return log_monitor


@pytest.mark.SUITE_periodic
@pytest.mark.usefixtures('automatic_process_killer')
@pytest.mark.usefixtures('asset_processor')
@pytest.mark.usefixtures('cdk')
@pytest.mark.parametrize('feature_name', [AWS_CORE_FEATURE_NAME])
@pytest.mark.parametrize('region_name', ['us-west-2'])
@pytest.mark.parametrize('assume_role_arn', ['arn:aws:iam::645075835648:role/o3de-automation-tests'])
@pytest.mark.parametrize('session_name', ['o3de-Automation-session'])
@pytest.mark.usefixtures('workspace')
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['AWS/Core'])
@pytest.mark.usefixtures('resource_mappings')
@pytest.mark.parametrize('resource_mappings_filename', [AWS_RESOURCE_MAPPING_FILE_NAME])
@pytest.mark.usefixtures('aws_credentials')
@pytest.mark.parametrize('profile_name', ['AWSAutomationTest'])
class TestAWSCoreAWSResourceInteraction(object):
    """
    Test class to verify AWSCore can downloading a file from S3.
    """
    def test_download_from_s3(self,
                              level: str,
                              launcher: pytest.fixture,
                              cdk: pytest.fixture,
                              workspace: pytest.fixture,
                              asset_processor: pytest.fixture,
                              resource_mappings: pytest.fixture
                              ):
        """
        Setup: Deploys cdk and updates resource mapping file.
        Tests: Getting AWS credentials for no signed in user.
        Verification: Log monitor looks for success download. The existence and contents of the file are also verified.
        """

        log_monitor = setup(launcher, cdk, resource_mappings, asset_processor)

        launcher.args = ['+LoadLevel', level]
        launcher.args.extend(['-rhi=null'])

        user_dir = os.path.join(workspace.paths.project(), 'user')
        download_dir = os.path.join(user_dir, 's3_download')
        if not os.path.exists(download_dir):
            os.makedirs(download_dir)

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - [S3] Head object request is done',
                                '(Script) - [S3] Head object success: Object example.txt is found.',
                                '(Script) - [S3] Get object success: Object example.txt is downloaded.'],
                unexpected_lines=['(Script) - [S3] Head object error: No response body.',
                                  '(Script) - [S3] Get object error: Request validation failed, output file directory doesn\'t exist.'],
                halt_on_unexpected=True
                )

            assert result, "Expected lines weren't found."

        download_path = os.path.join(download_dir, 'output.txt')

        file_was_downloaded = os.path.exists(download_path)
        # clean up the file directories.
        if file_was_downloaded:
            os.remove(download_path)
        os.rmdir(download_dir)

        assert file_was_downloaded, 'The expected file wasn\'t successfully downloaded'

    def test_invoke_lambda(self,
                           level: str,
                           launcher: pytest.fixture,
                           cdk: pytest.fixture,
                           resource_mappings: pytest.fixture,
                           workspace: pytest.fixture,
                           asset_processor: pytest.fixture
                           ):
        """
        Setup: Deploys the CDK.
        Tests: Runs the test level.
        Verification: Searches the logs for the expected output from the example lambda.
        """

        log_monitor = setup(launcher, cdk, resource_mappings, asset_processor)

        launcher.args = ['+LoadLevel', level]
        launcher.args.extend(['-rhi=null'])

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - [Lambda] Completed Invoke',
                                '(Script) - [Lambda] Invoke success: {"statusCode": 200, "body": {}}'],
                unexpected_lines=['(Script) - Request validation failed, output file miss full path.',
                                  '(Script) - '],
                halt_on_unexpected=True
            )

        assert result

    def test_get_dynamodb_value(self,
                                level: str,
                                launcher: pytest.fixture,
                                cdk: pytest.fixture,
                                resource_mappings: pytest.fixture,
                                workspace: pytest.fixture,
                                asset_processor: pytest.fixture,
                                aws_utils: pytest.fixture,
                                ):
        """
        Setup: Deploys  the CDK application
        Test: Runs a launcher with a level that loads a scriptcanvas that pulls a DynamoDB table value.
        Verification: The value is output in the logs and verified by the test.
        """

        def write_test_table_data():
            client = aws_utils.client('dynamodb')
            table_name = resource_mappings.get_resource_name_id("AWSCore.ExampleDynamoTableOutput")
            try:
                client.put_item(
                    TableName=table_name,
                    Item={
                        'id': {
                            'S': 'Item1'
                        }
                    }
                )
                logger.info(f'Loaded data into table {table_name}')
            except ClientError:
                logger.exception(f'Failed to load data into table {table_name}')
                raise

        log_monitor = setup(launcher, cdk, resource_mappings, asset_processor)
        write_test_table_data()

        launcher.args = ['+LoadLevel', level]
        launcher.args.extend(['-rhi=null'])

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - [DynamoDB] Results finished'],
                unexpected_lines=['(Script) - Request validation failed, output file miss full path.',
                                  '(Script) - '],
                halt_on_unexpected=True
            )

        assert result
