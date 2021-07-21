"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest
import os
import logging
import ly_test_tools.log.log_monitor

# fixture imports
from AWS.Windows.resource_mappings.resource_mappings import resource_mappings
from AWS.Windows.cdk.cdk_utils import Cdk
from AWS.common.aws_utils import AwsUtils
from assetpipeline.ap_fixtures.asset_processor_fixture import asset_processor as asset_processor

AWS_PROJECT_NAME = 'AWS-AutomationTest'
AWS_CLIENT_AUTH_FEATURE_NAME = 'AWSClientAuth'
AWS_CLIENT_AUTH_DEFAULT_PROFILE_NAME = 'default'

GAME_LOG_NAME = 'Game.log'

logger = logging.getLogger(__name__)


@pytest.mark.SUITE_periodic
@pytest.mark.usefixtures('automatic_process_killer')
@pytest.mark.usefixtures('asset_processor')
@pytest.mark.usefixtures('workspace')
@pytest.mark.parametrize('project', ['AutomatedTesting'])
@pytest.mark.parametrize('level', ['AWS/ClientAuth'])
@pytest.mark.usefixtures('cdk')
@pytest.mark.parametrize('feature_name', [AWS_CLIENT_AUTH_FEATURE_NAME])
@pytest.mark.usefixtures('resource_mappings')
@pytest.mark.parametrize('resource_mappings_filename', ['default_aws_resource_mappings.json'])
@pytest.mark.usefixtures('aws_utils')
@pytest.mark.parametrize('region_name', ['us-west-2'])
@pytest.mark.parametrize('assume_role_arn', ['arn:aws:iam::645075835648:role/o3de-automation-tests'])
@pytest.mark.parametrize('session_name', ['o3de-Automation-session'])
class TestAWSClientAuthAnonymousCredentials(object):
    """
    Test class to verify AWS Cognito Identity pool anonymous authorization.
    """

    def test_anonymous_credentials(self,
                                   level: str,
                                   launcher: pytest.fixture,
                                   cdk: pytest.fixture,
                                   resource_mappings: pytest.fixture,
                                   workspace: pytest.fixture,
                                   asset_processor: pytest.fixture
                                   ):
        """
        Setup: Deploys cdk and updates resource mapping file.
        Tests: Getting AWS credentials for no signed in user.
        Verification: Log monitor looks for success credentials log.
        """
        logger.info(f'Cdk stack names:\n{cdk.list()}')
        stacks = cdk.deploy()
        resource_mappings.populate_output_keys(stacks)
        asset_processor.start()
        asset_processor.wait_for_idle()

        file_to_monitor = os.path.join(launcher.workspace.paths.project_log(), GAME_LOG_NAME)
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
