"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# !/usr/bin/env python3

from aws_cdk import core
from aws_client_auth.client_auth_stack import AWSClientAuthStack
import os

"""Configuration"""
REGION = os.environ.get('O3DE_AWS_DEPLOY_REGION', os.environ['CDK_DEFAULT_REGION'])
ACCOUNT = os.environ.get('O3DE_AWS_DEPLOY_ACCOUNT', os.environ['CDK_DEFAULT_ACCOUNT'])

# Set the common prefix to group stacks in a project together.
PROJECT_NAME = os.environ.get('O3DE_AWS_PROJECT_NAME', f'O3DE-AWS-PROJECT').upper()

env = core.Environment(account=ACCOUNT, region=REGION)
app = core.App()

AWSClientAuthStack(app, PROJECT_NAME, env=env)

app.synth()
