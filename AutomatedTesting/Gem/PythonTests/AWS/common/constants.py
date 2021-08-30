"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os

# ARN of the IAM role to assume for retrieving temporary AWS credentials
ASSUME_ROLE_ARN = os.environ.get('ASSUME_ROLE_ARN', 'arn:aws:iam::645075835648:role/o3de-automation-tests')
# Name of the AWS project deployed by the CDK applications
AWS_PROJECT_NAME = os.environ.get('O3DE_AWS_PROJECT_NAME', 'AWSAUTO')
# Region for the existing CloudFormation stacks used by the automation tests
AWS_REGION = os.environ.get('O3DE_AWS_DEPLOY_REGION', 'us-east-1')
# Name of the default resource mapping config file used by the automation tests
AWS_RESOURCE_MAPPING_FILE_NAME = 'default_aws_resource_mappings.json'
# Name of the game launcher log
GAME_LOG_NAME = 'Game.log'
# Name of the IAM role session for retrieving temporary AWS credentials
SESSION_NAME = 'o3de-Automation-session'
