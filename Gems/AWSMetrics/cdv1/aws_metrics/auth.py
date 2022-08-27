"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_iam as iam
)

from .aws_metrics_stack import AWSMetricsStack
from aws_metrics.policy_statements_builder.user_policy_statements_builder import UserPolicyStatementsBuilder
from aws_metrics.policy_statements_builder.admin_policy_statements_builder import AdminPolicyStatementsBuilder
from .aws_utils import resource_name_sanitizer


class AuthPolicy:
    """
    Creator of auth policies related to the Feature stack
    """

    def __init__(self, stack: AWSMetricsStack, application_name: str):
        self._stack = stack
        self._application_name = application_name

    def generate_policy(self, role_name: str) -> None:
        """
        Generate required role policy for calling service / using resources.

        See https://docs.aws.amazon.com/apigateway/latest/developerguide/api-gateway-control-access-using-iam-policies-to-create-and-manage-api.html
        :param role_name: Role to create the managed user policy for
        :return: The created IAM managed policy.
        """
        if role_name == 'User':
            policy_statements_builder = UserPolicyStatementsBuilder()
        elif role_name == 'Admin':
            policy_statements_builder = AdminPolicyStatementsBuilder()
        else:
            raise Exception('Role name needs to be either User or Admin')

        policy_id = f'{role_name}Policy'

        policy_statements_builder = policy_statements_builder\
            .add_aws_metrics_stack_policy_statements(self._stack)\
            .add_data_ingestion_policy_statements(self._stack.data_ingestion_component)\
            .add_real_time_data_processing_policy_statements(self._stack.real_time_data_processing_component)\
            .add_dashboard_policy_statements(self._stack.dashboard_component)

        # Add policy statements for the optional batch processing feature
        policy_statements_builder = policy_statements_builder\
            .add_data_lake_integration_policy_statements(self._stack.data_lake_integration_component) \
            .add_batch_processing_policy_statements(self._stack.batch_processing_component)\
            .add_batch_analytics_policy_statements(self._stack.batch_analytics_component)

        policy_statements = policy_statements_builder.build()

        policy = iam.ManagedPolicy(
            self._stack,
            policy_id,
            managed_policy_name=resource_name_sanitizer.sanitize_resource_name(
                f'{self._stack.stack_name}-{role_name}Policy', 'iam_managed_policy'),
            statements=policy_statements)

        policy_output = core.CfnOutput(
            self._stack,
            id=f'{policy_id}Output',
            description=f'{role_name} policy arn to call service',
            export_name=f'{self._application_name}:{policy_id}',
            value=policy.managed_policy_arn)
