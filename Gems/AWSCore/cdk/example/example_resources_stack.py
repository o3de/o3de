"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
import os

from aws_cdk import (
    aws_lambda as lambda_,
    aws_iam as iam,
    aws_s3 as s3,
    aws_s3_deployment as s3_deployment,
    aws_dynamodb as dynamo,
    core
)

from .auth import AuthPolicy


class ExampleResources(core.Stack):
    """
    Defines a set of resources to use with AWSCore's ScriptBehaviours and examples. The example resources are:
    * An S3 bucket with a text file
    * A python 'echo' lambda
    * A small dynamodb table with the a primary 'id': str key
    """
    def __init__(self, scope: core.Construct, id_: str, project_name: str, feature_name: str, **kwargs) -> None:
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
        self.__grant_access()

    def __grant_access(self):
        user_group = iam.Group.from_group_arn(
            self,
            f'{self._project_name}-{self._feature_name}-ImportedUserGroup',
            core.Fn.import_value(f'{self._project_name}:UserGroup')
        )
        admin_group = iam.Group.from_group_arn(
            self,
            f'{self._project_name}-{self._feature_name}-ImportedAdminGroup',
            core.Fn.import_value(f'{self._project_name}:AdminGroup')
        )

        # Provide the admin and user groups permissions to read the example S3 bucket.
        # Cannot use the grant_read method defined by the Bucket structure since the method tries to add to
        # the resource-based policy but the imported IAM groups (which are tokens from Fn.ImportValue) are
        # not valid principals in S3 bucket policies.
        # Check https://aws.amazon.com/premiumsupport/knowledge-center/s3-invalid-principal-in-policy-error/
        user_group.add_to_principal_policy(
            iam.PolicyStatement(
                actions=[
                    "s3:GetBucket*",
                    "s3:GetObject*",
                    "s3:List*"
                ],
                effect=iam.Effect.ALLOW,
                resources=[self._s3_bucket.bucket_arn, f'{self._s3_bucket.bucket_arn}/*']
            )
        )
        admin_group.add_to_principal_policy(
            iam.PolicyStatement(
                actions=[
                    "s3:GetBucket*",
                    "s3:GetObject*",
                    "s3:List*"
                ],
                effect=iam.Effect.ALLOW,
                resources=[self._s3_bucket.bucket_arn, f'{self._s3_bucket.bucket_arn}/*']
            )
        )

        # Provide the admin and user groups permissions to invoke the example Lambda function.
        # Cannot use the grant_invoke method defined by the Function structure since the method tries to add to
        # the resource-based policy but the imported IAM groups (which are tokens from Fn.ImportValue) are
        # not valid principals in Lambda function policies.
        user_group.add_to_principal_policy(
            iam.PolicyStatement(
                actions=[
                    "lambda:InvokeFunction"
                ],
                effect=iam.Effect.ALLOW,
                resources=[self._lambda.function_arn]
            )
        )
        admin_group.add_to_principal_policy(
            iam.PolicyStatement(
                actions=[
                    "lambda:InvokeFunction"
                ],
                effect=iam.Effect.ALLOW,
                resources=[self._lambda.function_arn]
            )
        )

        # Provide the admin and user groups permissions to read from the DynamoDB table.
        self._table.grant_read_data(user_group)
        self._table.grant_read_data(admin_group)

    def __create_s3_bucket(self) -> s3.Bucket:
        # Create a sample S3 bucket following S3 best practices
        # # See https://docs.aws.amazon.com/AmazonS3/latest/dev/security-best-practices.html
        # 1. Block all public access to the bucket
        # 2. Use SSE-S3 encryption. Explore encryption at rest options via
        #    https://docs.aws.amazon.com/AmazonS3/latest/userguide/serv-side-encryption.html
        # 3. Enable Amazon S3 server access logging
        #    https://docs.aws.amazon.com/AmazonS3/latest/userguide/ServerLogs.html
        server_access_logs_bucket = None
        if self.node.try_get_context('disable_access_log') != 'true':
            server_access_logs_bucket = s3.Bucket.from_bucket_name(
                self,
                f'{self._project_name}-{self._feature_name}-ImportedAccessLogsBucket',
                core.Fn.import_value(f"{self._project_name}:ServerAccessLogsBucket")
            )

        example_bucket = s3.Bucket(
            self,
            f'{self._project_name}-{self._feature_name}-Example-S3bucket',
            block_public_access=s3.BlockPublicAccess.BLOCK_ALL,
            encryption=s3.BucketEncryption.S3_MANAGED,
            server_access_logs_bucket=
            server_access_logs_bucket if server_access_logs_bucket else None,
            server_access_logs_prefix=
            f'{self._project_name}-{self._feature_name}-{self.region}-AccessLogs' if server_access_logs_bucket else None
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
            description='An example S3 bucket name to use with AWSCore ScriptBehaviors',
            export_name=f"{self.stack_name}:ExampleS3Bucket",
            value=self._s3_bucket.bucket_name)

        # Define exports
        # Export resource group
        self._lambda_output = core.CfnOutput(
            self,
            id=f'ExampleLambdaOutput',
            description='An example Lambda name to use with AWSCore ScriptBehaviors',
            export_name=f"{self.stack_name}::ExampleLambdaFunction",
            value=self._lambda.function_name)

        # Export DynamoDB Table
        self._table_output = core.CfnOutput(
            self,
            id=f'ExampleDynamoTableOutput',
            description='An example DynamoDB Table name to use with AWSCore ScriptBehaviors',
            export_name=f"{self.stack_name}:ExampleTable",
            value=self._table.table_name)

        # Export user policy
        self._user_policy = core.CfnOutput(
            self,
            id=f'ExampleUserPolicyOutput',
            description='A User policy to invoke example resources',
            export_name=f"{self.stack_name}:ExampleUserPolicy",
            value=self._policy.managed_policy_arn)
