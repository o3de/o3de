"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.
For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import boto3
import pytest
import subprocess
import typing

import ly_test_tools.environment.process_utils as process_utils


class AwsCredentials:
    def __init__(self, profile_name: str):
        self._profile_name = profile_name

    def get_aws_credentials(self) -> typing.Tuple[str, str, str]:
        """
        Get aws credentials from the specific named profile.

        :return AWS credentials.
        """
        access_key_id = self._get_aws_credential_attribute_value('aws_access_key_id')
        secret_access_key = self._get_aws_credential_attribute_value('aws_secret_access_key')
        session_token = self._get_aws_credential_attribute_value('aws_session_token')

        return access_key_id, secret_access_key, session_token

    def set_aws_credentials_by_session(self, session: boto3.Session) -> None:
        """
        Set AWS credentials stored in the specific named profile using an assumed role session.

        :param session: assumed role session.
        """
        credentials = session.get_credentials().get_frozen_credentials()
        self.set_aws_credentials(credentials.access_key, credentials.secret_key, credentials.token)

    def set_aws_credentials(self, aws_access_key_id: str, aws_secret_access_key: str,
                            aws_session_token: str) -> None:
        """
        Set AWS credentials stored in the specific named profile.

        :param aws_access_key_id: AWS access key id.
        :param aws_secret_access_key: AWS secrete access key.
        :param aws_session_token: AWS assumed role session.
        """
        process_utils.check_call(['aws', 'configure', 'set', 'aws_access_key_id',
                                  aws_access_key_id, '--profile', self._profile_name], shell=True)
        process_utils.check_call(['aws', 'configure', 'set', 'aws_secret_access_key',
                                  aws_secret_access_key, '--profile', self._profile_name], shell=True)
        process_utils.check_call(['aws', 'configure', 'set', 'aws_session_token',
                                  aws_session_token, '--profile', self._profile_name], shell=True)

    def _get_aws_credential_attribute_value(self, key_name: str) -> str:
        """
        Get the value of an AWS credential attribute from the specific named profile.

        :return Value of an AWS credential attribute.
        """
        try:
            value = process_utils.check_output(
                ['aws', 'configure', 'get', f'{self._profile_name}.{key_name}'], shell=True)
        except subprocess.CalledProcessError as e:
            if not e.output:
                # Named profile or key doesn't exist
                value = ''
            else:
                raise e

        if value == '\r\n':
            # Value is a new line character
            value = ''

        return value


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
