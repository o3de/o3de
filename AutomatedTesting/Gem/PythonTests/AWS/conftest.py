"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import logging
from os.path import abspath
import pytest
import typing

from AWS.common.aws_utils import AwsUtils
from AWS.common.aws_credentials import AwsCredentials
from AWS.common.resource_mappings import ResourceMappings

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
def resource_mappings(
        request: pytest.fixture,
        project: str,
        feature_name: str,
        resource_mappings_filename: str,
        stacks: typing.List,
        workspace: pytest.fixture,
        aws_utils: pytest.fixture) -> ResourceMappings:
    """
    Fixture for setting up resource mappings file.
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :param project: Project to find resource mapping file.
    :param feature_name: AWS Gem name that is prepended to resource mapping keys.
    :param resource_mappings_filename: Name of resource mapping file.
    :param stacks: List of stack names to describe and populate resource mappings with.
    :param workspace: ly_test_tools workspace fixture.
    :param aws_utils: AWS utils fixture.
    :return: ResourceMappings class object.
    """

    path = f'{workspace.paths.engine_root()}/{project}/Config/{resource_mappings_filename}'
    logger.info(f'Resource mapping path : {path}')
    logger.info(f'Resource mapping resolved path : {abspath(path)}')
    resource_mappings_obj = ResourceMappings(abspath(path), aws_utils.assume_session().region_name, feature_name,
                                             aws_utils.assume_account_id(), aws_utils.client('cloudformation'))
    resource_mappings_obj.populate_output_keys(stacks)

    def teardown():
        resource_mappings_obj.clear_output_keys()

    request.addfinalizer(teardown)

    return resource_mappings_obj


@pytest.fixture(scope='function')
def aws_credentials(request: pytest.fixture, aws_utils: pytest.fixture, profile_name: str):
    """
    Fixture for setting up temporary AWS credentials from assume role.

    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :param aws_utils: aws_utils fixture.
    :param profile_name: Named AWS profile to store temporary credentials.
    """
    aws_credentials_obj = AwsCredentials(profile_name)
    original_access_key, original_secret_access_key, original_token = aws_credentials_obj.get_aws_credentials()
    aws_credentials_obj.set_aws_credentials_by_session(aws_utils.assume_session())

    def teardown():
        # Reset to the named profile using the original AWS credentials
        aws_credentials_obj.set_aws_credentials(original_access_key, original_secret_access_key, original_token)
    request.addfinalizer(teardown)

    return aws_credentials_obj
