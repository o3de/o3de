"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import os
import pytest

import ly_test_tools.environment.process_utils as process_utils


class CdkApplication:
    """
    CDK application is responsible for deploying and managing the AWS resources required by the backend.
    """
    def __init__(self, aws_project_name: str, account_id: str, region: str):
        """
        Initialize the environment variable and application information.
        :param aws_project_name: AWS project name.
        :param account_id: AWS account ID for interacting with AWS services.
        :param region: Region for deploying the CDK application.
        """
        self._cdk_env = dict()
        self._cdk_env['O3D_AWS_PROJECT_NAME'] = aws_project_name
        self._cdk_env['O3D_AWS_DEPLOY_REGION'] = region
        self._cdk_env['O3D_AWS_DEPLOY_ACCOUNT'] = account_id

        self._stack_name = ''
        self._cdk_application_path = ''

    def deploy(self, cdk_application_path: str = '', context_variable: str = ''):
        """
        Deploy the CDK application.
        :param cdk_application_path: Path to the CDK application code.
        :param context_variable: Context variable for enabling optional features.
        """
        if cdk_application_path:
            self._cdk_application_path = cdk_application_path

        if not self._cdk_application_path:
            return

        deploy_cdk_application_cmd = ['cdk', 'deploy', '--require-approval', 'never']
        if context_variable:
            deploy_cdk_application_cmd.extend(['-c', f'{context_variable}'])

        output = process_utils.check_output(
            deploy_cdk_application_cmd,
            cwd=self._cdk_application_path,
            env=dict(os.environ, **self._cdk_env),
            shell=True)

        # Output contains the ARN of the deployed stack. Parse the ARN to get the stack name.
        output_sections = output.split('/')
        assert len(output_sections), 3
        self._stack_name = output.split('/')[-2]

    def destroy(self):
        """
        Destroy the CDK application.
        """
        destroy_cdk_application_cmd = ['cdk', 'destroy', '-f']
        process_utils.check_output(
            destroy_cdk_application_cmd,
            cwd=self._cdk_application_path,
            env=dict(os.environ, **self._cdk_env),
            shell=True)

        self._stack_name = ''
        self._cdk_application_path = ''

    @property
    def stack_name(self):
        """
        Get the name of the CloudFormation stack created by teh CDK application.
        :return: Name of the CloudFormation stack.
        """
        return self._stack_name


@pytest.fixture(scope='class')
def cdk_application(
        request: pytest.fixture,
        aws_project_name: str,
        account_id: str,
        region: str):
    """
    Fixture for setting up a CDK application.
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :param aws_project_name: AWS project name.
    :param account_id: AWS account ID for interacting with AWS services.
    :param region: Region for deploying the CDK application.
    :return: CloudFormation stack name deployed by the CDK application.
    """
    application = CdkApplication(aws_project_name, account_id, region)

    def teardown():
        # Destroy the CDK application.
        application.destroy()
    request.addfinalizer(teardown)

    return application
