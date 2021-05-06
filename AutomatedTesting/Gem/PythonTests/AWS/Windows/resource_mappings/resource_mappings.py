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
import json

AWS_RESOURCE_MAPPINGS_KEY = 'AWSResourceMappings'
AWS_RESOURCE_MAPPINGS_ACCOUNT_ID_KEY = 'AccountId'
AWS_RESOURCE_MAPPINGS_REGION_KEY = 'Region'


class ResourceMappings:
    """
    ResourceMappings class that handles writing Cloud formation outputs to resource mappings json file in a project.
    """

    def __init__(self, file_path: str, region: str, profile: str, feature_name: str, account_id: str):
        """
        :param file_path: Path for the resource mapping file.
        :param region: Region value for the resource mapping file.
        :param profile: Profile to use for credentials when calling boto3 cloud formation service.SS
        :param feature_name: Feature gem name to use to append name to mappings key.
        :param account_id: AWS account id value for the resource mapping file.
        """
        self._cdk_env = os.environ.copy()
        self._cdk_env['PATH'] = 'e:\\o3de\\python\\runtime\\python-3.7.10-rev1-windows\\python\\;' + self._cdk_env[
            'PATH']
        self._resource_mapping_file_path = file_path
        self._region = region
        self._feauture_name = feature_name
        self._account_id = account_id

        assert os.path.exists(self._resource_mapping_file_path), \
            f'Invalid resource mapping file path {self._resource_mapping_file_path}'

        session = boto3.session.Session(profile_name=profile, region_name=region)
        self._client = session.client('cloudformation')

    def populate_output_keys(self, stacks=[]) -> None:
        """
        Calls describe stacks on cloud formation service and persists outputs to resource mappings file.
        :param stacks List of stack arns to describe and populate resource mappings with.
        """
        for stack_name in stacks:
            response = self._client.describe_stacks(
                StackName=stack_name
            )
            stacks = response.get('Stacks', [])
            assert len(stacks) == 1, f'{stack_name} is invalid.'

            self.__write_resource_mappings(stacks[0].get('Outputs', []))

    def __write_resource_mappings(self, outputs) -> None:
        with open(self._resource_mapping_file_path) as file_content:
            resource_mappings = json.load(file_content)

        resource_mappings[AWS_RESOURCE_MAPPINGS_ACCOUNT_ID_KEY] = self._account_id
        resource_mappings[AWS_RESOURCE_MAPPINGS_REGION_KEY] = self._region

        # Append new mappings.
        resource_mappings[AWS_RESOURCE_MAPPINGS_KEY] = resource_mappings.get(AWS_RESOURCE_MAPPINGS_KEY, {})

        for output in outputs:
            resource_key = f'{self._feauture_name}.{output.get("OutputKey", "InvalidKey")}'
            resource_mappings[AWS_RESOURCE_MAPPINGS_KEY][resource_key] = resource_mappings[
                AWS_RESOURCE_MAPPINGS_KEY].get(resource_key, {})
            resource_mappings[AWS_RESOURCE_MAPPINGS_KEY][resource_key]['Type'] = 'AutomationTestType'
            resource_mappings[AWS_RESOURCE_MAPPINGS_KEY][resource_key]['Name/ID'] = output.get('OutputValue',
                                                                                               'InvalidId')

        with open(self._resource_mapping_file_path, 'w') as file_content:
            json.dump(resource_mappings, file_content, indent=4)

    def destroy(self) -> None:
        """
        Resets resource mappings file
        """
        with open(self._resource_mapping_file_path) as file_content:
            resource_mappings = json.load(file_content)

        resource_mappings[AWS_RESOURCE_MAPPINGS_ACCOUNT_ID_KEY] = ''
        resource_mappings[AWS_RESOURCE_MAPPINGS_REGION_KEY] = 'us-west-2'

        # Append new mappings.
        resource_mappings[AWS_RESOURCE_MAPPINGS_KEY] = resource_mappings.get(AWS_RESOURCE_MAPPINGS_KEY, {})
        resource_mappings[AWS_RESOURCE_MAPPINGS_KEY] = {}

        with open(self._resource_mapping_file_path, 'w') as file_content:
            json.dump(resource_mappings, file_content, indent=4)

        self._resource_mapping_file_path = ''
        self._region = ''
        self._client = None


@pytest.fixture(scope='function')
def resource_mappings(
        request: pytest.fixture,
        project: str,
        region: str,
        profile: str,
        feature_name: str,
        resource_mappings_filename: str,
        account_id: str,
        workspace: pytest.fixture) -> ResourceMappings:
    """
    Fixture for setting up resource mappings file.
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :param project: Project to find resource mapping file.
    :param region: Region to update in resource mapping file.
    :param profile: AWS profile used for credentials to describe stacks.
    :param feature_name: AWS Gem name that is prepended to resource mapping keys.
    :param resource_mappings_filename: Name of resource mapping file.
    :param account_id: AWS account query stacks from.
    :param workspace: Region for deploying the CDK application.
    :return: ResourceMappings class object.
    """

    path = f'{workspace.paths.engine_root()}\\{project}\\Config\\{resource_mappings_filename}'
    resource_mappings_obj = ResourceMappings(path, region, profile, feature_name, account_id)

    def teardown():
        resource_mappings_obj.destroy()

    request.addfinalizer(teardown)

    return resource_mappings_obj
