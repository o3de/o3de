#!/usr/bin/env python3

"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

import os

from aws_cdk import (
    core
)

from constants import Constants
from core.aws_core import AWSCore
from example.example_resources_stack import ExampleResources

"""Configuration"""
REGION = os.environ.get('O3DE_AWS_DEPLOY_REGION', os.environ.get('CDK_DEFAULT_REGION'))
ACCOUNT = os.environ.get('O3DE_AWS_DEPLOY_ACCOUNT', os.environ.get('CDK_DEFAULT_ACCOUNT'))

# Set the common prefix to group stacks in a project together.
PROJECT_NAME = os.environ.get('O3DE_AWS_PROJECT_NAME', f'O3DE-AWS-PROJECT').upper()

# The name of this feature
FEATURE_NAME = 'AWSCore'

# The name of this CDK application
PROJECT_FEATURE_NAME = f'{PROJECT_NAME}-{FEATURE_NAME}'

"""End of Configuration"""

# Set-up regions to deploy core stack to, or use default if not set
env = core.Environment(account=ACCOUNT, region=REGION)

app = core.App()

core_construct = AWSCore(
    app,
    id_=f'{PROJECT_FEATURE_NAME}-Construct',
    project_name=PROJECT_NAME,
    feature_name=FEATURE_NAME,
    env=env
)

# Below is the Core example stack which is provided for working with AWSCore ScriptCanvas examples.
# It also provided as an example how to reference resources across stacks via stack outputs.
# See https://docs.aws.amazon.com/cdk/latest/guide/resources.html#resource_stack

example_stack = ExampleResources(
    app,
    id_=f'{PROJECT_FEATURE_NAME}-Example-{env.region}',
    project_name=f'{PROJECT_NAME}',
    feature_name=FEATURE_NAME,
    tags={Constants.O3DE_PROJECT_TAG_NAME: PROJECT_NAME, Constants.O3DE_FEATURE_TAG_NAME: FEATURE_NAME},
    env=env
)

# Add the core stack as a dependency of the feature stack since the feature stack
# requires the core stack outputs for deployment.
example_stack.add_dependency(core_construct.common_stack)

app.synth()
