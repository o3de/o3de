"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os

from aws_cdk import (
    aws_lambda as lambda_,
    aws_s3 as s3,
    aws_s3_deployment as s3_deployment,
    aws_dynamodb as dynamo,
    core
)

from core_stack_properties import CoreStackProperties
from .auth import AuthPolicy


class ExampleResources(core.Stack):
    """
    Defines a set of resources to use with AWSCore's ScriptBehaviours and examples. The example resources are:
    * An S3 bucket with a text file
    * A python 'echo' lambda
    * A small dynamodb table with the a primary 'id': str key
    """
    def __init__(self, scope: core.Construct, id_: str, project_name: str, feature_name: str,
                 props_: CoreStackProperties, **kwargs) -> None:
        super().__init__(scope, id_, **kwargs,
                         description=f'Contains resources for the AWSCore examples as part of the '
                                     f'{project_name} project')

        self._project_name = project_name
        self._feature_name = feature_name

        self._policy = AuthPolicy(context=self).generate_admin_policy(stack=self)
        self._s3_bucket = self.__create_s3_bucket()
        self._lambda = self.__create_example_lambda()
        self._table = self.__create_dynamodb_table()

        self.__create_outputs()

        # Finally grant cross stack references
        self.__grant_access(props=props_)

    def __grant_access(self, props: CoreStackProperties):
        self._s3_bucket.grant_read(props.user_group)
        self._s3_bucket.grant_read(props.admin_group)

        self._lambda.grant_invoke(props.user_group)
        self._lambda.grant_invoke(props.admin_group)

        self._table.grant_read_data(props.user_group)
        self._table.grant_read_data(props.admin_group)

    def __create_s3_bucket(self) -> s3.Bucket:
        # Create a sample S3 bucket following S3 best practices
        # # See https://docs.aws.amazon.com/AmazonS3/latest/dev/security-best-practices.html
        # 1. Block all public access to the bucket
        # 2. Use SSE-S3 encryption. Explore encryption at rest options via
        #    https://docs.aws.amazon.com/AmazonS3/latest/userguide/serv-side-encryption.html
        example_bucket = s3.Bucket(
            self,
            f'{self._project_name}-{self._feature_name}-Example-S3bucket',
            block_public_access=s3.BlockPublicAccess.BLOCK_ALL,
            encryption=s3.BucketEncryption.S3_MANAGED
        )

        s3_deployment.BucketDeployment(
            self,
            f'{self._project_name}-{self._feature_name}-S3bucket-Deployment',
            destination_bucket=example_bucket,
            sources=[
                s3_deployment.Source.asset('example/s3_content')
            ],
            retain_on_delete=False
        )
        return example_bucket

    def __create_example_lambda(self) -> lambda_.Function:
        # create lambda function
        function = lambda_.Function(
            self,
            f'{self._project_name}-{self._feature_name}-Lambda-Function',
            runtime=lambda_.Runtime.PYTHON_3_8,
            handler="lambda-handler.main",
            code=lambda_.Code.asset(os.path.join(os.path.dirname(__file__), 'lambda'))
        )
        return function

    def __create_dynamodb_table(self) -> dynamo.Table:
        # create dynamo table
        # NB: CDK does not support seeding data, see simple table_seeder.py
        demo_table = dynamo.Table(
            self,
            f'{self._project_name}-{self._feature_name}-Table',
            partition_key=dynamo.Attribute(
                name="id",
                type=dynamo.AttributeType.STRING
            )
        )
        return demo_table

    def __create_outputs(self) -> None:
        # Define exports
        # Export resource group
        self._s3_output = core.CfnOutput(
            self,
            id=f'ExampleBucketOutput',
            description='An example S3 bucket to use with AWSCore ScriptBehaviors',
            export_name=f"ExampleS3Bucket",
            value=self._s3_bucket.bucket_arn)

        # Define exports
        # Export resource group
        self._lambda_output = core.CfnOutput(
            self,
            id=f'ExampleLambdaOutput',
            description='An example Lambda to use with AWSCore ScriptBehaviors',
            export_name=f"ExampleLambdaFunction",
            value=self._lambda.function_arn)

        # Export DynamoDB Table
        self._table_output = core.CfnOutput(
            self,
            id=f'ExampleDynamoTableOutput',
            description='An example DynamoDB Table to use with AWSCore ScriptBehaviors',
            export_name=f"ExampleTable",
            value=self._table.table_arn)

        # Export user policy
        self._user_policy = core.CfnOutput(
            self,
            id=f'ExampleUserPolicyOutput',
            description='A User policy to invoke example resources',
            export_name=f"ExampleUserPolicy",
            value=self._policy.managed_policy_arn)
