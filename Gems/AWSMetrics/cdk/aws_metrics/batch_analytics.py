"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

from aws_cdk import (
    core,
    aws_athena as athena
)

from . import aws_metrics_constants
from .aws_utils import resource_name_sanitizer


class BatchAnalytics:
    """
    Query the metrics stored in the S3 data lake via Amazon Athena
    """
    def __init__(self,
                 stack: core.Construct,
                 application_name: str,
                 analytics_bucket_name: str,
                 events_database_name: str,
                 events_table_name) -> None:
        self._stack = stack
        self._application_name = application_name
        self._analytics_bucket_name = analytics_bucket_name
        self._events_database_name = events_database_name
        self._events_table_name = events_table_name

        self._create_athena_work_group()
        self._create_athena_queries()

    def _create_athena_work_group(self) -> None:
        """
        Create a specific athena work group for access control.
        """
        self._athena_work_group = athena.CfnWorkGroup(
            self._stack,
            id='AthenaWorkGroup',
            name=resource_name_sanitizer.sanitize_resource_name(
                f'{self._stack.stack_name}-AthenaWorkGroup', 'athena_work_group'),
            recursive_delete_option=True,
            state='ENABLED',
            work_group_configuration=athena.CfnWorkGroup.WorkGroupConfigurationProperty(
                publish_cloud_watch_metrics_enabled=True,
                result_configuration=athena.CfnWorkGroup.ResultConfigurationProperty(
                    encryption_configuration=athena.CfnWorkGroup.EncryptionConfigurationProperty(
                        encryption_option='SSE_S3'
                    ),
                    output_location=core.Fn.sub(
                        body='s3://${AnalyticsBucket}/${AthenaOutputDirectory}/',
                        variables={
                            'AnalyticsBucket': self._analytics_bucket_name,
                            'AthenaOutputDirectory': aws_metrics_constants.ATHENA_OUTPUT_DIRECTORY
                        }
                    )
                )
            )
        )
        core.CfnOutput(
            self._stack,
            id='AthenaWorkGroupName',
            description='Name of the Athena work group that contains sample queries',
            export_name=f"{self._application_name}:AthenaWorkGroup",
            value=self._athena_work_group.name)

    def _create_athena_queries(self) -> None:
        """
        Create several example queries for reference.
        """
        self._named_queries = [
            athena.CfnNamedQuery(
                self._stack,
                id='NamedQuery-CreatePartitionedEventsJson',
                name=resource_name_sanitizer.sanitize_resource_name(
                    f'{self._stack.stack_name}-NamedQuery-CreatePartitionedEventsJson', 'athena_named_query'),
                database=self._events_database_name,
                query_string="CREATE TABLE events_json "
                             "WITH (format='JSON',partitioned_by=ARRAY['application_id']) "
                             "AS SELECT year, month, day, event_id, application_id "
                             f"FROM \"{self._events_database_name}\".\"{self._events_table_name}\"",
                description='This command demonstrates how to create a new table of raw events'
                            ' transformed to JSON format. Output is partitioned by Application',
                work_group=self._athena_work_group.name
            ),
            athena.CfnNamedQuery(
                self._stack,
                id='NamedQuery-TotalEventsLastMonth',
                name=resource_name_sanitizer.sanitize_resource_name(
                    f'{self._stack.stack_name}-NamedQuery-TotalEventsLastMonth', 'athena_named_query'),
                database=self._events_database_name,
                query_string="WITH detail AS "
                             "(SELECT date_trunc('month', date(date_parse(CONCAT(year, '-', month, '-', day), '%Y-%m-%d'))) as event_month, * "
                             f"FROM \"{self._events_database_name}\".\"{self._events_table_name}\") "
                             "SELECT "
                             "date_trunc('month', event_month) as month, application_id, count(DISTINCT event_id) as event_count "
                             "FROM detail "
                             "GROUP BY date_trunc('month', event_month), application_id",
                description='Total events over last month',
                work_group=self._athena_work_group.name
            ),
            athena.CfnNamedQuery(
                self._stack,
                id='NamedQuery-LoginLastMonth',
                name=resource_name_sanitizer.sanitize_resource_name(
                    f'{self._stack.stack_name}-NamedQuery-LoginLastMonth', 'athena_named_query'),
                database=self._events_database_name,
                query_string="WITH detail AS ("
                             "SELECT date_trunc('month', date(date_parse(CONCAT(year, '-', month, '-', day), '%Y-%m-%d'))) as event_month, * "
                             f"FROM \"{self._events_database_name}\".\"{self._events_table_name}\") "
                             "SELECT "
                             "date_trunc('month', event_month) as month, "
                             "count(*) as new_accounts "
                             "FROM detail "
                             "WHERE event_name = 'login' "
                             "GROUP BY date_trunc('month', event_month)",
                description='Total number of login events over the last month',
                work_group=self._athena_work_group.name
            )
        ]

        for named_query in self._named_queries:
            named_query.node.add_dependency(self._athena_work_group)

    @property
    def athena_work_group_name(self) -> athena.CfnWorkGroup.name:
        return self._athena_work_group.name

