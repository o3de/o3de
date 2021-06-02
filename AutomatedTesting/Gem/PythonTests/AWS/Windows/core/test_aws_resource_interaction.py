import pytest
import ly_test_tools
import ly_test_tools.log.log_monitor
import os
import logging

from AWS.Windows.resource_mappings.resource_mappings import resource_mappings
from AWS.Windows.cdk.cdk import cdk
from AWS.common.aws_utils import aws_utils
from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor as asset_processor

AWS_PROJECT_NAME = 'AWS-AutomationTest'
AWS_CORE_FEATURE_NAME = 'AWSCore'
AWS_CORE_DEFAULT_PROFILE_NAME = 'default'


GAME_LOG_NAME = 'Game.log'

logger = logging.getLogger(__name__)


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
@pytest.fixture()
def setup_environment(request, cdk: pytest.fixture, workspace: pytest.fixture, resource_mappings: pytest.fixture, asset_processor: pytest.fixture):
    """
    Sets up the CDK before tests are run. Runs the Asset Processor.
    """

    logger.info(f'Cdk stack names:\n{cdk.list()}')
    stacks = cdk.deploy(additonal_params=['--all'])
    resource_mappings.populate_output_keys(stacks)
    asset_processor.start()
    asset_processor.wait_for_idle()

    def teardown():
        asset_processor.stop()

    request.addfinalizer(teardown)

    return stacks


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
# @pytest.mark.usefixtures('setup_environment')
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

        logger.info(f'Cdk stack names:\n{cdk.list()}')
        stacks = cdk.deploy(additonal_params=['--all'])
        resource_mappings.populate_output_keys(stacks)
        asset_processor.start()
        asset_processor.wait_for_idle()

        file_to_monitor = os.path.join(launcher.workspace.paths.project_log(), GAME_LOG_NAME)
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

        launcher.args = ['+LoadLevel', level]

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - head object request is done',
                                '(Script) - get object request is done',
                                '(Script) - Object example.txt is found.',
                                '(Script) - Object example.txt is downloaded.'],
                unexpected_lines=['(Script) - Request validation failed, output file miss full path.',
                                  '(Script) - '],
                halt_on_unexpected=True,
                )

            assert result, ''

        download_dir = os.path.join(os.getcwd(), 's3download/output.txt')

        assert os.path.exists(download_dir), 'The expected file wasn\'t successfully downloaded'

    def test_download_from_s3_raw(self,
                                  level: str,
                                  launcher: pytest.fixture,
                                  cdk: pytest.fixture,
                                  resource_mappings: pytest.fixture,
                                  workspace: pytest.fixture,
                                  asset_processor: pytest.fixture
                                  ):

        assert True

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

        project_log = launcher.workspace.paths.project_log()
        file_to_monitor = os.path.join(project_log, GAME_LOG_NAME)
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

        launcher.args = ['+LoadLevel', level]

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - No response body.'
                                '(Script) - Success: {"statusCode": 200, "body": event}'],
                unexpected_lines=['(Script) - Request validation failed, output file miss full path.',
                                  '(Script) - '],
                halt_on_unexpected=True,
            )

        assert result

    def test_invoke_lambda_raw(self,
                               level: str,
                               launcher: pytest.fixture,
                               cdk: pytest.fixture,
                               resource_mappings: pytest.fixture,
                               workspace: pytest.fixture,
                               asset_processor: pytest.fixture
                               ):
        assert False

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

        project_log = launcher.workspace.paths.project_log()
        file_to_monitor = os.path.join(project_log, GAME_LOG_NAME)
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

        launcher.args = ['+LoadLevel', level]

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - id: {"S":	"It Worked"}'],
                unexpected_lines=['(Script) - Request validation failed, output file miss full path.',
                                  '(Script) - '],
                halt_on_unexpected=True,
            )

        assert result
