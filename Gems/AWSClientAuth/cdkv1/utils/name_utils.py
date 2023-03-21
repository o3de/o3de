"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import re
from aws_cdk import core
from .resource_name_sanitizer import sanitize_resource_name


def format_aws_resource_name(feature_name: str, project_name: str, env: core.Environment, resource_type: str):
    return sanitize_resource_name(f'{project_name}-{feature_name}-{resource_type}-{env.region}', resource_type)


def format_aws_resource_id(feature_name: str, project_name: str, env: core.Environment, resource_type: str):
    return f'{project_name}{feature_name}{resource_type}Id{env.region}'


def format_aws_resource_sid(feature_name: str, project_name: str, resource_type: str):
    sid = f'{project_name}{feature_name}{resource_type}SId'
    # Strip out all chars not valid in a sid
    return re.sub(r'[^a-zA-Z0-9]', '', sid)


def format_aws_resource_authenticated_id(feature_name: str, project_name: str, env: core.Environment,
                                         resource_type: str, authenticated: bool):
    authenticated_string = 'Authenticated' if authenticated else 'Unauthenticated'
    return f'{project_name}{feature_name}{resource_type}Id{authenticated_string}-{env.region}'


def format_aws_resource_authenticated_name(feature_name: str, project_name: str, env: core.Environment,
                                           resource_type: str, authenticated: bool):
    authenticated_string = 'Authenticated' if authenticated else 'Unauthenticated'
    return sanitize_resource_name(
        f'{project_name}{feature_name}{resource_type}{authenticated_string}-{env.region}', resource_type)
