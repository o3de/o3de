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

class AwsUtils:

    def __init__(self, arn: str, session_name: str):
        client = boto3.client('sts')
        account_id = client.get_caller_identity()["Account"]
        print(account_id)

        response = client.assume_role(RoleArn=arn, RoleSessionName=session_name)

        self._session = boto3.Session(aws_access_key_id=response['Credentials']['AccessKeyId'],
                                aws_secret_access_key=response['Credentials']['SecretAccessKey'],
                                aws_session_token=response['Credentials']['SessionToken'])

        client = self._session.client('sts')
        account_id = client.get_caller_identity()["Account"]
        print(account_id)

    def client(self, service: str):
        """
        Get the client for a specific AWS service from configured session
        :return: Client for the AWS service.
        """
        return self._session.client(service)

    def session(self):
        return self._session

    def destroy(self) -> None:
        """
        clears stored session
        """
        self._session = None


@pytest.fixture(scope='function')
def aws_utils(
        request: pytest.fixture,
        assume_role_arn: str,
        session_name: str):
    """
    Fixture for setting up a Cdk
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :param assume_role_arn: Project name used for cdk project name env variable.
    :param session_name: AWS account to deploy cdk resources in.
    :return AWSUtils class object.
    """
    aws_utils_obj = AwsUtils(assume_role_arn, session_name)

    def teardown():
        aws_utils_obj.destroy()

    request.addfinalizer(teardown)

    return aws_utils_obj
