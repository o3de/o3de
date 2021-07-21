"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
from os.path import abspath
import pytest
import json
import logging

logger = logging.getLogger(__name__)

AWS_RESOURCE_MAPPINGS_KEY = 'AWSResourceMappings'
AWS_RESOURCE_MAPPINGS_ACCOUNT_ID_KEY = 'AccountId'
AWS_RESOURCE_MAPPINGS_REGION_KEY = 'Region'


class ResourceMappings:
    """
    ResourceMappings class that handles writing Cloud formation outputs to resource mappings json file in a project.
    """

    def __init__(self, file_path: str, region: str, feature_name: str, account_id: str, workspace: pytest.fixture,
                 cloud_formation_client):
        """
        :param file_path: Path for the resource mapping file.
        :param region: Region value for the resource mapping file.
        :param feature_name: Feature gem name to use to append name to mappings key.
        :param account_id: AWS account id value for the resource mapping file.
        :param workspace: ly_test_tools workspace fixture.
        :param cloud_formation_client: AWS cloud formation client.
        """
        self._cdk_env = os.environ.copy()
        self._cdk_env['PATH'] = f'{workspace.paths.engine_root()}\\python;' + self._cdk_env['PATH']
        self._resource_mapping_file_path = file_path
        self._region = region
        self._feature_name = feature_name
        self._account_id = account_id
        self._resource_mappings = {}

        assert os.path.exists(self._resource_mapping_file_path), \
            f'Invalid resource mapping file path {self._resource_mapping_file_path}'
        self._client = cloud_formation_client

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

            self._write_resource_mappings(stacks[0].get('Outputs', []))

    def _write_resource_mappings(self, outputs, append_feature_name = True) -> None:
        with open(self._resource_mapping_file_path) as file_content:
            resource_mappings = json.load(file_content)

        resource_mappings[AWS_RESOURCE_MAPPINGS_ACCOUNT_ID_KEY] = self._account_id
        resource_mappings[AWS_RESOURCE_MAPPINGS_REGION_KEY] = self._region

        # Append new mappings.
        resource_mappings[AWS_RESOURCE_MAPPINGS_KEY] = resource_mappings.get(AWS_RESOURCE_MAPPINGS_KEY, {})

        for output in outputs:
            if append_feature_name:
                resource_key = f'{self._feature_name}.{output.get("OutputKey", "InvalidKey")}'
            else:
                resource_key = output.get("OutputKey", "InvalidKey")
            resource_mappings[AWS_RESOURCE_MAPPINGS_KEY][resource_key] = resource_mappings[
                AWS_RESOURCE_MAPPINGS_KEY].get(resource_key, {})
            resource_mappings[AWS_RESOURCE_MAPPINGS_KEY][resource_key]['Type'] = 'AutomationTestType'
            resource_mappings[AWS_RESOURCE_MAPPINGS_KEY][resource_key]['Name/ID'] = output.get('OutputValue',
                                                                                               'InvalidId')

        self._resource_mappings = resource_mappings
        with open(self._resource_mapping_file_path, 'w') as file_content:
            json.dump(resource_mappings, file_content, indent=4)

    def clear_output_keys(self) -> None:
        """
        Clears values of all resource mapping keys. Sets region to default to us-west-2
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

    def get_resource_name_id(self, resource_key: str):
        return self._resource_mappings[AWS_RESOURCE_MAPPINGS_KEY][resource_key]['Name/ID']


@pytest.fixture(scope='function')
def resource_mappings(
        request: pytest.fixture,
        project: str,
        feature_name: str,
        resource_mappings_filename: str,
        workspace: pytest.fixture,
        aws_utils: pytest.fixture) -> ResourceMappings:
    """
    Fixture for setting up resource mappings file.
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :param project: Project to find resource mapping file.
    :param feature_name: AWS Gem name that is prepended to resource mapping keys.
    :param resource_mappings_filename: Name of resource mapping file.
    :param workspace: ly_test_tools workspace fixture.
    :param aws_utils: AWS utils fixture.
    :return: ResourceMappings class object.
    """

    path = f'{workspace.paths.engine_root()}/{project}/Config/{resource_mappings_filename}'
    logger.info(f'Resource mapping path : {path}')
    logger.info(f'Resource mapping resolved path : {abspath(path)}')
    resource_mappings_obj = ResourceMappings(abspath(path), aws_utils.assume_session().region_name, feature_name,
                                             aws_utils.assume_account_id(), workspace,
                                             aws_utils.client('cloudformation'))

    def teardown():
        resource_mappings_obj.clear_output_keys()

    request.addfinalizer(teardown)

    return resource_mappings_obj
