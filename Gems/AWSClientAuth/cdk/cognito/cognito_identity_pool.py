"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (core,
                     aws_cognito as cognito,
                     aws_iam as iam)
from utils import name_utils
from cognito.cognito_user_pool import CognitoUserPool
from auth.cognito_identity_pool_role import CognitoIdentityPoolRole
from utils.constants import *


class CognitoIdentityPool:
    """
    Create Identity pool. Allow authenticated and unauthenticated flow. Add authenticated authorization using
    Cognito user pool, login with Amazon and Google 3rd party provider support.
    """

    def __init__(self, scope: core.Construct, feature_name: str, project_name: str, env: core.Environment,
                 cognito_user_pool: CognitoUserPool) -> None:
        """
        :param scope: Construct role scope will be attached to.
        :param feature_name: Name of the feature for resource.
        :param project_name: Name of the project for resource.
        :param env: Environment set up by App.
        :param cognito_user_pool: User pool to allow authenticated users from.
        """

        supported_login_providers = {} if LOGIN_WITH_AMAZON_APP_CLIENT_ID or GOOGLE_APP_CLIENT_ID else None
        if LOGIN_WITH_AMAZON_APP_CLIENT_ID:
            supported_login_providers['www.amazon.com'] = LOGIN_WITH_AMAZON_APP_CLIENT_ID
        if GOOGLE_APP_CLIENT_ID:
            supported_login_providers['accounts.google.com'] = GOOGLE_APP_CLIENT_ID

        self._identity_pool = cognito.CfnIdentityPool(scope, id=name_utils.format_aws_resource_id(feature_name,
                                                                                                  project_name, env,
                                                                                                  cognito.CfnIdentityPool.__name__),
                                                      identity_pool_name=name_utils.format_aws_resource_name(
                                                          feature_name, project_name, env,
                                                          cognito.CfnIdentityPool.__name__),
                                                      allow_unauthenticated_identities=True,
                                                      allow_classic_flow=True,
                                                      cognito_identity_providers=[
                                                          cognito.CfnIdentityPool.CognitoIdentityProviderProperty(
                                                              client_id=cognito_user_pool.get_user_pool_client().ref,
                                                              provider_name=cognito_user_pool.get_user_pool().attr_provider_name)
                                                      ],
                                                      supported_login_providers=supported_login_providers)

        self._identity_pool.add_depends_on(cognito_user_pool.get_user_pool())
        self._identity_pool.add_depends_on(cognito_user_pool.get_user_pool_client())

        # Create roles to associate with Cognito Identity pool
        self._auth_role = CognitoIdentityPoolRole(scope, feature_name, project_name, env,
                                                  self._identity_pool, authenticated=True)
        self._unauth_role = CognitoIdentityPoolRole(scope, feature_name, project_name, env,
                                                    self._identity_pool, authenticated=False)

        self._auth_role.get_role().node.add_dependency(self._identity_pool)
        self._unauth_role.get_role().node.add_dependency(self._identity_pool)

        # Attach roles to Cognito Identity pool
        cognito.CfnIdentityPoolRoleAttachment(scope,
                                              id=name_utils.format_aws_resource_id(feature_name, project_name,
                                                                                   env,
                                                                                   cognito.CfnIdentityPoolRoleAttachment.__name__),
                                              identity_pool_id=self._identity_pool.ref,
                                              roles={
                                                  'authenticated': self._auth_role.get_role().role_arn,
                                                  'unauthenticated': self._unauth_role.get_role().role_arn
                                              })

        core.CfnOutput(
            scope,
            'CognitoIdentityPoolId',
            description="Cognito Identity pool id",
            value=self._identity_pool.ref)

    def get_authenticated_role(self) -> iam.Role:
        """
        :return: Created Authenticated IAM role
        """
        return self._auth_role

    def get_unauthenticated_role(self) -> iam.Role:
        """
        :return: Created Unauthenticated IAM role
        """
        return self._unauth_role

    def get_identity_pool(self) -> cognito.CfnIdentityPool:
        """
        :return: Created identity pool
        """
        return self._identity_pool
