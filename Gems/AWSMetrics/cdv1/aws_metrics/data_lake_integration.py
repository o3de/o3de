"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_iam as iam,
    aws_s3 as s3,
    aws_glue as glue
)

from . import aws_metrics_constants
from .aws_utils import resource_name_sanitizer


class DataLakeIntegration:
    """
    Create the AWS resources including the S3 bucket, Glue database, table and crawler for data lake integration
    """
    def __init__(self, stack: core.Construct, application_name: str,
                 server_access_logs_bucket: str = None) -> None:
        self._stack = stack
        self._application_name = application_name
        self._server_access_logs_bucket = server_access_logs_bucket

        self._create_analytics_bucket()
        self._create_events_database()
        self._create_events_table()
        self._create_events_crawler()

    def _create_analytics_bucket(self) -> None:
        """
        Create a a private bucket that should only be accessed by the resources defined in the CDK application.
        The bucket uses server-side encryption with a CMK managed by S3:
        https://docs.aws.amazon.com/AmazonS3/latest/userguide/UsingKMSEncryption.html
        """
        # Enable server access logging if the server access logs bucket is provided following S3 best practices.
        # See https://docs.aws.amazon.com/AmazonS3/latest/dev/security-best-practices.html
        server_access_logs_bucket = s3.Bucket.from_bucket_name(
            self._stack,
            f'{self._stack.stack_name}-ImportedAccessLogsBucket',
            self._server_access_logs_bucket,
        ) if self._server_access_logs_bucket else None

        # Bucket name cannot contain uppercase characters
        # Do not specify the bucket name here since bucket name is required to be unique globally. If we set
        # a specific name here, only one customer can deploy the bucket successfully.
        self._analytics_bucket = s3.Bucket(
            self._stack,
            id=resource_name_sanitizer.sanitize_resource_name(
                f'{self._stack.stack_name}-AnalyticsBucket'.lower(), 's3_bucket'),
            encryption=s3.BucketEncryption.S3_MANAGED,
            block_public_access=s3.BlockPublicAccess(
                block_public_acls=True,
                block_public_policy=True,
                ignore_public_acls=True,
                restrict_public_buckets=True
            ),
            server_access_logs_bucket=server_access_logs_bucket,
            server_access_logs_prefix=f'{self._stack.stack_name}-AccessLogs' if server_access_logs_bucket else None
        )

        # For Amazon S3 buckets, you must delete all objects in the bucket for deletion to succeed.
        cfn_bucket = self._analytics_bucket.node.find_child('Resource')
        cfn_bucket.apply_removal_policy(core.RemovalPolicy.DESTROY)

        core.CfnOutput(
            self._stack,
            id='AnalyticsBucketName',
            description='Name of the S3 bucket for storing metrics event data',
            export_name=f"{self._application_name}:AnalyticsBucket",
            value=self._analytics_bucket.bucket_name)

    def _create_events_database(self) -> None:
        """
        Create the Glue database for metrics events.
        """
        # Database name cannot contain uppercase characters
        self._events_database = glue.CfnDatabase(
            self._stack,
            id='EventsDatabase',
            catalog_id=self._stack.account,
            database_input=glue.CfnDatabase.DatabaseInputProperty(
                description=f'Metrics events database for stack {self._stack.stack_name}',
                location_uri=f's3://{self._analytics_bucket.bucket_name}',
                name=f'{self._stack.stack_name}-EventsDatabase'.lower()
            )
        )
        core.CfnOutput(
            self._stack,
            id='EventDatabaseName',
            description='Glue database for metrics events.',
            export_name=f"{self._application_name}:EventsDatabase",
            value=self._events_database.ref)

    def _create_events_table(self) -> None:
        """
        Create the Glue table for metrics events. This table is used by the Kinesis Data Firehose
        to convert data from the JSON format to the Parquet format before writing it to Amazon S3.
        """
        self._events_table = glue.CfnTable(
            self._stack,
            id=f'EventsTable',
            catalog_id=self._stack.account,
            database_name=self._events_database.ref,
            table_input=glue.CfnTable.TableInputProperty(
                description=f'Stores metrics event data from the analytics pipeline for stack {self._stack.stack_name}',
                name=aws_metrics_constants.GLUE_TABLE_NAME,
                table_type='EXTERNAL_TABLE',
                partition_keys=[
                    glue.CfnTable.ColumnProperty(
                        name='year',
                        type='string'
                    ),
                    glue.CfnTable.ColumnProperty(
                        name='month',
                        type='string'
                    ),
                    glue.CfnTable.ColumnProperty(
                        name='day',
                        type='string'
                    ),
                ],
                parameters={
                    'classification': 'parquet',
                    'compressionType': 'none',
                    'typeOfData': 'file'
                },
                storage_descriptor=glue.CfnTable.StorageDescriptorProperty(
                    input_format=aws_metrics_constants.GLUE_TABLE_INPUT_FORMAT,
                    output_format=aws_metrics_constants.GLUE_TABLE_OUTPUT_FORMAT,
                    serde_info=glue.CfnTable.SerdeInfoProperty(
                        serialization_library=aws_metrics_constants.GLUE_TABLE_SERIALIZATION_LIBRARY,
                        parameters={
                            'serialization.format':
                                aws_metrics_constants.GLUE_TABLE_SERIALIZATION_LIBRARY_SERIALIZATION_FORMAT
                        }
                    ),
                    stored_as_sub_directories=False,
                    location=f's3://{self._analytics_bucket.bucket_name}/{aws_metrics_constants.GLUE_TABLE_NAME}/',
                    columns=[
                        glue.CfnTable.ColumnProperty(
                            name='event_id',
                            type='string'
                        ),
                        glue.CfnTable.ColumnProperty(
                            name='event_type',
                            type='string'
                        ),
                        glue.CfnTable.ColumnProperty(
                            name='event_name',
                            type='string'
                        ),
                        glue.CfnTable.ColumnProperty(
                            name='event_timestamp',
                            type='string'
                        ),
                        glue.CfnTable.ColumnProperty(
                            name='event_version',
                            type='string'
                        ),
                        glue.CfnTable.ColumnProperty(
                            name='event_source',
                            type='string'
                        ),
                        glue.CfnTable.ColumnProperty(
                            name='application_id',
                            type='string'
                        ),
                        glue.CfnTable.ColumnProperty(
                            name='event_data',
                            type='string'
                        )
                    ]
                )
            )
        )

    def _create_events_crawler(self) -> None:
        """
        Create the Glue crawler to populate the AWS Glue Data Catalog with tables.
        """
        self._create_events_crawler_role()

        self._events_crawler = glue.CfnCrawler(
            self._stack,
            id='EventsCrawler',
            name=f'{self._stack.stack_name}-EventsCrawler',
            role=self._events_crawler_role.role_arn,
            database_name=self._events_database.ref,
            targets=glue.CfnCrawler.TargetsProperty(
                s3_targets=[
                    glue.CfnCrawler.S3TargetProperty(
                        path=f's3://{self._analytics_bucket.bucket_name}/{aws_metrics_constants.GLUE_TABLE_NAME}/'
                    )
                ]
            ),
            schema_change_policy=glue.CfnCrawler.SchemaChangePolicyProperty(
                update_behavior='UPDATE_IN_DATABASE',
                delete_behavior='LOG',
            ),
            configuration=aws_metrics_constants.CRAWLER_CONFIGURATION
        )

        core.CfnOutput(
            self._stack,
            id='EventsCrawlerName',
            description='Glue Crawler to populate the AWS Glue Data Catalog with metrics events tables',
            export_name=f"{self._application_name}:EventsCrawler",
            value=self._events_crawler.name)

    def _create_events_crawler_role(self) -> None:
        """
        Create the IAM role for the Glue crawler.
        """
        policy_statements = list()

        s3_policy_statement = iam.PolicyStatement(
            actions=[
                's3:ListBucket',
                's3:GetObject',
                's3:PutObject',
                's3:DeleteObject'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                self._analytics_bucket.bucket_arn,
                f'{self._analytics_bucket.bucket_arn}/*'
            ]
        )
        policy_statements.append(s3_policy_statement)

        glue_table_policy_statement = iam.PolicyStatement(
            actions=[
                'glue:BatchGetPartition',
                'glue:GetPartition',
                'glue:GetPartitions',
                'glue:BatchCreatePartition',
                'glue:CreatePartition',
                'glue:CreateTable',
                'glue:GetTable',
                'glue:GetTables',
                'glue:GetTableVersion',
                'glue:GetTableVersions',
                'glue:UpdatePartition',
                'glue:UpdateTable'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub(
                    'arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:catalog'
                ),
                core.Fn.sub(
                    body='arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:table/${EventsDatabase}/*',
                    variables={
                        'EventsDatabase': self._events_database.ref
                    }
                ),
                core.Fn.sub(
                    body='arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:database/${EventsDatabase}',
                    variables={
                        'EventsDatabase': self._events_database.ref
                    }
                )
            ]
        )
        policy_statements.append(glue_table_policy_statement)

        glue_database_policy_statement = iam.PolicyStatement(
            actions=[
                'glue:GetDatabase',
                'glue:GetDatabases',
                'glue:UpdateDatabase'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub(
                    'arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:catalog'
                ),
                core.Fn.sub(
                    body='arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:database/${EventsDatabase}',
                    variables={
                        'EventsDatabase': self._events_database.ref
                    }
                )
            ]
        )
        policy_statements.append(glue_database_policy_statement)

        log_policy_statement = iam.PolicyStatement(
            actions=[
                'logs:CreateLogGroup',
                'logs:CreateLogStream',
                'logs:PutLogEvents'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub(
                    'arn:${AWS::Partition}:logs:${AWS::Region}:${AWS::AccountId}:log-group:/aws-glue/crawlers:*'
                )
            ]
        )
        policy_statements.append(log_policy_statement)

        events_crawler_policy_document = iam.PolicyDocument(
            statements=policy_statements
        )

        self._events_crawler_role = iam.Role(
            self._stack,
            id='EventsCrawlerRole',
            role_name=resource_name_sanitizer.sanitize_resource_name(
                f'{self._stack.stack_name}-EventsCrawlerRole', 'iam_role'),
            assumed_by=iam.ServicePrincipal(
                service='glue.amazonaws.com'
            ),
            inline_policies={
                'GameAnalyticsPipelineGlueCrawlerPolicy': events_crawler_policy_document
            }
        )

    @property
    def analytics_bucket_arn(self) -> s3.Bucket.bucket_arn:
        return self._analytics_bucket.bucket_arn

    @property
    def analytics_bucket_name(self) -> s3.Bucket.bucket_name:
        return self._analytics_bucket.bucket_name

    @property
    def events_database_name(self) -> str:
        return self._events_database.ref

    @property
    def events_table_name(self) -> str:
        return self._events_table.ref

    @property
    def events_crawler_name(self) -> str:
        return self._events_crawler.name

    @property
    def events_crawler_role_arn(self) -> iam.Role.role_arn:
        return self._events_crawler_role.role_arn
