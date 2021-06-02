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
import logging

logger = logging.getLogger(__name__)
logging.getLogger('boto').setLevel(logging.CRITICAL)


class AwsUtils:

    def __init__(self, arn: str, session_name: str, region_name: str):
        local_session = boto3.Session(profile_name='default')
        local_sts_client = local_session.client('sts')
        self._local_account_id = local_sts_client.get_caller_identity()["Account"]
        logger.info(f'Local Account Id: {self._local_account_id}')

        response = local_sts_client.assume_role(RoleArn=arn, RoleSessionName=session_name)

        self._assume_session = boto3.Session(aws_access_key_id=response['Credentials']['AccessKeyId'],
                                             aws_secret_access_key=response['Credentials']['SecretAccessKey'],
                                             aws_session_token=response['Credentials']['SessionToken'],
                                             region_name=region_name)

        assume_sts_client = self._assume_session.client('sts')
        assume_account_id = assume_sts_client.get_caller_identity()["Account"]
        logger.info(f'Assume Account Id: {assume_account_id}')
        self._assume_account_id = assume_account_id

    def client(self, service: str):
        """
        Get the client for a specific AWS service from configured session
        :return: Client for the AWS service.
        """
        return self._assume_session.client(service)

    def resource(self, service: str):
        """
        Get the resource for a specific AWS service from configured session
        :return: Client for the AWS service.
        """
        return self._assume_session.resource(service)

    def assume_session(self):
        return self._assume_session

    def local_account_id(self):
        return self._local_account_id

    def assume_account_id(self):
        return self._assume_account_id

    def destroy(self) -> None:
        """
        clears stored session
        """
        self._assume_session = None


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
