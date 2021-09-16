"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_kinesisfirehose as kinesisfirehose,
    aws_iam as iam,
    aws_lambda as lambda_,
    aws_logs as logs
)

import os

from . import aws_metrics_constants
from .aws_utils import resource_name_sanitizer


class BatchProcessing:
    """
    Create the AWS resources including the events processing Lambda and
    the Kinesis Data Firehose delivery stream for batch processing.
    """
    def __init__(self,
                 stack: core.Construct,
                 application_name: str,
                 input_stream_arn: str,
                 analytics_bucket_arn: str,
                 events_database_name: str,
                 events_table_name) -> None:
        self._stack = stack
        self._application_name = application_name
        self._input_stream_arn = input_stream_arn
        self._analytics_bucket_arn = analytics_bucket_arn
        self._events_database_name = events_database_name
        self._events_table_name = events_table_name

        self._create_events_processing_lambda()
        self._create_events_firehose_delivery_stream()

    def _create_events_processing_lambda(self) -> None:
        """
        Generate the events processing lambda to filter the invalid metrics events.
        """
        events_processing_lambda_name = resource_name_sanitizer.sanitize_resource_name(
            f'{self._stack.stack_name}-EventsProcessingLambda', 'lambda_function')
        self._create_events_processing_lambda_role(events_processing_lambda_name)

        self._events_processing_lambda = lambda_.Function(
            self._stack,
            id='EventsProcessingLambda',
            function_name=events_processing_lambda_name,
            log_retention=logs.RetentionDays.ONE_MONTH,
            memory_size=aws_metrics_constants.LAMBDA_MEMORY_SIZE_IN_MB,
            runtime=lambda_.Runtime.PYTHON_3_7,
            timeout=core.Duration.minutes(aws_metrics_constants.LAMBDA_TIMEOUT_IN_MINUTES),
            handler='events_processing.lambda_handler',
            code=lambda_.Code.from_asset(
                    os.path.join(os.path.dirname(__file__), 'lambdas', 'events_processing_lambda')),
            role=self._events_processing_lambda_role
        )
        core.CfnOutput(
            self._stack,
            id='EventProcessingLambdaName',
            description='Lambda function for processing metrics events data.',
            export_name=f"{self._application_name}:EventProcessingLambda",
            value=self._events_processing_lambda.function_name)

    def _create_events_processing_lambda_role(self, function_name: str) -> None:
        """
        Generate the IAM role for the events processing Lambda.
        """
        events_processing_lambda_policy_document = iam.PolicyDocument(
            statements=[
                iam.PolicyStatement(
                    actions=[
                        'logs:CreateLogGroup',
                        'logs:CreateLogStream',
                        'logs:PutDestination',
                        'logs:PutLogEvents'
                    ],
                    effect=iam.Effect.ALLOW,
                    resources=[
                        core.Fn.sub(
                            'arn:${AWS::Partition}:logs:${AWS::Region}:${AWS::AccountId}:log-group:'
                            '/aws/lambda/${FunctionName}*',
                            variables={
                                'FunctionName': function_name
                            }
                        )
                    ]
                )
            ]
        )

        self._events_processing_lambda_role = iam.Role(
            self._stack,
            id='EventsProcessingLambdaRole',
            role_name=resource_name_sanitizer.sanitize_resource_name(
                f'{self._stack.stack_name}-EventsProcessingLambdaRole', 'iam_role'),
            assumed_by=iam.ServicePrincipal(
                service='lambda.amazonaws.com'
            ),
            inline_policies={
                'EventsProcessingLambdaPolicy': events_processing_lambda_policy_document
            }
        )

    def _create_events_firehose_delivery_stream(self) -> None:
        """
        Generate the Kinesis Data Firehose delivery stream to convert input data and deliver them to the data lake.
        """
        self._create_firehose_s3_delivery_log_option()
        self._create_data_firehose_role()

        self._events_firehose_delivery_stream = kinesisfirehose.CfnDeliveryStream(
            self._stack,
            id=f'EventsFirehoseDeliveryStream',
            delivery_stream_type='KinesisStreamAsSource',
            delivery_stream_name=resource_name_sanitizer.sanitize_resource_name(
                f'{self._stack.stack_name}-EventsFirehoseDeliveryStream', 'firehose_delivery_stream'),
            kinesis_stream_source_configuration=kinesisfirehose.CfnDeliveryStream.KinesisStreamSourceConfigurationProperty(
                kinesis_stream_arn=self._input_stream_arn,
                role_arn=self._firehose_delivery_stream_role.role_arn
            ),

            extended_s3_destination_configuration=kinesisfirehose.CfnDeliveryStream.ExtendedS3DestinationConfigurationProperty(
                bucket_arn=self._analytics_bucket_arn,
                buffering_hints=kinesisfirehose.CfnDeliveryStream.BufferingHintsProperty(
                    interval_in_seconds=aws_metrics_constants.DELIVERY_STREAM_BUFFER_HINTS_INTERVAL_IN_SECONDS,
                    size_in_m_bs=aws_metrics_constants.DELIVERY_STREAM_BUFFER_HINTS_SIZE_IN_MBS
                ),
                prefix=aws_metrics_constants.S3_DESTINATION_PREFIX,
                error_output_prefix=aws_metrics_constants.S3_DESTINATION_ERROR_OUTPUT_PREFIX,
                role_arn=self._firehose_delivery_stream_role.role_arn,
                compression_format=aws_metrics_constants.S3_COMPRESSION_FORMAT,
                cloud_watch_logging_options=kinesisfirehose.CfnDeliveryStream.CloudWatchLoggingOptionsProperty(
                    enabled=True,
                    log_group_name=self._firehose_delivery_stream_log_group.log_group_name,
                    log_stream_name=self._firehose_s3_delivery_log_stream.log_stream_name
                ),
                processing_configuration=kinesisfirehose.CfnDeliveryStream.ProcessingConfigurationProperty(
                    enabled=True,
                    processors=[
                        kinesisfirehose.CfnDeliveryStream.ProcessorProperty(
                            type='Lambda',
                            parameters=[
                                kinesisfirehose.CfnDeliveryStream.ProcessorParameterProperty(
                                    parameter_name='LambdaArn',
                                    parameter_value=self._events_processing_lambda.function_arn
                                ),
                                kinesisfirehose.CfnDeliveryStream.ProcessorParameterProperty(
                                    parameter_name='BufferIntervalInSeconds',
                                    parameter_value=aws_metrics_constants.PROCESSOR_BUFFER_INTERVAL_IN_SECONDS
                                ),
                                kinesisfirehose.CfnDeliveryStream.ProcessorParameterProperty(
                                    parameter_name='BufferSizeInMBs',
                                    parameter_value=aws_metrics_constants.PROCESSOR_BUFFER_SIZE_IN_MBS
                                ),
                                kinesisfirehose.CfnDeliveryStream.ProcessorParameterProperty(
                                    parameter_name='NumberOfRetries',
                                    parameter_value=aws_metrics_constants.PROCESSOR_BUFFER_NUM_OF_RETRIES
                                )
                            ]
                        )
                    ]
                ),
                data_format_conversion_configuration=kinesisfirehose.CfnDeliveryStream.DataFormatConversionConfigurationProperty(
                    enabled=True,
                    input_format_configuration=kinesisfirehose.CfnDeliveryStream.InputFormatConfigurationProperty(
                        deserializer=kinesisfirehose.CfnDeliveryStream.DeserializerProperty(
                            open_x_json_ser_de=kinesisfirehose.CfnDeliveryStream.OpenXJsonSerDeProperty(
                                case_insensitive=True,
                                convert_dots_in_json_keys_to_underscores=False
                            )
                        )
                    ),
                    output_format_configuration=kinesisfirehose.CfnDeliveryStream.OutputFormatConfigurationProperty(
                        serializer=kinesisfirehose.CfnDeliveryStream.SerializerProperty(
                            parquet_ser_de=kinesisfirehose.CfnDeliveryStream.ParquetSerDeProperty(
                                compression=aws_metrics_constants.PARQUET_SER_DE_COMPRESSION
                            )
                        )
                    ),
                    schema_configuration=kinesisfirehose.CfnDeliveryStream.SchemaConfigurationProperty(
                        catalog_id=self._stack.account,
                        role_arn=self._firehose_delivery_stream_role.role_arn,
                        database_name=self._events_database_name,
                        table_name=self._events_table_name,
                        region=self._stack.region,
                        version_id='LATEST'
                    )
                )
            )
        )

    def _create_firehose_s3_delivery_log_option(self) -> None:
        """
        Generated the CloudWatch log group and log stream that Kinesis Data Firehose
        uses for the delivery stream.
        """
        self._firehose_delivery_stream_log_group = logs.LogGroup(
            self._stack,
            id='FirehoseLogGroup',
            log_group_name=resource_name_sanitizer.sanitize_resource_name(
                f'{self._stack.stack_name}-FirehoseLogGroup', 'cloudwatch_log_group'),
            removal_policy=core.RemovalPolicy.DESTROY,
            retention=logs.RetentionDays.ONE_MONTH
        )

        self._firehose_s3_delivery_log_stream = logs.LogStream(
            self._stack,
            id='FirehoseS3DeliveryLogStream',
            log_group=self._firehose_delivery_stream_log_group,
            log_stream_name=f'{self._stack.stack_name}-FirehoseS3DeliveryLogStream',
            removal_policy=core.RemovalPolicy.DESTROY
        )

    def _create_data_firehose_role(self) -> None:
        """
        Generated IAM role for the Kinesis Data Firehose delivery stream.
        """
        policy_statements = list()

        data_lake_policy_statement = iam.PolicyStatement(
            actions=[
                's3:AbortMultipartUpload',
                's3:GetBucketLocation',
                's3:GetObject',
                's3:ListBucket',
                's3:ListBucketMultipartUploads',
                's3:PutObject'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                self._analytics_bucket_arn,
                f'{self._analytics_bucket_arn}/*'
            ]
        )
        policy_statements.append(data_lake_policy_statement)

        events_processing_lambda_policy_statement = iam.PolicyStatement(
            actions=[
                'lambda:InvokeFunction',
                'lambda:GetFunctionConfiguration',
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                self._events_processing_lambda.function_arn
            ]
        )
        policy_statements.append(events_processing_lambda_policy_statement)

        input_stream_policy_statement = iam.PolicyStatement(
            actions=[
                'kinesis:DescribeStream',
                'kinesis:GetShardIterator',
                'kinesis:GetRecords',
                'kinesis:ListShards'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                self._input_stream_arn
            ]
        )
        policy_statements.append(input_stream_policy_statement)

        log_policy_statement = iam.PolicyStatement(
            actions=[
                'logs:PutLogEvents',
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                self._firehose_delivery_stream_log_group.log_group_arn
            ]
        )
        policy_statements.append(log_policy_statement)

        data_catalog_policy_statement = iam.PolicyStatement(
            actions=[
                'glue:GetTable',
                'glue:GetTableVersion',
                'glue:GetTableVersions'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub(
                    'arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:catalog'
                ),
                core.Fn.sub(
                    body='arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:table/${EventsDatabase}/*',
                    variables={
                        'EventsDatabase': self._events_database_name
                    }
                ),
                core.Fn.sub(
                    body='arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:database/${EventsDatabase}',
                    variables={
                        'EventsDatabase': self._events_database_name
                    }
                )
            ]
        )
        policy_statements.append(data_catalog_policy_statement)

        firehose_delivery_policy = iam.PolicyDocument(
            statements=policy_statements
        )

        self._firehose_delivery_stream_role = iam.Role(
            self._stack,
            id='GameEventsFirehoseRole',
            role_name=resource_name_sanitizer.sanitize_resource_name(
                f'{self._stack.stack_name}-GameEventsFirehoseRole', 'iam_role'),
            assumed_by=iam.ServicePrincipal(
                service='firehose.amazonaws.com'
            ),
            inline_policies={
                'FirehoseDelivery': firehose_delivery_policy
            }
        )

    @property
    def events_processing_lambda_name(self) -> lambda_.Function.function_name:
        return self._events_processing_lambda.function_name

    @property
    def events_processing_lambda_arn(self) -> lambda_.Function.function_name:
        return self._events_processing_lambda.function_arn

    @property
    def events_processing_lambda_role_arn(self) -> iam.Role.role_arn:
        return self._events_processing_lambda_role.role_arn

    @property
    def delivery_stream_name(self) -> kinesisfirehose.CfnDeliveryStream.delivery_stream_name:
        return self._events_firehose_delivery_stream.ref

    @property
    def delivery_stream_role_arn(self) -> iam.Role.role_arn:
        return self._firehose_delivery_stream_role.role_arn

    @property
    def delivery_stream_log_group_name(self) -> logs.LogGroup.log_group_name:
        return self._firehose_delivery_stream_log_group.log_group_name


