"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from __future__ import annotations

from aws_cdk import (
    core,
    aws_iam as iam
)

import typing

from aws_metrics.aws_metrics_stack import AWSMetricsStack
from aws_metrics.real_time_data_processing import RealTimeDataProcessing
from aws_metrics.data_ingestion import DataIngestion
from aws_metrics.dashboard import Dashboard
from aws_metrics.data_lake_integration import DataLakeIntegration
from aws_metrics.batch_processing import BatchProcessing
from aws_metrics.batch_analytics import BatchAnalytics
import aws_metrics.aws_metrics_constants as constants

from aws_metrics.policy_statements_builder.user_policy_statements_builder import UserPolicyStatementsBuilder


class AdminPolicyStatementsBuilder(UserPolicyStatementsBuilder):
    """
    Build the admin user policy statement list for the AWSMetrics gem
    """
    def __init__(self):
        super().__init__()

    def add_aws_metrics_stack_policy_statements(self, component: AWSMetricsStack) -> AdminPolicyStatementsBuilder:
        """
        Add the additional policy statements to update the CloudFormation stack for admin.

        :param component: CloudFormation stack created by the metrics gem.
        :return: The policy statement builder itself.
        """
        self._policy_statement_mapping['bootstrap'] = iam.PolicyStatement(
            actions=[
                's3:GetBucketLocation',
                's3:GetObject',
                's3:ListBucket',
                's3:PutObject'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub('arn:${AWS::Partition}:s3:::cdktoolkit-stagingbucket-*'),
                core.Fn.sub('arn:${AWS::Partition}:s3:::cdktoolkit-stagingbucket-*/*'),
            ],
            sid='UpdateBootstrapBucket'
        )

        self._policy_statement_mapping['cloudformation'] = iam.PolicyStatement(
            actions=[
                "cloudformation:DescribeStacks",
                "cloudformation:GetTemplate",
                'cloudformation:CreateChangeSet',
                'cloudformation:DescribeChangeSet',
                'cloudformation:ExecuteChangeSet',
                'cloudformation:DeleteChangeSet',
                'cloudformation:DescribeStackEvents'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub(
                    body='arn:${AWS::Partition}:cloudformation:${AWS::Region}:${AWS::AccountId}:stack/${StackName}/*',
                    variables={
                        'StackName': component.stack_name
                    }
                ),
                core.Fn.sub('arn:${AWS::Partition}:cloudformation:${AWS::Region}:${AWS::AccountId}:stack/CDKToolkit/*')
            ],
            sid='UpdateResourcesStacks'
        )

        return self

    def add_data_ingestion_policy_statements(self, component: DataIngestion) -> AdminPolicyStatementsBuilder:
        """
        Add the additional policy statement to check service APIs log and the input data stream for admin.

        :param component: Data ingestion component created by the metrics gem.
        :return: The policy statement builder itself.
        """
        super().add_data_ingestion_policy_statements(component)

        self._policy_statement_mapping['kinesis_stream'] = iam.PolicyStatement(
            actions=[
                'kinesis:DescribeStreamSummary'
            ],
            effect=iam.Effect.ALLOW,
            resources=[component.input_stream_arn],
            sid='DescribeKinesisStream'
        )

        self._add_to_logs_policy_statement(
            [
                core.Fn.sub(
                    body='arn:${AWS::Partition}:logs:${AWS::Region}:${AWS::AccountId}:log-group:'
                         'API-Gateway-Execution-Logs_${RestApiId}/${Stage}:log-stream:*',
                    variables={
                        'RestApiId': component.rest_api_id,
                        'Stage': component.deployment_stage
                    }
                )
            ]
        )

        return self

    def add_real_time_data_processing_policy_statements(self, component: RealTimeDataProcessing) -> AdminPolicyStatementsBuilder:
        """
        Add the additional policy statements to update the Kinesis Data Analytics application
        and analytics processing Lambda for admin.

        :param component: Real-time data processing component created by the metrics gem.
        :return: The policy statement builder itself.
        """
        self._policy_statement_mapping['kinesis_analytics'] = iam.PolicyStatement(
            actions=[
                'kinesisanalytics:AddApplicationOutput',
                'kinesisanalytics:DeleteApplicationOutput',
                'kinesisanalytics:DescribeApplication',
                'kinesisanalytics:StartApplication',
                'kinesisanalytics:StopApplication',
                'kinesisanalytics:UpdateApplication'
            ],
            effect=iam.Effect.ALLOW,

            resources=[
                core.Fn.sub(
                    body='arn:${AWS::Partition}:kinesisanalytics:${AWS::Region}:${AWS::AccountId}:application/'
                         '${AnalyticsApplicationName}',
                    variables={
                        'AnalyticsApplicationName': component.analytics_application_name
                    }
                )
            ],
            sid='UpdateAnalyticsApplication'
        )

        self._add_to_iam_policy_statement(
            [
                component.analytics_application_role_arn,
                component.analytics_application_lambda_role_arn
            ]
        )

        self._add_to_lambda_policy_statement([component.analytics_processing_lambda_arn])

        self._add_to_logs_policy_statement(
            [
                core.Fn.sub(
                    body='arn:${AWS::Partition}:logs:${AWS::Region}:${AWS::AccountId}:log-group:'
                         '/aws/lambda/${AnalyticsProcessingLambdaName}:log-stream:*',
                    variables={
                        'AnalyticsProcessingLambdaName': component.analytics_processing_lambda_name
                    }
                )
            ]
        )

        return self

    def add_dashboard_policy_statements(self, component: Dashboard) -> AdminPolicyStatementsBuilder:
        """
        Add the additional policy statements to update the CloudWatch dashboard for admin.

        :param component: CloudWatch dashboard component created by the metrics gem.
        :return: The policy statement builder itself.
        """
        self._policy_statement_mapping['dashboard'] = iam.PolicyStatement(
            actions=[
                'cloudwatch:GetDashboard',
                'cloudwatch:PutDashboard'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub(
                        body='arn:${AWS::Partition}:cloudwatch::${AWS::AccountId}:dashboard/${DashboardName}',
                        variables={
                            'DashboardName': component.dashboard_name
                        }
                    )
                ],
            sid='UpdateDashboard'
        )

        return self

    def add_data_lake_integration_policy_statements(
            self,
            component: DataLakeIntegration) -> AdminPolicyStatementsBuilder:
        """
        Add the policy statements to retrieve the analytics bucket content and
        update Glue database, table and crawler for admin.

        :param component: CloudWatch dashboard component created by the metrics gem.
        :return: The policy statement builder itself.
        """
        if not component:
            return self

        self._policy_statement_mapping['glue_database'] = iam.PolicyStatement(
            actions=[
                'glue:GetDatabase',
                'glue:UpdateDatabase'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub('arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:catalog'),
                core.Fn.sub(
                    body='arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:database/${EventsDatabaseName}',
                    variables={
                        'EventsDatabaseName': component.events_database_name
                    }
                )
            ],
            sid='UpdateEventsDatabase'
        )

        self._policy_statement_mapping['glue_table'] = iam.PolicyStatement(
                actions=[
                    'glue:GetTable',
                    'glue:GetTableVersion',
                    'glue:GetTableVersions',
                    'glue:UpdateTable',
                    'glue:GetPartitions'
                ],
                effect=iam.Effect.ALLOW,
                resources=[
                    core.Fn.sub('arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:catalog'),
                    core.Fn.sub(
                        body='arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:table/'
                             '${EventsDatabaseName}/${EventsTableName}',
                        variables={
                            'EventsDatabaseName': component.events_database_name,
                            'EventsTableName': component.events_table_name
                        }
                    ),
                    core.Fn.sub(
                        body='arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:'
                             'database/${EventsDatabaseName}',
                        variables={
                            'EventsDatabaseName': component.events_database_name
                        }
                    )
                ],
                sid='UpdateEventsTable'
            )

        self._policy_statement_mapping['glue_crawler'] = iam.PolicyStatement(
            actions=[
                'glue:GetCrawler',
                'glue:StartCrawler',
                'glue:StopCrawler',
                'glue:UpdateCrawler'
            ],
            effect=iam.Effect.ALLOW,
            resources=[core.Fn.sub(
                body='arn:${AWS::Partition}:glue:${AWS::Region}:${AWS::AccountId}:crawler/${EventsCrawlerName}',
                variables={
                    'EventsCrawlerName': component.events_crawler_name
                }
            )],
            sid='UpdateEventsCrawler'
        )

        self._policy_statement_mapping['s3_read'] = iam.PolicyStatement(
            actions=[
                's3:GetObject',
                's3:ListBucket'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub(
                    body='arn:${AWS::Partition}:s3:::${AnalyticsBucketName}',
                    variables={
                        'AnalyticsBucketName': component.analytics_bucket_name
                    }
                ),
                core.Fn.sub(
                    body='arn:${AWS::Partition}:s3:::${AnalyticsBucketName}/*',
                    variables={
                        'AnalyticsBucketName': component.analytics_bucket_name
                    }
                )
            ],
            sid='GetAnalyticsBucketObjects'
        )

        self._policy_statement_mapping['s3_write'] = iam.PolicyStatement(
            actions=[
                's3:PutObject'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub(
                    body='arn:${AWS::Partition}:s3:::${AnalyticsBucketName}/${AthenaOutputDirectory}',
                    variables={
                        'AnalyticsBucketName': component.analytics_bucket_name,
                        'AthenaOutputDirectory': constants.ATHENA_OUTPUT_DIRECTORY
                    }
                ),
                core.Fn.sub(
                    body='arn:${AWS::Partition}:s3:::${AnalyticsBucketName}/${AthenaOutputDirectory}/*',
                    variables={
                        'AnalyticsBucketName': component.analytics_bucket_name,
                        'AthenaOutputDirectory': constants.ATHENA_OUTPUT_DIRECTORY
                    }
                )
            ],
            sid='PutQueryResults'
        )

        self._add_to_iam_policy_statement([component.events_crawler_role_arn])

        self._add_to_logs_policy_statement(
            [
                core.Fn.sub('arn:${AWS::Partition}:logs:${AWS::Region}:${AWS::AccountId}:log-group:'
                            '/aws-glue/crawlers:log-stream:*')
            ]
        )

        return self

    def add_batch_processing_policy_statements(self, component: BatchProcessing) -> AdminPolicyStatementsBuilder:
        """
        Add the policy statements to update the Kinesis Data Firehose delivery stream
        and events processing Lambda for admin.

        :param component: batch processing component created by the metrics gem.
        :return: The policy statement builder itself.
        """
        if not component:
            return self

        self._policy_statement_mapping['firehose'] = iam.PolicyStatement(
            actions=[
                'firehose:DescribeDeliveryStream',
                'firehose:UpdateDestination'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub(
                    body='arn:${AWS::Partition}:firehose:${AWS::Region}:${AWS::AccountId}:'
                         'deliverystream/${DeliveryStreamName}',
                    variables={
                        'DeliveryStreamName': component.delivery_stream_name
                    }
                )
            ],
            sid='UpdateDeliveryStreamDestination'
        )

        self._add_to_lambda_policy_statement([component.events_processing_lambda_arn])

        self._add_to_iam_policy_statement(
            [
                component.delivery_stream_role_arn,
                component.events_processing_lambda_role_arn
            ]
        )

        self._add_to_logs_policy_statement(
            [
                core.Fn.sub(
                    body='arn:${AWS::Partition}:logs:${AWS::Region}:${AWS::AccountId}:log-group:'
                         '/aws/lambda/${EventsProcessingLambdaName}:log-stream:*',
                    variables={
                        'EventsProcessingLambdaName': component.events_processing_lambda_name
                    }
                ),
                core.Fn.sub(
                    body='arn:${AWS::Partition}:logs:${AWS::Region}:${AWS::AccountId}:log-group:'
                         '${DeliveryStreamLogGroupName}:log-stream:*',
                    variables={
                        'DeliveryStreamLogGroupName': component.delivery_stream_log_group_name
                    }
                )
            ]
        )

        return self

    def add_batch_analytics_policy_statements(self, component: BatchAnalytics) -> AdminPolicyStatementsBuilder:
        """
        Add the policy statements to get named queries/executions and update the work group for admin.

        :param component: Batch processing component created by the metrics gem.
        :return: The policy statement builder itself.
        """
        if not component:
            return self

        self._policy_statement_mapping['athena'] = iam.PolicyStatement(
            actions=[
                'athena:BatchGetNamedQuery',
                'athena:BatchGetQueryExecution',
                'athena:GetNamedQuery',
                'athena:GetQueryExecution',
                'athena:GetQueryResults',
                'athena:StartQueryExecution',
                'athena:StopQueryExecution',
                'athena:ListNamedQueries',
                'athena:ListQueryExecutions',
                'athena:GetWorkGroup',
                'athena:UpdateWorkGroup'
            ],
            effect=iam.Effect.ALLOW,
            resources=[
                core.Fn.sub(
                    body='arn:${AWS::Partition}:athena:${AWS::Region}:${AWS::AccountId}:workgroup/${WorkGroupName}',
                    variables={
                        'WorkGroupName': component.athena_work_group_name
                    }
                )
            ],
            sid='UpdateAthenaWorkGroupAndRunQuery'
        )
        return self

    def _add_to_logs_policy_statement(self, resource_list: typing.List[str]) -> None:
        """
        Add new resources to the logs policy statement.

        :param resource_list: Resources to add.
        """
        if not self._policy_statement_mapping.get('logs'):
            self._policy_statement_mapping['logs'] = iam.PolicyStatement(
                actions=[
                    'logs:DescribeLogStreams',
                    'logs:GetLogEvents'
                ],
                effect=iam.Effect.ALLOW,
                resources=resource_list,
                sid='AccessLogs'
            )
        else:
            self._policy_statement_mapping['logs'].add_resources(*resource_list)

    def _add_to_lambda_policy_statement(self, resource_list: typing.List[str]) -> None:
        """
        Add new resources to the Lambda policy statement.

        :param resource_list: Resources to add.
        """
        if not self._policy_statement_mapping.get('lambda'):
            self._policy_statement_mapping['lambda'] = iam.PolicyStatement(
                actions=[
                    'lambda:GetFunction',
                    'lambda:GetFunctionConfiguration',
                    'lambda:UpdateFunctionCode',
                    'lambda:UpdateFunctionConfiguration',
                    'lambda:ListTags',
                    'lambda:TagResource',
                    'lambda:UntagResource'
                ],
                effect=iam.Effect.ALLOW,
                resources=resource_list,
                sid='UpdateLambda'
            )
        else:
            self._policy_statement_mapping['lambda'].add_resources(*resource_list)

    def _add_to_iam_policy_statement(self, resource_list: typing.List[str]) -> None:
        """
        Add new resources to the IAM policy statement.

        :param resource_list: Resources to add.
        """
        if not self._policy_statement_mapping.get('iam'):
            self._policy_statement_mapping['iam'] = iam.PolicyStatement(
                actions=[
                    'iam:PassRole',
                    'iam:GetRole'
                ],
                effect=iam.Effect.ALLOW,
                resources=resource_list,
                sid='PassRole'
            )
        else:
            self._policy_statement_mapping['iam'].add_resources(*resource_list)
