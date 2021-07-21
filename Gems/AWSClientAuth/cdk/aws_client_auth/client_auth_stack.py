"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (core)
from auth.cognito_user_pool_sms_role import CognitoUserPoolSMSRole
from cognito.cognito_user_pool import CognitoUserPool
from cognito.cognito_identity_pool import CognitoIdentityPool
from utils import name_utils
from utils.constants import *

class AWSClientAuthStack(core.Stack):
    """
    Composes AWS resources required by AWSClientAuth gem to provide authentication and authorization.
    """

    def __init__(self, scope: core.Construct, project_name: str, env: core.Environment,
                 **kwargs) -> None:
        """
        :param scope: Construct role scope will be attached to.
        :param project_name: Name of the project for resource.
        :param env: Environment set up by App.
        :param kwargs: -
        """
        super().__init__(scope, id=name_utils.format_aws_resource_id(STACK_FEATURE_NAME, project_name, env,
                                                                     core.Stack.__name__),
                         stack_name=name_utils.format_aws_resource_name(STACK_FEATURE_NAME, project_name, env,
                                                                        core.Stack.__name__), env=env,
                         tags={'AWSProject': project_name, 'AWSFeature': STACK_FEATURE_NAME},
                         description=f'Deployed resources for the AWS Client Auth Gem for {project_name} project '
                                     f'in {env.region} region',
                         **kwargs)

        sms_role = CognitoUserPoolSMSRole(self, STACK_FEATURE_NAME, project_name, env)

        cognito_user_pool = CognitoUserPool(self, STACK_FEATURE_NAME, project_name, env, sms_role)

        CognitoIdentityPool(self, STACK_FEATURE_NAME, project_name, env, cognito_user_pool)
