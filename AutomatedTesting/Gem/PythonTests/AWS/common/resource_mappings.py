"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os
import json
import logging
from AWS.common import constants

logger = logging.getLogger(__name__)

AWS_RESOURCE_MAPPINGS_KEY = 'AWSResourceMappings'
AWS_RESOURCE_MAPPINGS_ACCOUNT_ID_KEY = 'AccountId'
AWS_RESOURCE_MAPPINGS_REGION_KEY = 'Region'


class ResourceMappings:
    """
    ResourceMappings class that handles writing Cloud formation outputs to resource mappings json file in a project.
    """

    def __init__(self, file_path: str, region: str, feature_name: str, account_id: str, cloud_formation_client):
        """
        :param file_path: Path for the resource mapping file.
        :param region: Region value for the resource mapping file.
        :param feature_name: Feature gem name to use to append name to mappings key.
        :param account_id: AWS account id value for the resource mapping file.
        :param cloud_formation_client: AWS cloud formation client.
        """
        self._resource_mapping_file_path = file_path
        self._region = region
        self._feature_name = feature_name
        self._account_id = account_id
        self._resource_mappings = {}

        assert os.path.exists(self._resource_mapping_file_path), \
            f'Invalid resource mapping file path {self._resource_mapping_file_path}'
        self._client = cloud_formation_client

    def populate_output_keys(self, stacks=None) -> None:
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

    def _write_resource_mappings(self, outputs, append_feature_name=True) -> None:
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
        resource_mappings[AWS_RESOURCE_MAPPINGS_REGION_KEY] = constants.AWS_REGION

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

    def clear_select_keys(self, resource_keys=None) -> None:
        """
        Clears values from select resource mapping keys.
        :param resource_keys: list of keys to clear out
        """
        with open(self._resource_mapping_file_path) as file_content:
            resource_mappings = json.load(file_content)

        for key in resource_keys:
            resource_mappings[key] = ''

        with open(self._resource_mapping_file_path, 'w') as file_content:
            json.dump(resource_mappings, file_content, indent=4)