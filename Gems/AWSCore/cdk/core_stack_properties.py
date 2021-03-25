"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from aws_cdk import (
    core,
    aws_iam as iam
)


class CoreStackProperties(core.StackProps):
    """
    Support for cross stack references in the application.

    Define any properties from the CoreStack other stacks in this application
    may need to consume.
    """
    # Common IAM group for users
    user_group: iam.Group

    # Common IAM group for Admin users
    admin_group: iam.Group
