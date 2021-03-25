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
from .aws_metrics_stack import AWSMetricsStack
from .auth import AuthPolicy


class AWSMetrics(core.Construct):
    """
    Orchestrates setting up the AWS Metrics Stack(s)
    """

    def __init__(self,
                 scope: core.Construct,
                 id_: str,
                 project_name: str,
                 feature_name: str,
                 env: core.Environment) -> None:
        super().__init__(scope, id_)
        # Set-up any stack name(s) to be unique in account
        stack_name = f'{project_name}-{feature_name}-{env.region}'
        application_name = f'{project_name}-{feature_name}'

        # Check context variables to get enabled optional features
        optional_features = {
            'batch_processing': self.node.try_get_context("batch_processing") == 'true'
        }

        # Deploy AWS Metrics Stack
        self._feature_stack = AWSMetricsStack(
            scope,
            stack_name,
            stack_name=stack_name,
            application_name=application_name,
            description=f'Contains resources for the AWS Metrics Gem Feature stack as part of the {project_name} project',
            optional_features=optional_features,
            tags={'LyAWSProject': project_name, 'LyAWSFeature': feature_name},
            env=env)

        # Create the User and Admin polices for the AWSMetrics Gem
        policy_generator = AuthPolicy(
            self._feature_stack,
            application_name=application_name
        )
        policy_generator.generate_policy(role_name='User')
        policy_generator.generate_policy(role_name='Admin')
