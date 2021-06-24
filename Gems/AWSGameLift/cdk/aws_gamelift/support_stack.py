"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

from aws_cdk import aws_iam as iam
from aws_cdk import aws_s3_assets as assets
from aws_cdk import core


class SupportStack(core.Stack):
    """
    The support stack

    Defines AWS resources that help to create GameLift builds from local files
    """
    def __init__(self, scope: core.Construct, id_: str,
                 stack_name: str, fleet_configurations: dict, **kwargs) -> None:
        super().__init__(scope, id_, **kwargs)
        self._stack_name = stack_name
        self._support_iam_role = self._create_support_iam_role()

        for index in range(len(fleet_configurations)):
            # Update the fleet configuration to include the corresponding build id
            fleet_configurations[index]['build_configuration']['storage_location'] = self._upload_build_asset(
                fleet_configurations[index].get('build_configuration', {}), index)

    def _create_support_iam_role(self) -> iam.Role:
        """
        Create an IAM role for GameLift to read build files stored in S3.
        :return: Generated IAM role.
        """
        support_role = iam.Role(
            self,
            id=f'{self._stack_name}-SupportRole',
            assumed_by=iam.ServicePrincipal(
                service='gamelift.amazonaws.com'
            )
        )

        return support_role

    def _upload_build_asset(self, build_configuration: dict, identifier: int) -> dict:
        """
        Upload the local build files to S3 for a creating GameLift build.
        :param build_configuration: Configuration of the GameLift build.
        :param identifier: Unique identifier of the asset which will be included in the resource id.
        :return: Storage location of the S3 object.
        """
        build_asset = assets.Asset(
            self,
            id=f'{self._stack_name}-Asset{identifier}',
            path=build_configuration.get('build_path')
        )
        # Grant the support IAM role permission to read the asset
        build_asset.grant_read(self._support_iam_role)

        storage_location = {
            'bucket': build_asset.s3_bucket_name,
            'key': build_asset.s3_object_key,
            'role_arn': self._support_iam_role.role_arn
        }

        return storage_location

