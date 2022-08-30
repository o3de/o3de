"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_iam as iam,
    aws_cognito as cognito
)
from utils import name_utils


class CognitoIdentityPoolRole:
    """
     Creates iam role with permissions to Cognito identity pool to get authenticated or unauthenticated credentials
     for associated managed policy. Policy statement can be added to the managed policy by other gem cdk stack to add
     their custom permissions.
    """

    def __init__(self, scope: core.Construct, feature_name: str, project_name: str, env: core.Environment,
                 identity_pool: cognito.CfnIdentityPool, authenticated: bool) -> None:
        """
        :param scope: Construct role scope will be attached to.
        :param feature_name: Name of the feature for resource.
        :param project_name: Name of the project for resource.
        :param env: Environment set up by App.
        :param identity_pool: Already created identity pool resource.
        :param authenticated: Allow only authenticated users to get credentials if True.
        """
        authenticated_string = 'authenticated' if authenticated else 'unauthenticated'

        self._role = iam.Role(scope,
                              id=name_utils.format_aws_resource_authenticated_id(feature_name, project_name, env,
                                                                                 iam.Role.__name__, authenticated),
                              role_name=name_utils.format_aws_resource_authenticated_name(feature_name, project_name,
                                                                                          env,
                                                                                          iam.Role.__name__,
                                                                                          authenticated),
                              assumed_by=iam.FederatedPrincipal('cognito-identity.amazonaws.com', conditions={
                                  'StringEquals': {
                                      'cognito-identity.amazonaws.com:aud': identity_pool.ref},
                                  'ForAnyValue:StringLike': {
                                      'cognito-identity.amazonaws.com:amr': [
                                          f'{authenticated_string}'
                                      ]
                                  }
                              }, assume_role_action='sts:AssumeRoleWithWebIdentity'))

        # The above role is created for developers to add custom permissions that they need to provide authorized
        # clients. Developers should update the policy statements below to add their required permissions.
        # As an example s3:ListBuckets permissions are provided.
        # Note: There must be at least one policy statement here.
        stack_statement = iam.PolicyStatement(
            actions=[
                's3:ListBuckets'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                f'arn:aws:s3:::{project_name}/*'
            ],
            sid=name_utils.format_aws_resource_sid(feature_name, project_name, iam.PolicyStatement.__name__)
        )

        self._managed_policy = iam.ManagedPolicy(
            self._role,
            id=name_utils.format_aws_resource_authenticated_id(feature_name, project_name, env,
                                                               iam.ManagedPolicy.__name__, authenticated),
            managed_policy_name=name_utils.format_aws_resource_authenticated_name(feature_name, project_name, env,
                                                                                  iam.ManagedPolicy.__name__,
                                                                                  authenticated),
            statements=[stack_statement])

        core.CfnOutput(
            scope,
            f'CognitoIdentityPool{authenticated_string.capitalize()}RoleArn',
            description='Cognito Identity pool Authenticated role arn',
            value=self._role.role_arn)

        core.CfnOutput(
            scope,
            f'CognitoIdentityPool{authenticated_string.capitalize()}ManagedPolicyArn',
            description="Cognito Identity pool Unauthenticated role arn",
            value=self._managed_policy.managed_policy_arn)

    def get_role(self) -> iam.Role:
        """
        :return: Created IAM role
        """
        return self._role

    def get_managed_policy(self) -> iam.ManagedPolicy:
        """
        :return: Created Managed Policy
        """
        return self._managed_policy
