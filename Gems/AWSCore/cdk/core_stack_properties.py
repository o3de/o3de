"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
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
