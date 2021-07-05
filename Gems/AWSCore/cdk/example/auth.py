"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_iam as iam
)


class AuthPolicy:
    """
    Creator of auth policies related for the example stack
    """
    def __init__(self, context: core.Construct):
        self._context = context
        self._policy_output = None

    def generate_user_policy(self, stack: core.Stack) -> None:
        """
        Generate require role policy for calling resources created in the stack.

        Currently all resources use grant_access to groups so no direct policy
        is generated.

        :param stack: The stack to use to generate the policy for
        :return: The created Admin IAM managed policy.
        """
        return None

    def generate_admin_policy(self, stack: core.Stack) -> iam.ManagedPolicy:
        """
        Generate required role policy for calling service / using resources.

        :param stack: The stack to use to generate the policy for
        :return: The created Admin IAM managed policy.
        """
        policy_id = f'CoreExampleAdminPolicy'

        policy_statements = []

        # Add permissions to describe stacks and resources
        stack_statement = iam.PolicyStatement(
            actions=[
                "cloudformation:DescribeStackResources",
                "cloudformation:DescribeStackResource",
                "cloudformation:ListStackResources"
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                f"arn:{stack.partition}:cloudformation:{stack.region}:{stack.account}:stack/{stack.stack_name}"
            ],
            sid="ReadDeploymentStacks",
        )
        policy_statements.append(stack_statement)

        policy = iam.ManagedPolicy(
            self._context,
            policy_id,
            managed_policy_name=f'{stack.stack_name}-AdminPolicy',
            statements=policy_statements)

        self._policy_output = core.CfnOutput(
            self._context,
            id=f'{policy_id}AdminOutput',
            description='Admin user policy arn to work with resources',
            export_name=f"{stack.stack_name}:{policy_id}",
            value=policy.managed_policy_arn)
        return policy
