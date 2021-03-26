"""
 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from aws_cdk import core


def format_aws_resource_name(feature_name: str, project_name: str, env: core.Environment, resource_type: str):
    return f'{project_name}-{feature_name}-{resource_type}-{env.region}'


def format_aws_resource_id(feature_name: str, project_name: str, env: core.Environment, resource_type: str):
    return f'{project_name}{feature_name}{resource_type}Id{env.region}'


def format_aws_resource_sid(feature_name: str, project_name: str, resource_type: str):
    return f'{project_name}{feature_name}{resource_type}SId'


def format_aws_resource_authenticated_id(feature_name: str, project_name: str, env: core.Environment,
                                         resource_type: str, authenticated: bool):
    authenticated_string = 'Authenticated' if authenticated else 'Unauthenticated'
    return f'{project_name}{feature_name}{resource_type}Id{authenticated_string}-{env.region}'


def format_aws_resource_authenticated_name(feature_name: str, project_name: str, env: core.Environment,
                                           resource_type: str, authenticated: bool):
    authenticated_string = 'Authenticated' if authenticated else 'Unauthenticated'
    return f'{project_name}{feature_name}{resource_type}{authenticated_string}-{env.region}'
