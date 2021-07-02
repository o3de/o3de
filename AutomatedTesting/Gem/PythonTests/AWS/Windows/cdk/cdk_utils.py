"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import pytest
import boto3
import uuid
import logging
import subprocess
import botocore

import ly_test_tools.environment.process_utils as process_utils
from typing import List

BOOTSTRAP_STACK_NAME = 'CDKToolkit'
BOOTSTRAP_STAGING_BUCKET_LOGIC_ID = 'StagingBucket'

logger = logging.getLogger(__name__)


class Cdk:
    """
    Cdk class that provides methods to run cdk application commands.
    Expects system to have NodeJS, AWS CLI and CDK installed globally and have their paths setup as env variables.
    """

    def __init__(self):
        self._cdk_env = ''
        self._stacks = []
        self._cdk_path = os.path.dirname(os.path.realpath(__file__))
        self._session = ''

        cdk_npm_latest_version_cmd = ['npm', 'view', 'aws-cdk', 'version']

        output = process_utils.check_output(
            cdk_npm_latest_version_cmd,
            cwd=self._cdk_path,
            shell=True)
        cdk_npm_latest_version = output.split()[0]

        cdk_version_cmd = ['cdk', 'version']
        output = process_utils.check_output(
            cdk_version_cmd,
            cwd=self._cdk_path,
            shell=True)
        cdk_version = output.split()[0]
        logger.info(f'Current CDK version {cdk_version}')

        if cdk_version != cdk_npm_latest_version:
            try:
                logger.info(f'Updating CDK to latest')
                # uninstall and reinstall cdk in case npm has been updated.
                output = process_utils.check_output(
                    'npm uninstall -g aws-cdk',
                    cwd=self._cdk_path,
                    shell=True)

                logger.info(f'Uninstall CDK output: {output}')

                output = process_utils.check_output(
                    'npm install -g aws-cdk@latest',
                    cwd=self._cdk_path,
                    shell=True)

                logger.info(f'Install CDK output: {output}')
            except subprocess.CalledProcessError as error:
                logger.warning(f'Failed reinstalling latest CDK on npm'
                               f'\nError:{error.stderr}')

    def setup(self, cdk_path: str, project: str, account_id: str,
                 workspace: pytest.fixture, session: boto3.session.Session, bootstrap_required: bool):
        """
        :param cdk_path: Path where cdk app.py is stored.
        :param project: Project name used for cdk project name env variable.
        :param account_id: AWS account id to use with cdk application.
        :param workspace: ly_test_tools workspace fixture.
        :param workspace: bootstrap_required deploys bootstrap stack.
        """

        self._cdk_env = os.environ.copy()
        unique_id = uuid.uuid4().hex[-4:]
        self._cdk_env['O3DE_AWS_PROJECT_NAME'] = project[:4] + unique_id if len(project) > 4 else project + unique_id
        self._cdk_env['O3DE_AWS_DEPLOY_REGION'] = session.region_name
        self._cdk_env['O3DE_AWS_DEPLOY_ACCOUNT'] = account_id
        self._cdk_env['PATH'] = f'{workspace.paths.engine_root()}\\python;' + self._cdk_env['PATH']

        credentials = session.get_credentials().get_frozen_credentials()
        self._cdk_env['AWS_ACCESS_KEY_ID'] = credentials.access_key
        self._cdk_env['AWS_SECRET_ACCESS_KEY'] = credentials.secret_key
        self._cdk_env['AWS_SESSION_TOKEN'] = credentials.token
        self._cdk_path = cdk_path

        self._session = session

        output = process_utils.check_output(
            'python -m pip install -r requirements.txt',
            cwd=self._cdk_path,
            env=self._cdk_env,
            shell=True)

        logger.info(f'Installing cdk python dependencies: {output}')

        if bootstrap_required:
            self.bootstrap()

    def bootstrap(self) -> None:
        """
        Deploy the bootstrap stack.
        """
        try:
            bootstrap_cmd = ['cdk', 'bootstrap',
                             f'aws://{self._cdk_env["O3DE_AWS_DEPLOY_ACCOUNT"]}/{self._cdk_env["O3DE_AWS_DEPLOY_REGION"]}']

            process_utils.check_call(
                bootstrap_cmd,
                cwd=self._cdk_path,
                env=self._cdk_env,
                shell=True)
        except botocore.exceptions.ClientError as clientError:
            logger.warning(f'Failed creating Bootstrap stack {BOOTSTRAP_STACK_NAME} not found. '
                           f'\nError:{clientError["Error"]["Message"]}')

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

        logger.info(f'CDK Path {self._cdk_path}')
        destroy_cdk_application_cmd = ['cdk', 'destroy', '--all', '-f']

        try:
            process_utils.check_output(
                destroy_cdk_application_cmd,
                cwd=self._cdk_path,
                env=self._cdk_env,
                shell=True)

        except subprocess.CalledProcessError as e:
            logger.error(e.output)
            raise e

        self._stacks = []

    def remove_bootstrap_stack(self) -> None:
        """
        Remove the CDK bootstrap stack.
        :param aws_utils: aws_utils fixture.
        """
        # Check if the bootstrap stack exists.
        response = self._session.client('cloudformation').describe_stacks(
            StackName=BOOTSTRAP_STACK_NAME
        )
        stacks = response.get('Stacks', [])
        if not stacks or len(stacks) is 0:
            return

        # Clear the bootstrap staging bucket before deleting the bootstrap stack.
        response = self._session.client('cloudformation').describe_stack_resource(
            StackName=BOOTSTRAP_STACK_NAME,
            LogicalResourceId=BOOTSTRAP_STAGING_BUCKET_LOGIC_ID
        )

        staging_bucket_name = response.get('StackResourceDetail', {}).get('PhysicalResourceId', '')
        if staging_bucket_name:
            s3 = self._session.resource('s3')
            bucket = s3.Bucket(staging_bucket_name)
            for key in bucket.objects.all():
                key.delete()

        # Delete the bootstrap stack.
        # Should not need to delete the stack if S3 bucket can be cleaned.
        # self._session.client('cloudformation').delete_stack(
        #     StackName=BOOTSTRAP_STACK_NAME
        # )
