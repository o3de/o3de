#!/usr/bin/env python3

"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import core
from aws_metrics.aws_metrics_construct import AWSMetrics

import os

"""Configuration"""
REGION = os.environ.get('O3DE_AWS_DEPLOY_REGION', os.environ['CDK_DEFAULT_REGION'])
ACCOUNT = os.environ.get('O3DE_AWS_DEPLOY_ACCOUNT', os.environ['CDK_DEFAULT_ACCOUNT'])

# Set the common prefix to group stacks in a project together. Defaults to LY-AWS<UUID>.
PROJECT_NAME = os.environ.get('O3DE_AWS_PROJECT_NAME', f'O3DE-AWS-PROJECT').upper()

# The name of this feature
FEATURE_NAME = 'AWSMetrics'

# The name of this CDK application
PROJECT_FEATURE_NAME = f'{PROJECT_NAME}-{FEATURE_NAME}'

"""End of Configuration"""

app = core.App()

# Set-up regions to deploy stack to, or use default if not set
env = core.Environment(
    account=ACCOUNT,
    region=REGION)

feature_stack = AWSMetrics(
    app,
    id_=PROJECT_FEATURE_NAME,
    project_name=PROJECT_NAME,
    feature_name=FEATURE_NAME,
    env=env)

app.synth()
