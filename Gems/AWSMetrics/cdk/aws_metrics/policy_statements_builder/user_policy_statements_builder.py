"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from __future__ import annotations

from aws_cdk import (
    aws_iam as iam
)

from aws_metrics.policy_statements_builder.policy_statements_builder_interface import PolicyStatementsBuilderInterface
from aws_metrics.data_ingestion import DataIngestion


class UserPolicyStatementsBuilder(PolicyStatementsBuilderInterface):
    """
    Build the basic user policy statement list for the AWSMetrics gem
    """

    def __init__(self):
        super().__init__()

    def add_data_ingestion_policy_statements(self, component: DataIngestion) -> UserPolicyStatementsBuilder:
        """
        Add the policy statement to invoke service APIs for basic users.

        :param component: Data ingestion component created by the metrics gem.
        :return: The policy statement builder itself.
        """
        self._policy_statement_mapping['ApiGateway'] = iam.PolicyStatement(
            actions=[
                'execute-api:Invoke'
            ],
            effect=iam.Effect.ALLOW,
            resources=[component.execute_api_arn],
            sid=f'CallServiceAPI'
        )

        return self
