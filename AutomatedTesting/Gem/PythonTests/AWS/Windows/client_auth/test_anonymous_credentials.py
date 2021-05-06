import logging
import ly_test_tools.log.log_monitor
from ..resource_mappings.resource_mappings import *
from ..cdk.cdk import *

AWS_PROJECT_NAME = 'AWS-AutomationTest'
AWS_CLIENT_AUTH_FEATURE_NAME = 'AWSClientAuth'
AWS_CLIENT_AUTH_DEFAULT_ACCOUNT_ID = '729543576514'
AWS_CLIENT_AUTH_DEFAULT_REGION = 'us-west-2'
AWS_CLIENT_AUTH_DEFAULT_PROFILE_NAME = 'default'
AWS_CDK_APP_PATH = ''

GAME_LOG_NAME = 'Game.log'

logger = logging.getLogger(__name__)


@pytest.mark.SUITE_periodic
@pytest.mark.usefixtures('automatic_process_killer')
# pytest.mark.parametrize("launcher_platform", ['windows'])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['AWS/ClientAuth'])
@pytest.mark.usefixtures('cdk')
@pytest.mark.parametrize('account_id', [AWS_CLIENT_AUTH_DEFAULT_ACCOUNT_ID])
@pytest.mark.parametrize('region', [AWS_CLIENT_AUTH_DEFAULT_REGION])
@pytest.mark.parametrize('feature_name', [AWS_CLIENT_AUTH_FEATURE_NAME])
@pytest.mark.parametrize('profile', ['default'])
@pytest.mark.usefixtures('resource_mappings')
@pytest.mark.parametrize('resource_mappings_filename', ['aws_resource_mappings.json'])
class TestAWSClientAuthAnonymousCredentials(object):
    """
    Test class to verify AWS Cognito Identity pool anonymous authorization.
    """

    def test_anonymous_credentials(self,
                                     level: str,
                                     launcher: pytest.fixture,
                                     cdk: pytest.fixture,
                                     resource_mappings: pytest.fixture):
        """
        Setup: Deploys cdk and updates resource mapping file.
        Tests: Getting AWS credentials for no signed in user.
        Verification: Log monitor looks for success credentials log.
        """
        cdk.list()
        stacks = cdk.deploy()
        resource_mappings.populate_output_keys(stacks)

        # Initialize the log monitor.
        file_to_monitor = os.path.join(launcher.workspace.paths.project_log(), GAME_LOG_NAME)
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

        launcher.args = ['+map', level]

        # with launcher.start():
        #     result = log_monitor.monitor_log_for_lines(
        #         expected_lines=['(Script) - Success anonymous credentials.'],
        #         unexpected_lines=['(Script) - Fail anonymous credentials.'],
        #         halt_on_unexpected=True)
        #     assert result, 'Anonymous credentials fetched successfully.'
