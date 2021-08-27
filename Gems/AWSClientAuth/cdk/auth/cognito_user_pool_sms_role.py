"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_iam as iam
)
from utils import name_utils


class CognitoUserPoolSMSRole:
    """
    Class to create IAM Role to allow Cognito user pool to send sms for SignUp and MFA
    """

    def __init__(self, scope: core.Construct, feature_name: str, project_name: str, env: core.Environment) -> None:
        """
        :param feature_name: Name of the feature for resource.
        :param project_name: Name of the project for resource.
        :param env: Environment set up by App.
        """
        self._sms_role = iam.Role(
            scope,
            name_utils.format_aws_resource_id(feature_name, project_name, env, iam.Role.__name__),
            description='Role permissions used by Cognito user pool to send sms',
            assumed_by=iam.ServicePrincipal("cognito-idp.amazonaws.com"),
            # Deny all others and then allow only for the current sms role.
            inline_policies={
                'SNSRoleInlinePolicy':
                    iam.PolicyDocument(
                        statements=[
                            # SMS role will be used by CognitoIDP tp allow to publish to SNS topic owned by CognitoIDP
                            # team to push a sms.
                            # Need to use * as the resource name used by CognitoIDP principal service is unknown.
                            iam.PolicyStatement(
                                effect=iam.Effect.ALLOW,
                                actions=['sns:Publish'],
                                resources=['*']
                            )
                        ]
                    )
            },
        )

        # Below does not work as add_to_policy creates a parallel CF construct.
        # self._sms_role.add_to_policy(policy_statement)

        output = core.CfnOutput(
            scope,
            'CognitoUserPoolSMSRoleArn',
            description="Cognito User pool SMS role Arn",
            value=self._sms_role.role_arn)

    def get_role(self) -> iam.Role:
        """
        :return: Returns created IAM SMS role
        """
        return self._sms_role
