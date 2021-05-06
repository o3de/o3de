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
from typing import List


class Cdk:
    """
    Cdk class that provides methods to run cdk application commands.
    Expects system to have NodeJS, AWS CLI and CDK installed globally and have their paths setup as env variables.
    """
    def __init__(self, cdk_application_path: str, project: str, account_id: str, region: str, profile: str
                 , workspace: pytest.fixture):
        """
        :param cdk_application_path: Path where cdk app.py is stored.
        :param project: Project name used for cdk project name env variable.
        :param account_id: AWS account to deploy cdk resources in.
        :param region: Region for deploying the cdk resources in.
        :param profile: AWS profile to use for credentials.
        :param workspace: ly_test_tools workspace fixture.
        """
        self._cdk_env = os.environ.copy()
        self._cdk_env['O3DE_AWS_PROJECT_NAME'] = project
        self._cdk_env['O3DE_AWS_DEPLOY_REGION'] = region
        self._cdk_env['O3DE_AWS_DEPLOY_ACCOUNT'] = account_id
        self._cdk_env['PATH'] = f'{workspace.paths.engine_root()}\\python\\runtime\\python-3.7.10-rev1-windows' \
                                f'\\python\\;' + self._cdk_env['PATH']
        self._stacks = []
        self._cdk_path = cdk_application_path
        self._profile = profile

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

    def synth(self) -> None:
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

    def deploy(self, context_variable: str = '') -> List[str]:
        """
        Deploys all the CDK stacks.
        :param context_variable: Context variable for enabling optional features.
        :return List of deployed stack arns.
        """
        if not self._cdk_path:
            return []

        deploy_cdk_application_cmd = ['cdk', 'deploy', '--require-approval', 'never', '--profile', self._profile]
        if context_variable:
            deploy_cdk_application_cmd.extend(['-c', f'{context_variable}'])

        output = process_utils.check_output(
            deploy_cdk_application_cmd,
            cwd=self._cdk_path,
            env=dict(os.environ, **self._cdk_env),
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
            env=dict(os.environ, **self._cdk_env),
            shell=True)

        self._stacks = []
        self._cdk_path = ''
        self._profile = ''


@pytest.fixture(scope='function')
def cdk(
        request: pytest.fixture,
        project: str,
        account_id: str,
        region: str,
        feature_name: str,
        profile: str,
        workspace: pytest.fixture,
        destroy_stacks_on_teardown: bool = True) -> Cdk:
    """
    Fixture for setting up a Cdk
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :param project: Project name used for cdk project name env variable.
    :param account_id: AWS account to deploy cdk resources in.
    :param region: Region for deploying the cdk resources in.
    :param feature_name: Feature gem name to expect cdk folder in.
    :param profile: AWS profile to use for credentials.
    :param workspace: ly_test_tools workspace fixture.
    :param destroy_stacks_on_teardown: option to control calling destroy ot the end of test.
    :return Cdk class object.
    """

    cdk_path = f'{workspace.paths.engine_root()}/Gems/{feature_name}/cdk'
    cdk_obj = Cdk(cdk_path, project, account_id, region, profile, workspace)

    def teardown():
        if destroy_stacks_on_teardown:
            cdk_obj.destroy()
    request.addfinalizer(teardown)

    return cdk_obj
