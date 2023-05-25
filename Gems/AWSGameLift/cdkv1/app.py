#!/usr/bin/env python3
"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os

from aws_cdk import core

from aws_gamelift.aws_gamelift_construct import AWSGameLift

"""Configuration"""
REGION = os.environ.get('O3DE_AWS_DEPLOY_REGION', os.environ.get('CDK_DEFAULT_REGION'))
ACCOUNT = os.environ.get('O3DE_AWS_DEPLOY_ACCOUNT', os.environ.get('CDK_DEFAULT_ACCOUNT'))

# Set the common prefix to group stacks in a project together.
PROJECT_NAME = os.environ.get('O3DE_AWS_PROJECT_NAME', f'O3DE-AWS-PROJECT').upper()

# The name of this feature
FEATURE_NAME = 'AWSGameLift'

# The name of this CDK application
PROJECT_FEATURE_NAME = f'{PROJECT_NAME}-{FEATURE_NAME}'

# Standard Tag Key for project based tags
O3DE_PROJECT_TAG_NAME = 'O3DEProject'
# Standard Tag Key for feature based tags
O3DE_FEATURE_TAG_NAME = 'O3DEFeature'

"""End of Configuration"""

# Set-up regions to deploy stack to, or use default if not set
env = core.Environment(
    account=ACCOUNT,
    region=REGION)

app = core.App()
feature_struct = AWSGameLift(
    app,
    id_=PROJECT_FEATURE_NAME,
    project_name=PROJECT_NAME,
    feature_name=FEATURE_NAME,
    tags={O3DE_PROJECT_TAG_NAME: PROJECT_NAME, O3DE_FEATURE_TAG_NAME: FEATURE_NAME},
    env=env
)
app.synth()
