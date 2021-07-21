"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (core,
                     aws_cognito as cognito,
                     aws_iam as iam)
from utils import name_utils
from auth.cognito_user_pool_sms_role import CognitoUserPoolSMSRole


class CognitoUserPool:
    """
    Creates User pool. Sets up MFA with text. Allows enabling MFA. Allows signing up by email and phone.
    """

    def __init__(self, scope: core.Construct, feature_name: str, project_name: str, env: core.Environment,
                 sms_role: CognitoUserPoolSMSRole) -> None:
        """
        :param scope: Construct role scope will be attached to.
        :param feature_name: Name of the feature for resource.
        :param project_name: Name of the project for resource.
        :param env: Environment set up by App.
        :param sms_role: SMS IAM role created using
        """
        self._user_pool = cognito.CfnUserPool(scope, name_utils.format_aws_resource_id(feature_name, project_name, env,
                                                                                       cognito.CfnUserPool.__name__),
                                              user_pool_name=name_utils.format_aws_resource_name(feature_name,
                                                                                                 project_name, env,
                                                                                                 cognito.CfnUserPool.__name__),
                                              admin_create_user_config=cognito.CfnUserPool.AdminCreateUserConfigProperty(
                                                  allow_admin_create_user_only=False),
                                              account_recovery_setting=cognito.CfnUserPool.AccountRecoverySettingProperty(
                                                  recovery_mechanisms=[cognito.CfnUserPool.RecoveryOptionProperty(
                                                      name='verified_email', priority=1),
                                                      cognito.CfnUserPool.RecoveryOptionProperty(
                                                          name='verified_phone_number', priority=2)]),
                                              auto_verified_attributes=['email', 'phone_number'],
                                              enabled_mfas=['SMS_MFA'],
                                              mfa_configuration='OPTIONAL',
                                              sms_configuration=cognito.CfnUserPool.SmsConfigurationProperty(
                                                  external_id=name_utils.format_aws_resource_name(feature_name,
                                                                                                  project_name, env,
                                                                                                  cognito.CfnUserPool.__name__) + '-external',
                                                  sns_caller_arn=sms_role.get_role().role_arn))

        self._user_pool.node.add_dependency(sms_role.get_role())
        self._user_pool_client = cognito.CfnUserPoolClient(scope,
                                                           name_utils.format_aws_resource_id(feature_name, project_name,
                                                                                             env,
                                                                                             cognito.CfnUserPoolClient.__name__),
                                                           client_name=name_utils.format_aws_resource_name(feature_name,
                                                                                                           project_name,
                                                                                                           env,
                                                                                                           cognito.CfnUserPoolClient.__name__),
                                                           user_pool_id=self._user_pool.ref,
                                                           explicit_auth_flows=['ALLOW_ADMIN_USER_PASSWORD_AUTH',
                                                                                'ALLOW_CUSTOM_AUTH',
                                                                                'ALLOW_USER_PASSWORD_AUTH',
                                                                                'ALLOW_USER_SRP_AUTH',
                                                                                'ALLOW_REFRESH_TOKEN_AUTH'],
                                                           # access_token_validity=5, # Does not work
                                                           # id_token_validity=5,  # Does not work
                                                           # refresh_token_validity=30,  # Does not work
                                                           )
        self._user_pool_client.add_depends_on(self._user_pool)

        core.CfnOutput(
            scope,
            'CognitoUserPoolId',
            description="Cognito User pool id",
            value=self._user_pool.ref)

        core.CfnOutput(
            scope,
            'CognitoUserPoolAppClientId',
            description="Cognito User pool App client id",
            value=self._user_pool_client.ref)

    def get_user_pool(self) -> cognito.CfnUserPool:
        """
        :return: Cognito user pool
        """
        return self._user_pool

    def get_user_pool_client(self) -> cognito.CfnUserPoolClient:
        """
        :return: Cognito user pool client.
        """
        return self._user_pool_client

    def create_using_higher_construct(self):
        raise NotImplemented('Add implementation')
        # # Below does not work as Creating App client without refresh tokens auth flow param is not accepted by CF.
        # # Create Cognito user pool
        # user_pool = cognito.UserPool(scope,
        #                                  id=name_utils.format_aws_resource_id(stack_feature_name, project_name, env,
        #                                                                       cognito.UserPool.__name__),
        #                                  user_pool_name=name_utils.format_aws_resource_name(stack_feature_name,
        #                                                                                     project_name, env,
        #                                                                                     cognito.UserPool.__name__),
        #                                  mfa=cognito.Mfa.OPTIONAL,
        #                                  mfa_second_factor=cognito.MfaSecondFactor(otp=False, sms=True),
        #                                  enable_sms_role=True,
        #                                  sms_role=sms_role.get_role(),
        #                                  sms_role_external_id='c87467be-4f34-11ea-b77f-2e728ce88125',
        #                                  self_sign_up_enabled=True)
        #
        # user_pool_client = user_pool.add_client(
        #     name_utils.format_aws_resource_id(stack_feature_name, project_name, env,
        #                                          cognito.UserPoolClient.__name__),
        #     # access_token_validity=core.Duration.minutes(6),
        #     auth_flows=cognito.AuthFlow(admin_user_password=True),
        #     # id_token_validity=core.Duration.minutes(6),
        #     user_pool_client_name=name_utils.format_aws_resource_name(stack_feature_name, project_name, env,
        #                                                               cognito.UserPoolClient.__name__),
        #     disable_o_auth=True)
