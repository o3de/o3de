"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import pytest
import logging
from AWS.common.aws_utils import AwsUtils
from AWS.Windows.cdk.cdk_utils import Cdk

logger = logging.getLogger(__name__)


@pytest.fixture(scope='function')
def aws_utils(
        request: pytest.fixture,
        assume_role_arn: str,
        session_name: str,
        region_name: str):
    """
    Fixture for AWS util functions
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :param assume_role_arn: Role used to fetch temporary aws credentials, configure service clients with obtained credentials.
    :param session_name: Session name to set.
    :param region_name: AWS account region to set for session.
    :return AWSUtils class object.
    """

    aws_utils_obj = AwsUtils(assume_role_arn, session_name, region_name)

    def teardown():
        aws_utils_obj.destroy()

    request.addfinalizer(teardown)

    return aws_utils_obj

# Set global pytest variable for cdk to avoid recreating instance
pytest.cdk_obj = None


@pytest.fixture(scope='function')
def cdk(
        request: pytest.fixture,
        project: str,
        feature_name: str,
        workspace: pytest.fixture,
        aws_utils: pytest.fixture,
        bootstrap_required: bool = True,
        destroy_stacks_on_teardown: bool = True) -> Cdk:
    """
    Fixture for setting up a Cdk
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :param project: Project name used for cdk project name env variable.
    :param feature_name: Feature gem name to expect cdk folder in.
    :param workspace: ly_test_tools workspace fixture.
    :param aws_utils: aws_utils fixture.
    :param bootstrap_required: Whether the bootstrap stack needs to be created to
        provision resources the AWS CDK needs to perform the deployment.
    :param destroy_stacks_on_teardown: option to control calling destroy ot the end of test.
    :return Cdk class object.
    """

    cdk_path = f'{workspace.paths.engine_root()}/Gems/{feature_name}/cdk'
    logger.info(f'CDK Path {cdk_path}')

    if pytest.cdk_obj is None:
        pytest.cdk_obj = Cdk()

    pytest.cdk_obj.setup(cdk_path, project, aws_utils.assume_account_id(), workspace, aws_utils.assume_session(),
                             bootstrap_required)
    def teardown():
        if destroy_stacks_on_teardown:
            pytest.cdk_obj.destroy()
            # Enable after https://github.com/aws/aws-cdk/issues/986 is fixed.
            # Until then clean the bootstrap bucket manually.
            # cdk_obj.remove_bootstrap_stack()

    request.addfinalizer(teardown)

    return pytest.cdk_obj
