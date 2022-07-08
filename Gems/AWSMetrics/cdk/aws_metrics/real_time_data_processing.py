"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_iam as iam,
    aws_kinesisanalytics as analytics,
    aws_lambda as lambda_,
    aws_logs as logs
)

import os

from . import aws_metrics_constants
from .aws_utils import resource_name_sanitizer


class RealTimeDataProcessing:
    """
    Create the AWS resources used for real time data processing
    """

    def __init__(self, stack: core.Construct, input_stream_arn: str, application_name: str) -> None:
        self._stack = stack
        self._input_stream_arn = input_stream_arn
        self._application_name = application_name

        self._create_analytics_processing_lambda()

        self._create_analytics_application()

    def _create_analytics_application(self) -> None:
        """
        Generate the Kinesis data analytics application to process the real-time data.
        The sample application filters input events and counts the total number of login within one minute.

        :return: The created Kinesis data analytics application.
        """
        self._analytics_application_role = self._create_analytics_application_role()

        self._analytics_application = analytics.CfnApplication(
            self._stack,
            'AnalyticsApplication',
            application_name=resource_name_sanitizer.sanitize_resource_name(
                f'{self._stack.stack_name}-AnalyticsApplication', 'kinesis_application'),
            inputs=[
                analytics.CfnApplication.InputProperty(
                    input_schema=analytics.CfnApplication.InputSchemaProperty(
                        record_columns=[
                            analytics.CfnApplication.RecordColumnProperty(
                                name='event_id',
                                sql_type='VARCHAR(32)',
                                mapping='$.event_id'
                            ),
                            analytics.CfnApplication.RecordColumnProperty(
                                name='event_type',
                                sql_type='VARCHAR(32)',
                                mapping='$.event_type'
                            ),
                            analytics.CfnApplication.RecordColumnProperty(
                                name='event_name',
                                sql_type='VARCHAR(32)',
                                mapping='$.event_name'
                            ),
                            analytics.CfnApplication.RecordColumnProperty(
                                name='event_version',
                                sql_type='VARCHAR(32)',
                                mapping='$.event_version'
                            ),
                            analytics.CfnApplication.RecordColumnProperty(
                                name='event_timestamp',
                                sql_type='VARCHAR(32)',
                                mapping='$.event_timestamp'
                            ),
                            analytics.CfnApplication.RecordColumnProperty(
                                name='application_id',
                                sql_type='VARCHAR(32)',
                                mapping='$.application_id'
                            )
                        ],
                        record_format=analytics.CfnApplication.RecordFormatProperty(
                            record_format_type='JSON'
                        )
                    ),
                    name_prefix='AnalyticsApp',
                    kinesis_streams_input=analytics.CfnApplication.KinesisStreamsInputProperty(
                        resource_arn=self._input_stream_arn,
                        role_arn=self._analytics_application_role.role_arn
                    )
                )
            ],
            application_description='',
            application_code=aws_metrics_constants.KINESIS_APPLICATION_CODE
        )

        self._application_output = analytics.CfnApplicationOutput(
            self._stack,
            'AnalyticsApplicationOutput',
            application_name=self._analytics_application.ref,
            output=analytics.CfnApplicationOutput.OutputProperty(
                destination_schema=analytics.CfnApplicationOutput.DestinationSchemaProperty(
                    record_format_type='JSON'
                ),
                lambda_output=analytics.CfnApplicationOutput.LambdaOutputProperty(
                    resource_arn=self._analytics_processing_lambda.function_arn,
                    role_arn=self._analytics_application_role.role_arn
                ),
                name='DESTINATION_STREAM'
            ),
        )

        core.CfnOutput(
            self._stack,
            id='AnalyticsApplicationName',
            description='Kinesis Data Analytics application to process the real-time metrics data',
            export_name=f"{self._application_name}:AnalyticsApplication",
            value=self._analytics_application.application_name)

    def _create_analytics_application_role(self) -> iam.Role:
        """
        Generate the IAM role for the Kinesis analytics application to read events from the input stream
        and send the processed data to the analytics processing lambda.

        :return: The created IAM role.
        """
        kinesis_access_policy_document = iam.PolicyDocument(
            statements=[
                iam.PolicyStatement(
                    actions=[
                        'kinesis:DescribeStream',
                        'kinesis:GetShardIterator',
                        'kinesis:GetRecords',
                        'kinesis:ListShards'
                    ],
                    effect=iam.Effect.ALLOW,
                    sid='ReadKinesisStream',
                    resources=[
                        self._input_stream_arn
                    ]
                )
            ]
        )

        lambda_access_policy_document = iam.PolicyDocument(
            statements=[
                iam.PolicyStatement(
                    actions=[
                        'lambda:InvokeFunction',
                        'lambda:GetFunctionConfiguration',
                    ],
                    effect=iam.Effect.ALLOW,
                    sid='AnalyticsProcessingInvokePermissions',
                    resources=[
                        self._analytics_processing_lambda.function_arn
                    ]
                )
            ]
        )

        kinesis_analytics_role = iam.Role(
            self._stack,
            id='AnalyticsApplicationRole',
            role_name=resource_name_sanitizer.sanitize_resource_name(
                f'{self._stack.stack_name}-AnalyticsApplicationRole', 'iam_role'),
            assumed_by=iam.ServicePrincipal(
                service='kinesisanalytics.amazonaws.com'
            ),
            inline_policies={
                'KinesisAccess': kinesis_access_policy_document,
                'LambdaAccess': lambda_access_policy_document
            }
        )

        return kinesis_analytics_role

    def _create_analytics_processing_lambda(self) -> None:
        """
        Generate the analytics processing lambda to send processed data to CloudWatch for visualization.
        """
        analytics_processing_function_name = resource_name_sanitizer.sanitize_resource_name(
            f'{self._stack.stack_name}-AnalyticsProcessingLambda', 'lambda_function')
        self._analytics_processing_lambda_role = self._create_analytics_processing_lambda_role(
            analytics_processing_function_name
        )
        self._analytics_processing_lambda = lambda_.Function(
            self._stack,
            id='AnalyticsProcessingLambda',
            function_name=analytics_processing_function_name,
            log_retention=logs.RetentionDays.ONE_MONTH,
            memory_size=aws_metrics_constants.LAMBDA_MEMORY_SIZE_IN_MB,
            runtime=lambda_.Runtime.PYTHON_3_7,
            timeout=core.Duration.minutes(aws_metrics_constants.LAMBDA_TIMEOUT_IN_MINUTES),
            handler='analytics_processing.lambda_handler',
            code=lambda_.Code.from_asset(
                os.path.join(os.path.dirname(__file__), 'lambdas', 'analytics_processing_lambda')),
            role=self._analytics_processing_lambda_role
        )
        core.CfnOutput(
            self._stack,
            id='AnalyticsProcessingLambdaName',
            description='Lambda function for sending processed data to CloudWatch.',
            export_name=f"{self._application_name}:AnalyticsProcessingLambda",
            value=self._analytics_processing_lambda.function_name)

    def _create_analytics_processing_lambda_role(self, function_name: str) -> iam.Role:
        """
        Generate the IAM role for the analytics processing lambda to send metrics to CloudWatch.

        @param function_name Name of the Lambda function.
        @return The created IAM role.
        """
        analytics_processing_policy_document = iam.PolicyDocument(
            statements=[
                # The following policy limits the user to publishing metrics only in the namespace named AWSMetrics.
                # Check the following document for more details:
                # https://docs.aws.amazon.com/AmazonCloudWatch/latest/monitoring/iam-cw-condition-keys-namespace.html
                iam.PolicyStatement(
                    actions=[
                        'cloudwatch:PutMetricData',
                    ],
                    effect=iam.Effect.ALLOW,
                    resources=[
                        '*'
                    ],
                    conditions={
                        "StringEquals": {
                            "cloudwatch:namespace": "AWSMetrics"
                        }
                    }
                ),
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

        analytics_processing_lambda_role = iam.Role(
            self._stack,
            id='AnalyticsLambdaRole',
            role_name=resource_name_sanitizer.sanitize_resource_name(
                f'{self._stack.stack_name}-AnalyticsLambdaRole', 'iam_role'),
            assumed_by=iam.ServicePrincipal(
                service='lambda.amazonaws.com'
            ),
            inline_policies={
                'AnalyticsProcessingPolicy': analytics_processing_policy_document
            }
        )

        return analytics_processing_lambda_role

    @property
    def analytics_processing_lambda_name(self) -> lambda_.Function.function_name:
        return self._analytics_processing_lambda.function_name

    @property
    def analytics_processing_lambda_arn(self) -> lambda_.Function.function_arn:
        return self._analytics_processing_lambda.function_arn

    @property
    def analytics_application_lambda_role_arn(self) -> iam.Role.role_arn:
        return self._analytics_processing_lambda_role.role_arn

    @property
    def analytics_application_name(self) -> analytics.CfnApplication.application_name:
        return self._analytics_application.application_name

    @property
    def analytics_application_role_arn(self) -> iam.Role.role_arn:
        return self._analytics_application_role.role_arn


