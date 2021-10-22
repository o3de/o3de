"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import logging
import os
import pytest

import ly_test_tools.log.log_monitor

from AWS.common import constants

# fixture imports
from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor

AWS_CLIENT_AUTH_FEATURE_NAME = 'AWSClientAuth'

logger = logging.getLogger(__name__)


@pytest.mark.SUITE_awsi
@pytest.mark.usefixtures('asset_processor')
@pytest.mark.usefixtures('automatic_process_killer')
@pytest.mark.usefixtures('aws_utils')
@pytest.mark.usefixtures('workspace')
@pytest.mark.parametrize('assume_role_arn', [constants.ASSUME_ROLE_ARN])
@pytest.mark.parametrize('feature_name', [AWS_CLIENT_AUTH_FEATURE_NAME])
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.usefixtures('resource_mappings')
@pytest.mark.parametrize('resource_mappings_filename', [constants.AWS_RESOURCE_MAPPING_FILE_NAME])
@pytest.mark.parametrize('region_name', [constants.AWS_REGION])
@pytest.mark.parametrize('session_name', [constants.SESSION_NAME])
@pytest.mark.parametrize('stacks', [[f'{constants.AWS_PROJECT_NAME}-{AWS_CLIENT_AUTH_FEATURE_NAME}-Stack-{constants.AWS_REGION}']])
class TestAWSClientAuthWindows(object):
    """
    Test class to verify AWS Client Auth gem features on Windows.
    """

    @pytest.mark.parametrize('level', ['AWS/ClientAuth'])
    def test_anonymous_credentials(self,
                                   level: str,
                                   launcher: pytest.fixture,
                                   resource_mappings: pytest.fixture,
                                   workspace: pytest.fixture,
                                   asset_processor: pytest.fixture
                                   ):
        """
        Test to verify AWS Cognito Identity pool anonymous authorization.

        Setup: Updates resource mapping file using existing CloudFormation stacks.
        Tests: Getting credentials when no credentials are configured
        Verification: Log monitor looks for success credentials log.
        """
        asset_processor.start()
        asset_processor.wait_for_idle()

        file_to_monitor = os.path.join(launcher.workspace.paths.project_log(), constants.GAME_LOG_NAME)
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

        launcher.args = ['+LoadLevel', level]
        launcher.args.extend(['-rhi=null'])

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - Success anonymous credentials'],
                unexpected_lines=['(Script) - Fail anonymous credentials'],
                halt_on_unexpected=True,
            )
            assert result, 'Anonymous credentials fetched successfully.'

    def test_password_signin_credentials(self,
                                         launcher: pytest.fixture,
                                         resource_mappings: pytest.fixture,
                                         workspace: pytest.fixture,
                                         asset_processor: pytest.fixture,
                                         aws_utils: pytest.fixture
                                         ):
        """
        Test to verify AWS Cognito IDP Password sign in and Cognito Identity pool authenticated authorization.

        Setup: Updates resource mapping file using existing CloudFormation stacks.
        Tests: Sign up new test user, admin confirm the user, sign in and get aws credentials.
        Verification: Log monitor looks for success credentials log.
        """
        asset_processor.start()
        asset_processor.wait_for_idle()

        file_to_monitor = os.path.join(launcher.workspace.paths.project_log(), constants.GAME_LOG_NAME)
        log_monitor = ly_test_tools.log.log_monitor.LogMonitor(launcher=launcher, log_file_path=file_to_monitor)

        cognito_idp = aws_utils.client('cognito-idp')
        user_pool_id = resource_mappings.get_resource_name_id(f'{AWS_CLIENT_AUTH_FEATURE_NAME}.CognitoUserPoolId')
        logger.info(f'UserPoolId:{user_pool_id}')

        # Remove the user if already exists
        try:
            cognito_idp.admin_delete_user(
                UserPoolId=user_pool_id,
                Username='test1'
            )
        except cognito_idp.exceptions.UserNotFoundException:
            pass

        launcher.args = ['+LoadLevel', 'AWS/ClientAuthPasswordSignUp']
        launcher.args.extend(['-rhi=null'])

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - Signup Success'],
                unexpected_lines=['(Script) - Signup Fail'],
                halt_on_unexpected=True,
            )
            assert result, 'Sign Up Success.'

        launcher.stop()

        cognito_idp.admin_confirm_sign_up(
            UserPoolId=user_pool_id,
            Username='test1'
        )

        launcher.args = ['+LoadLevel', 'AWS/ClientAuthPasswordSignIn']
        launcher.args.extend(['-rhi=null'])

        with launcher.start(launch_ap=False):
            result = log_monitor.monitor_log_for_lines(
                expected_lines=['(Script) - SignIn Success', '(Script) - Success credentials'],
                unexpected_lines=['(Script) - SignIn Fail', '(Script) - Fail credentials'],
                halt_on_unexpected=True,
            )
            assert result, 'Sign in Success, fetched authenticated AWS temp credentials.'
