import pytest
import ly_test_tools
import ly_test_tools.log.log_monitor
import ly_test_tools.environment.process_utils as process_utils
import os
import logging
import json
import time
from typing import List

import boto3
from boto3.dynamodb.table import TableResource
from botocore.exceptions import ClientError
from AWS.Windows.resource_mappings.resource_mappings import resource_mappings
from AWS.Windows.cdk.cdk import cdk
from AWS.common.aws_utils import aws_utils
from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor as asset_processor

AWS_PROJECT_NAME = 'AWS-AutomationTest'
AWS_CORE_FEATURE_NAME = 'AWSCore'
AWS_CORE_DEFAULT_PROFILE_NAME = 'default'
AWS_RESOURCE_MAPPING_FILE_NAME = 'aws_resource_mappings.json'

AP_PROCESSES = [
            'AssetProcessor', 'AssetProcessorBatch', 'AssetBuilder', 'CrySCompileServer',
            'rc'  # Resource Compiler
        ]

process_utils.kill_processes_named("o3de", ignore_extensions=True)  # Kill ProjectManager windows

GAME_LOG_NAME = 'Game.log'

logger = logging.getLogger(__name__)


def kill_asset_processor(targets):
    for target in targets:
        process_utils.kill_processes_named(target, ignore_extensions=True)  # Kill AssetProcessor


def write_test_table_data(region: str):
    """
    Write a list of items to a DynamoDB table using the batch_writer.

    Each item must contain at least the keys required by the schema, which for the example
    is just 'id'
    """
    session = boto3.Session(region_name=region)

    aws_resource_directory = os.path.join("../../../../../Config", AWS_RESOURCE_MAPPING_FILE_NAME)
    with open(aws_resource_directory, "r") as aws_resource:
        aws_resource_data = json.load(aws_resource)

    table_name = aws_resource_data["AWSResourceMappings"]["AWSCore.ExampleDynamoTableOutput"]["Name/ID"]

    dynamodb = session.resource('dynamodb')
    table = dynamodb.Table(table_name)

    try:
        with table.batch_writer() as writer:
            item = {
                'id': 'Item1',
                'value': 123,
                'timestamp': int(time.time())
            }
            writer.put_item(Item=item)
        logger.info(f'Loaded data into table {table.name}')
    except ClientError:
        logger.exception(f'Failed to load data into table {table.name}')
        raise


@pytest.mark.SUITE_periodic
@pytest.mark.usefixtures('automatic_process_killer')
@pytest.mark.usefixtures('asset_processor')
@pytest.mark.usefixtures('cdk')
@pytest.mark.parametrize('feature_name', [AWS_CORE_FEATURE_NAME])
@pytest.mark.usefixtures('aws_utils')
@pytest.mark.parametrize('region_name', ['us-west-2'])
@pytest.mark.parametrize('assume_role_arn', ['arn:aws:iam::645075835648:role/o3de-automation-tests'])
@pytest.mark.parametrize('session_name', ['o3de-Automation-session'])
@pytest.mark.usefixtures('workspace')
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['AWS/Core'])
@pytest.mark.usefixtures('resource_mappings')
@pytest.mark.parametrize('resource_mappings_filename', ['aws_resource_mappings.json'])
class TestAWSCoreDownloadFromS3(object):
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

        kill_asset_processor(AP_PROCESSES)
        logger.info(f'Cdk stack names:\n{cdk.list()}')
        stacks = cdk.deploy(additonal_params=['--all'])
        resource_mappings.populate_output_keys(stacks)
        asset_processor.start()
        asset_processor.wait_for_idle()

        file_to_monitor = os.path.join(launcher.workspace.paths.project_log(), GAME_LOG_NAME)
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

        launcher.args = ['+LoadLevel', level]

        if not os.path.exists('s3download'):
            os.makedirs('s3download')

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - [S3] Head object request is done',
                                '(Script) - [S3] Head object success: Object example.txt is found.',
                                '(Script) - [S3] Get object request is done',
                                '(Script) - [S3] Head object success: Object example.txt is found.',
                                '(Script) - [S3] Get object success: Object example.txt is downloaded.'],
                unexpected_lines=['(Script) - [S3] Head object error: No response body.',
                                  '(Script) - [S3] Get object error: Request validation failed, output file directory doesn\'t exist.'],
                halt_on_unexpected=True,
                )

            assert result, ''

        download_dir = os.path.join(os.getcwd(), 's3download/output.txt')

        file_was_downloaded = os.path.exists(download_dir)
        # clean up the file directories.
        if file_was_downloaded:
            os.remove('s3download/output.txt')
        os.rmdir('./s3download')

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
        kill_asset_processor(AP_PROCESSES)
        logger.info(f'Cdk stack names:\n{cdk.list()}')
        stacks = cdk.deploy(additonal_params=['--all'])
        resource_mappings.populate_output_keys(stacks)
        asset_processor.start()
        asset_processor.wait_for_idle()

        project_log = launcher.workspace.paths.project_log()
        file_to_monitor = os.path.join(project_log, GAME_LOG_NAME)
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

        launcher.args = ['+LoadLevel', level]

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - [Lambda] Completed Invoke',
                                '(Script) - [Lambda] Invoke success: {"statusCode": 200, "body": {}}'],
                unexpected_lines=['(Script) - Request validation failed, output file miss full path.',
                                  '(Script) - '],
                halt_on_unexpected=True,
            )

        assert result

    def test_get_dynamodb_value(self,
                                level: str,
                                launcher: pytest.fixture,
                                cdk: pytest.fixture,
                                resource_mappings: pytest.fixture,
                                workspace,
                                asset_processor
                                ):
        """
        Setup: Deploys  the CDK application
        Test: Runs a launcher with a level that loads a scriptcanvas that pulls a DynamoDB table value.
        Verification: The value is output in the logs and verified by the test.
        """
        kill_asset_processor(AP_PROCESSES)
        logger.info(f'Cdk stack names:\n{cdk.list()}')
        stacks = cdk.deploy(additonal_params=['--all'])
        resource_mappings.populate_output_keys(stacks)
        write_test_table_data("us-west-2")
        asset_processor.start()
        asset_processor.wait_for_idle()

        project_log = launcher.workspace.paths.project_log()
        file_to_monitor = os.path.join(project_log, GAME_LOG_NAME)
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

        launcher.args = ['+LoadLevel', level]

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - [DynamoDB] Results finished'],
                unexpected_lines=['(Script) - Request validation failed, output file miss full path.',
                                  '(Script) - '],
                halt_on_unexpected=True,
            )

        assert result
