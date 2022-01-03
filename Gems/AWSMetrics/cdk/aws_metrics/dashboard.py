"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_cloudwatch as cloudwatch
)

from . import aws_metrics_constants
from .layout_widget_construct import LayoutWidget
from .aws_utils import resource_name_sanitizer


class Dashboard:
    """
    Create the real time analytics CloudWatch dashboard for the AWSMetrics Gem.
    """
    def __init__(
            self,
            stack: core.Construct,
            input_stream_name: str,
            analytics_processing_lambda_name: str,
            application_name: str,
            delivery_stream_name: str = '',
            events_processing_lambda_name: str = '',
            ) -> None:

        self._dashboard_name = resource_name_sanitizer.sanitize_resource_name(
            f'{stack.stack_name}-Dashboard', 'cloudwatch_dashboard')
        self._dashboard = cloudwatch.Dashboard(
            stack,
            id="DashBoard",
            dashboard_name=self._dashboard_name,
            start=aws_metrics_constants.DASHBOARD_TIME_RANGE_START
        )

        self._dashboard.add_widgets(
            LayoutWidget(
                layout_description=aws_metrics_constants.DASHBOARD_GLOBAL_DESCRIPTION,
                widgets=[
                    self._create_operational_health_layout(
                        input_stream_name,
                        delivery_stream_name,
                        analytics_processing_lambda_name,
                        events_processing_lambda_name),
                    self._create_real_time_analytics_layout()
                ],
                max_width=aws_metrics_constants.DASHBOARD_MAX_WIDGET_WIDTH)
        )

        core.CfnOutput(
            stack,
            id='DashboardName',
            description='CloudWatch dashboard to monitor the operational health and real-time metrics',
            export_name=f"{application_name}:Dashboard",
            value=self._dashboard_name)

    def _create_operational_health_layout(
            self,
            input_stream_name: str,
            delivery_stream_name: str,
            analytics_processing_lambda_name: str,
            events_processing_lambda_name: str) -> LayoutWidget:
        """
        This layout contains operational health metrics during events ingestion and analytics processing.

        @param input_stream_name Name of the input Kinesis data stream.
        @param delivery_stream_name Name of the Kinesis Firehose delivery stream.
        @param analytics_processing_lambda_name Name of the analytics processing Lambda function.
        @param events_processing_lambda_name Name of the events processing Lambda function.
        @return Operational health layout widget.
        """
        operational_health_graph_widgets = list()

        event_ingestion_left_widgets = [
            cloudwatch.Metric(
                metric_name="IncomingRecords",
                label="Kinesis Incoming Records",
                namespace="AWS/Kinesis",
                period=core.Duration.minutes(aws_metrics_constants.DASHBOARD_METRICS_TIME_PERIOD),
                statistic="Sum",
                dimensions={
                    "StreamName": input_stream_name
                }
            )
        ]
        if delivery_stream_name:
            event_ingestion_left_widgets.append(
                cloudwatch.Metric(
                    metric_name="DeliveryToS3.Records",
                    label="Firehose Delivery To S3 Records",
                    namespace="AWS/Firehose",
                    period=core.Duration.minutes(aws_metrics_constants.DASHBOARD_METRICS_TIME_PERIOD),
                    statistic="Sum",
                    dimensions={
                        "DeliveryStreamName": delivery_stream_name
                    }
                )
            )

        operational_health_graph_widgets.append(
            cloudwatch.GraphWidget(
                title="Events Ingestion",
                left=event_ingestion_left_widgets,
                live_data=True
            )
        )

        analytics_processing_lambda_errors_metrics, analytics_processing_lambda_error_rate_metrics = \
            self._get_lambda_operational_health_metrics(
                analytics_processing_lambda_name,
                "Analytics Processing Lambda"
            )

        lambda_processing_left_widgets = [analytics_processing_lambda_errors_metrics]
        lambda_processing_right_widgets = [analytics_processing_lambda_error_rate_metrics]

        if events_processing_lambda_name:
            events_processing_lambda_errors_metrics, events_processing_lambda_error_rate_metrics = \
                self._get_lambda_operational_health_metrics(
                    events_processing_lambda_name,
                    "Events Processing Lambda"
                )

            lambda_processing_left_widgets.append(events_processing_lambda_errors_metrics)
            lambda_processing_right_widgets.append(events_processing_lambda_error_rate_metrics)

        operational_health_graph_widgets.append(
            cloudwatch.GraphWidget(
                title="Lambda Processing",
                left=lambda_processing_left_widgets,
                right=lambda_processing_right_widgets,
                right_y_axis=cloudwatch.YAxisProps(
                    show_units=False,
                    min=0,
                    max=100
                ),
                live_data=True,
                view=cloudwatch.GraphWidgetView.TIME_SERIES
            )
        )

        operational_health_layout = LayoutWidget(
            layout_description=aws_metrics_constants.DASHBOARD_OPERATIONAL_HEALTH_DESCRIPTION,
            widgets=operational_health_graph_widgets,
            max_width=aws_metrics_constants.DASHBOARD_MAX_WIDGET_WIDTH // 2)

        return operational_health_layout

    def _get_lambda_operational_health_metrics(self, function_name: str, metrics_label_prefix: str):
        """
        Get the errors and error rate metrics for the provided Lambda function.

        @param function_name Name of the Lambda function.
        @param metrics_label_prefix Prefix for the metrics Label. Metrics Label needs to be unique in a graph.
        @return Error and error rate metrics of the Lambda function.
        """
        lambda_errors_metrics = cloudwatch.Metric(
            metric_name='Errors',
            label=f'{metrics_label_prefix} Errors',
            namespace='AWS/Lambda',
            period=core.Duration.minutes(aws_metrics_constants.DASHBOARD_METRICS_TIME_PERIOD),
            statistic='Sum',
            dimensions={
                'FunctionName': function_name
            }
        )

        error_metrics_id = f'{metrics_label_prefix.replace(" ", "_")}_error'.lower()
        invocations_metrics_id = f'{metrics_label_prefix.replace(" ", "_")}_invocations'.lower()
        # Divide the Errors metric by the Invocations metric to get an error rate.
        lambda_error_rate_metrics = cloudwatch.MathExpression(
            expression=f'100 - 100 * {error_metrics_id} / MAX([{error_metrics_id}, {invocations_metrics_id}])',
            period=core.Duration.minutes(aws_metrics_constants.DASHBOARD_METRICS_TIME_PERIOD),
            label=f'{metrics_label_prefix} Success Rate (%)',
            using_metrics={
                error_metrics_id: lambda_errors_metrics,
                invocations_metrics_id: cloudwatch.Metric(
                    metric_name='Invocations',
                    namespace='AWS/Lambda',
                    period=core.Duration.minutes(aws_metrics_constants.DASHBOARD_METRICS_TIME_PERIOD),
                    statistic='Sum',
                    dimensions={
                        'FunctionName': function_name
                    }
                ),
            }
        )

        return lambda_errors_metrics, lambda_error_rate_metrics

    def _create_real_time_analytics_layout(self) -> LayoutWidget:
        """
        This layout contains real-time analytics metrics including login.
        """
        real_time_analytics_layout = LayoutWidget(
            layout_description=aws_metrics_constants.DASHBOARD_REAL_TIME_ANALYTICS_DESCRIPTION,
            widgets=[
                cloudwatch.GraphWidget(
                    title="Logins",
                    left=[
                        cloudwatch.Metric(
                            metric_name="TotalLogins",
                            label="Logins",
                            namespace="AWSMetrics",
                            period=core.Duration.minutes(aws_metrics_constants.DASHBOARD_METRICS_TIME_PERIOD),
                            statistic="Sum"
                        )
                    ],
                    live_data=True
                )
            ],
            max_width=aws_metrics_constants.DASHBOARD_MAX_WIDGET_WIDTH // 2)

        return real_time_analytics_layout

    @property
    def dashboard_name(self) -> str:
        return self._dashboard_name

