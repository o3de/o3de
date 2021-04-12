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
import json
import os

from botocore.config import Config

AWS_RESOURCE_MAPPINGS_KEY = 'AWSResourceMappings'
DEFAULT_REGION = 'us-west-2'


class AwsUtils(object):
    """
    Retrieve boto3 clients/resources and provide util functions to modify the resource mappings file.
    """
    def __init__(self, region: str = DEFAULT_REGION):
        self._client_mappings = {}
        self._resource_mappings = {}
        self._config = Config(
            region_name=region,
            s3={
                'payload_signing_enabled': True
            }
        )

    def client(self, service: str):
        """
        Get the client for a specific AWS service. Config is defined by AwsUtils.

        :param service: Name of the AWS service.
        :return: Client for the AWS service.
        """
        if not self._client_mappings.get(service):
            self._client_mappings[service] = boto3.client(service, config=self._config)

        return self._client_mappings[service]

    def resource(self, service: str):
        """
        Get the resource for a specific AWS service. Config is defined by AwsUtils.

        :param service: Name of the AWS service.
        :return: Resource for the AWS service.
        """
        if not self._resource_mappings.get(service):
            self._resource_mappings[service] = boto3.resource(service, config=self._config)

        return self._resource_mappings[service]

    @staticmethod
    def add_resource_to_resource_mappings(
            resource_key: str,
            resource_type: str,
            resource_id: str,
            resource_mapping_file_path: str):
        """
        Add a resource to the resource mapping file.

        :param resource_key: Key for the resource.
        :param resource_type: Type of the resource.
        :param resource_id: Name/Id of the resource.
        :param resource_mapping_file_path: Path to the resource mapping file.
        """
        assert os.path.exists(resource_mapping_file_path), \
            f'Invalid resource mapping file path {resource_mapping_file_path}'

        with open(resource_mapping_file_path) as file_content:
            resource_mappings = json.load(file_content)

        resource_mappings[AWS_RESOURCE_MAPPINGS_KEY] = resource_mappings.get(AWS_RESOURCE_MAPPINGS_KEY, {})
        resource_mappings[AWS_RESOURCE_MAPPINGS_KEY][resource_key] = resource_mappings[AWS_RESOURCE_MAPPINGS_KEY].get(resource_key, {})
        resource_mappings[AWS_RESOURCE_MAPPINGS_KEY][resource_key]['Type'] = resource_type
        resource_mappings[AWS_RESOURCE_MAPPINGS_KEY][resource_key]['Name/ID'] = resource_id

        with open(resource_mapping_file_path, 'w') as file_content:
            json.dump(resource_mappings, file_content, indent=4)




