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
import boto3

import ly_test_tools.environment.process_utils as process_utils
from typing import List

BOOTSTRAP_STACK_NAME = 'CDKToolkit'
BOOTSTRAP_STAGING_BUCKET_LOGIC_ID = 'StagingBucket'

class Cdk:
    """
    Cdk class that provides methods to run cdk application commands.
    Expects system to have NodeJS, AWS CLI and CDK installed globally and have their paths setup as env variables.
    """

    def __init__(self, cdk_path: str, project: str, account_id: str,
                 workspace: pytest.fixture, session: boto3.session.Session):
        """
        :param cdk_path: Path where cdk app.py is stored.
        :param project: Project name used for cdk project name env variable.
        :param account_id: AWS account id to use with cdk application.
        :param workspace: ly_test_tools workspace fixture.
        """
        self._cdk_env = os.environ.copy()
        self._cdk_env['O3DE_AWS_PROJECT_NAME'] = project
        self._cdk_env['O3DE_AWS_DEPLOY_REGION'] = session.region_name
        self._cdk_env['O3DE_AWS_DEPLOY_ACCOUNT'] = account_id
        self._cdk_env['PATH'] = f'{workspace.paths.engine_root()}\\python;' + self._cdk_env['PATH']

        credentials = session.get_credentials().get_frozen_credentials()
        self._cdk_env['AWS_ACCESS_KEY_ID'] = credentials.access_key
        self._cdk_env['AWS_SECRET_ACCESS_KEY'] = credentials.secret_key
        self._cdk_env['AWS_SESSION_TOKEN'] = credentials.token
        self._stacks = []
        self._cdk_path = cdk_path

        output = process_utils.check_output(
            'python -m pip install -r requirements.txt',
            cwd=self._cdk_path,
            env=self._cdk_env,
            shell=True)

    def bootstrap(self) -> None:
        """
        Deploy the bootstrap stack.
        """
        bootstrap_cmd = ['cdk', 'bootstrap',
                         f'aws://{self._cdk_env["O3DE_AWS_DEPLOY_ACCOUNT"]}/{self._cdk_env["O3DE_AWS_DEPLOY_REGION"]}']

        process_utils.check_call(
            bootstrap_cmd,
            cwd=self._cdk_path,
            env=self._cdk_env,
            shell=True)

    def list(self) -> List[str]:
        """
        lists cdk stack names
        :return List of cdk stack names
        """
        if not self._cdk_path:
            return []

        list_cdk_application_cmd = ['cdk', 'list']
        output = process_utils.check_output(
            list_cdk_application_cmd,
            cwd=self._cdk_path,
            env=self._cdk_env,
            shell=True)

        return output.splitlines()

    def synthesize(self) -> None:
        """
        Synthesizes all cdk stacks
        """
        if not self._cdk_path:
            return

        list_cdk_application_cmd = ['cdk', 'synth']

        process_utils.check_output(
            list_cdk_application_cmd,
            cwd=self._cdk_path,
            env=self._cdk_env,
            shell=True)

    def deploy(self, context_variable: str = '', additonal_params: List[str] = None) -> List[str]:
        """
        Deploys all the CDK stacks.
        :param context_variable: Context variable for enabling optional features.
        :param additonal_params: Additonal parameters like --all can be passed in this way.
        :return List of deployed stack arns.
        """
        if not self._cdk_path:
            return []

        deploy_cdk_application_cmd = ['cdk', 'deploy', '--require-approval', 'never']
        if additonal_params:
            deploy_cdk_application_cmd.extend(additonal_params)
        if context_variable:
            deploy_cdk_application_cmd.extend(['-c', f'{context_variable}'])

        output = process_utils.check_output(
            deploy_cdk_application_cmd,
            cwd=self._cdk_path,
            env=self._cdk_env,
            shell=True)

        stacks = []
        for line in output.splitlines():
            line_sections = line.split('/')
            assert len(line_sections), 3
            stacks.append(line.split('/')[-2])

        return stacks

    def destroy(self) -> None:
        """
        Destroys the cdk application.
        """
        destroy_cdk_application_cmd = ['cdk', 'destroy', '-f']
        process_utils.check_output(
            destroy_cdk_application_cmd,
            cwd=self._cdk_path,
            env=self._cdk_env,
            shell=True)

        self._stacks = []
        self._cdk_path = ''

    @staticmethod
    def remove_bootstrap_stack(aws_utils: pytest.fixture) -> None:
        """
        Remove the CDK bootstrap stack.
        :param aws_utils: aws_utils fixture.
        """
        # Check if the bootstrap stack exists.
        response = aws_utils.client('cloudformation').describe_stacks(
            StackName=BOOTSTRAP_STACK_NAME
        )
        stacks = response.get('Stacks', [])
        if not stacks:
            return

        # Clear the bootstrap staging bucket before deleting the bootstrap stack.
        response = aws_utils.client('cloudformation').describe_stack_resource(
            StackName=BOOTSTRAP_STACK_NAME,
            LogicalResourceId=BOOTSTRAP_STAGING_BUCKET_LOGIC_ID
        )

        staging_bucket_name = response.get('StackResourceDetail', {}).get('PhysicalResourceId', '')
        if staging_bucket_name:
            s3 = aws_utils.resource('s3')
            bucket = s3.Bucket(staging_bucket_name)
            for key in bucket.objects.all():
                key.delete()

        # Delete the bootstrap stack.
        aws_utils.client('cloudformation').delete_stack(
            StackName=BOOTSTRAP_STACK_NAME
        )


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
    cdk_obj = Cdk(cdk_path, project, aws_utils.assume_account_id(), workspace, aws_utils.assume_session())

    if bootstrap_required:
        cdk_obj.bootstrap()

    def teardown():
        if destroy_stacks_on_teardown:
            cdk_obj.destroy()
            cdk_obj.remove_bootstrap_stack(aws_utils)

    request.addfinalizer(teardown)

    return cdk_obj
