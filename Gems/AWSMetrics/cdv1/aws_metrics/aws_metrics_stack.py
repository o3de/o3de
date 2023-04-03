"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core
)

from .real_time_data_processing import RealTimeDataProcessing
from .data_ingestion import DataIngestion
from .batch_processing import BatchProcessing
from .batch_analytics import BatchAnalytics
from .data_lake_integration import DataLakeIntegration
from .dashboard import Dashboard


class AWSMetricsStack(core.Stack):
    """
    Create the feature stack for the AWSMetrics Gem.

    Please reference the CloudFormation template provided by the Game Analytics Pipeline for the
    full production ready solution. This CDK application deploys a simplified version of this pipeline as an example.
    https://docs.aws.amazon.com/solutions/latest/game-analytics-pipeline/template.html
    """

    def __init__(self,
                 scope: core.Construct,
                 id_: str,
                 application_name: str,
                 optional_features: dict,
                 **kwargs) -> None:
        super().__init__(scope, id_, **kwargs)

        self._data_ingestion = DataIngestion(self, application_name)

        self._real_time_data_processing = RealTimeDataProcessing(
            self,
            input_stream_arn=self._data_ingestion.input_stream_arn,
            application_name=application_name
        )

        batch_processing_enabled = optional_features.get('batch_processing', False)
        server_access_logs_bucket = optional_features.get('server_access_logs_bucket')
        self._data_lake_integration = DataLakeIntegration(
            self,
            application_name=application_name,
            server_access_logs_bucket=server_access_logs_bucket
        ) if batch_processing_enabled else None

        self._batch_processing = BatchProcessing(
            self,
            input_stream_arn=self._data_ingestion.input_stream_arn,
            application_name=application_name,
            analytics_bucket_arn=self._data_lake_integration.analytics_bucket_arn,
            events_database_name=self._data_lake_integration.events_database_name,
            events_table_name=self._data_lake_integration.events_table_name
        ) if batch_processing_enabled else None

        self._batch_analytics = BatchAnalytics(
            self,
            application_name=application_name,
            analytics_bucket_name=self._data_lake_integration.analytics_bucket_name,
            events_database_name=self._data_lake_integration.events_database_name,
            events_table_name=self._data_lake_integration.events_table_name
        ) if batch_processing_enabled else None

        self._dashboard = Dashboard(
            self,
            input_stream_name=self._data_ingestion.input_stream_name,
            application_name=application_name,
            analytics_processing_lambda_name=self._real_time_data_processing.analytics_processing_lambda_name,
            delivery_stream_name=self._batch_processing.delivery_stream_name if batch_processing_enabled else '',
            events_processing_lambda_name=self._batch_processing.events_processing_lambda_name if batch_processing_enabled else ''
        )

    @property
    def data_ingestion_component(self):
        return self._data_ingestion

    @property
    def real_time_data_processing_component(self):
        return self._real_time_data_processing

    @property
    def dashboard_component(self):
        return self._dashboard

    @property
    def data_lake_integration_component(self):
        return self._data_lake_integration

    @property
    def batch_processing_component(self):
        return self._batch_processing

    @property
    def batch_analytics_component(self):
        return self._batch_analytics
