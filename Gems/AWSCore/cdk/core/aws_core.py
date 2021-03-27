"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from aws_cdk import core

from .core_stack import CoreStack
from constants import Constants


class AWSCore(core.Construct):
    """
    Orchestrates setting up the AWS Core Stack(s)

    The recommendation is that the Core stack holds regionless/global resources or resources
    that only get deployed in a single region (such as a resource group to group all your resources)
    """

    def __init__(self,
                 scope: core.Construct,
                 id_: str,
                 project_name: str,
                 feature_name: str,
                 env: core.Environment) -> None:
        super().__init__(scope, id_)
        # Expectation is that you will only deploy this stack once and that the stack has regionless resources only
        stack_name = f'{project_name}-{feature_name}'

        # Deploy AWS Metrics Stack
        self._feature_stack = CoreStack(
            scope,
            stack_name,
            project_name=project_name,
            feature_name=feature_name,
            description=f'Contains resources for the AWS Core Gem stack as part of the {project_name} project',
            tags={Constants.O3DE_PROJECT_TAG_NAME: project_name, Constants.O3DE_FEATURE_TAG_NAME: feature_name},
            env=env)

    @property
    def properties(self):
        return self._feature_stack.properties
