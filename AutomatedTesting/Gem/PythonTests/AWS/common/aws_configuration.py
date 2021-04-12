"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
import pytest
import os
import json

RESOURCE_MAPPING_FILE = 'Config/automated_testing_aws_resource_mappings.json'
RESOURCE_MAPPING_FILE_DEFAULT_VERSION = '1.0.0'
AWS_CORE_CONFIGURATION_FILE = 'Registry/awscoreconfiguration.setreg'


class AwsSolutionConfiguration:
    """
    AwsSolutionContext is used to store the configuration for AWS core and feature gems.
    """
    def __init__(self, project_path: str = ''):
        """
        Initialize the path for resource mapping file and aws core configuration file.
        :param project_path: Path to the project folder.
        """
        self._resource_mapping_file = os.path.join(project_path, RESOURCE_MAPPING_FILE)
        self._aws_core_configuration_file = os.path.join(project_path, AWS_CORE_CONFIGURATION_FILE)

    def reset_resource_mapping_file_content(self, content: dict):
        """
        Reset the content of the resource mapping file.
        :param content: Content of the resource mapping file.
        """
        self._reset_file_content(self.resource_mapping_file, content)

    def reset_aws_core_configuration_file_content(self, content: dict):
        """
        Reset the content of the aws core configuration file.
        :param content: Content of the aws core configuration file.
        """
        self._reset_file_content(self.aws_core_configuration_file, content)

    @property
    def resource_mapping_file(self):
        """
        Get the path to the resource mapping file.
        :return: Path to the resource mapping file.
        """
        return self._resource_mapping_file

    @property
    def aws_core_configuration_file(self):
        """
        Get the path to the aws core configuration file.
        :return: Path to the aws core configuration file.
        """
        return self._aws_core_configuration_file

    @staticmethod
    def _reset_file_content(path: str, content: dict):
        """
        Helper function to reset the config file content.
        :param path: Path to the config file.
        :param content: Updated content for the config file.
        """
        if not os.path.exists(os.path.dirname(path)):
            os.makedirs(os.path.dirname(path))

        with open(path, 'w') as file:
            json.dump(content, file, indent=4)


@pytest.fixture
def aws_configuration(
        request: pytest.fixture,
        workspace: pytest.fixture,
        profile_name: str,
        account_id: str,
        region: str):
    """
    Fixture for setting up the resource mapping file.
    :param request: _pytest.fixtures.SubRequest class that handles getting
        a pytest fixture from a pytest function/fixture.
    :param workspace: Workspace for running the test.
    :param profile_name: AWS profile for interacting with AWS services.
    :param account_id: AWS account ID for interacting with AWS services.
    :param region: Region for deploying AWS resources.
    :return: path to the resource mapping file.
    """
    configuration = AwsSolutionConfiguration(workspace.paths.project())

    resource_mapping_file_content = {'AccountId': account_id, 'Region': region,
                                     'Version': RESOURCE_MAPPING_FILE_DEFAULT_VERSION}
    configuration.reset_resource_mapping_file_content(resource_mapping_file_content)

    aws_core_configuration_file_content = {
        'Amazon': {
            'AWSCore': {
                'ProfileName': profile_name,
                'ResourceMappingConfigFileName': os.path.basename(configuration.resource_mapping_file)
            }
        }
    }
    configuration.reset_aws_core_configuration_file_content(aws_core_configuration_file_content)

    def teardown():
        # Remove all the configuration files for testing.
        os.remove(configuration.resource_mapping_file)
        os.remove(configuration.aws_core_configuration_file)
    request.addfinalizer(teardown)

    return configuration

