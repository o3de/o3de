"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

# Constants for API Gateway Service API
APIGATEWAY_STAGE = "api"


# Constants for Kinesis Data Analytics application
# The SQL code for the Kinesis analytics application.
KINESIS_APPLICATION_CODE = "-- ** Continuous Filter **\n"\
                   "CREATE OR REPLACE STREAM \"DESTINATION_STREAM\" (\n"\
                   "METRIC_NAME VARCHAR(1024),\n"\
                   "METRIC_TIMESTAMP BIGINT,\n"\
                   "METRIC_UNIT_VALUE_INT BIGINT,\n"\
                   "METRIC_UNIT VARCHAR(1024),\n"\
                   "OUTPUT_TYPE VARCHAR(1024));\n"\
                   "CREATE OR REPLACE PUMP \"LOGIN_PUMP\" AS\n"\
                   "INSERT INTO \"DESTINATION_STREAM\" (METRIC_NAME, METRIC_TIMESTAMP, METRIC_UNIT_VALUE_INT, METRIC_UNIT, OUTPUT_TYPE)\n"\
                   "SELECT STREAM 'TotalLogins', UNIX_TIMESTAMP(TIME_WINDOW), COUNT(distinct_stream.login_count) AS unique_count, 'Count', 'metrics'\n"\
                   "FROM (\n"\
                   "    SELECT STREAM DISTINCT\n"\
                   "    ROWTIME as window_time,\n"\
                   "    \"AnalyticsApp_001\".\"event_id\" as login_count,\n"\
                   "    STEP(\"AnalyticsApp_001\".ROWTIME BY INTERVAL '1' MINUTE) as TIME_WINDOW\n"\
                   "    FROM \"AnalyticsApp_001\"\n"\
                   "    WHERE \"AnalyticsApp_001\".\"event_name\" = 'login'\n"\
                   ") as distinct_stream\n"\
                   "GROUP BY\n"\
                   "    TIME_WINDOW,\n"\
                   "    STEP(distinct_stream.window_time BY INTERVAL '1' MINUTE);\n"


# Constants for the analytics processing and events processing lambda.
LAMBDA_TIMEOUT_IN_MINUTES = 5
# The amount of memory available to the function at runtime. Range from 128 to 10240.
# https://docs.aws.amazon.com/AWSCloudFormation/latest/UserGuide/aws-resource-lambda-function.html#cfn-lambda-function-memorysize
LAMBDA_MEMORY_SIZE_IN_MB = 256


# Constants for the Glue database and table.
GLUE_TABLE_NAME = 'firehose_events'
# Input/output format and serialization library for the Glue table (Same as the Game Analytics Pipeline solution).
# The Firehose delivery stream will use this table to convert the metrics data to the parquet format.
# Check https://docs.aws.amazon.com/firehose/latest/dev/record-format-conversion.html for converting
# the input record format in Kinesis Data Firehose.
GLUE_TABLE_INPUT_FORMAT = 'org.apache.hadoop.hive.ql.io.parquet.MapredParquetInputFormat'
GLUE_TABLE_OUTPUT_FORMAT = 'org.apache.hadoop.hive.ql.io.parquet.MapredParquetOutputFormat'
GLUE_TABLE_SERIALIZATION_LIBRARY = 'org.apache.hadoop.hive.ql.io.parquet.serde.ParquetHiveSerDe'
GLUE_TABLE_SERIALIZATION_LIBRARY_SERIALIZATION_FORMAT = '1'


# Constants for the Glue crawler.
CRAWLER_CONFIGURATION = '{'\
                        '   \"Version\":1.0,'\
                        '   \"CrawlerOutput\":'\
                        '   {'\
                        '       \"Partitions\":'\
                        '       {'\
                        '           \"AddOrUpdateBehavior\":\"InheritFromTable\"'\
                        '       },'\
                        '       \"Tables\":'\
                        '       {'\
                        '           \"AddOrUpdateBehavior\":\"MergeNewColumns\"'\
                        '       }'\
                        '   }'\
                        '}'\

# Constants for the Kinesis Data Firehose delivery stream.
# Hints for the buffering to perform before delivering data to the destination.
# These options are treated as hints, and therefore Kinesis Data Firehose might choose to
# use different values when it is optimal.
DELIVERY_STREAM_BUFFER_HINTS_INTERVAL_IN_SECONDS = 60
DELIVERY_STREAM_BUFFER_HINTS_SIZE_IN_MBS = 128

# Configuration for the destination S3 bucket.
S3_DESTINATION_PREFIX = GLUE_TABLE_NAME + '/year=!{timestamp:YYYY}/month=!{timestamp:MM}/day=!{timestamp:dd}/'
S3_DESTINATION_ERROR_OUTPUT_PREFIX = 'firehose_errors/year=!{timestamp:YYYY}/month=!{timestamp:MM}/' \
                                     'day=!{timestamp:dd}/!{firehose:error-output-type}'
# Parquet format is already compressed with SNAPPY
S3_COMPRESSION_FORMAT = 'UNCOMPRESSED'

# Configuration for the data processor for an Amazon Kinesis Data Firehose delivery stream.
# Set the length of time that Data Firehose buffers incoming data before delivering it to the destination. Valid Range:
# Minimum value of 60. Maximum value of 900.
# https://docs.aws.amazon.com/firehose/latest/APIReference/API_BufferingHints.html
PROCESSOR_BUFFER_INTERVAL_IN_SECONDS = '60'
# Buffer incoming data to the specified size before delivering it to the destination. Recommend setting this
# parameter to a value greater than the amount of data you typically ingest into the delivery stream in 10 seconds.
# https://docs.aws.amazon.com/firehose/latest/APIReference/API_BufferingHints.html
PROCESSOR_BUFFER_SIZE_IN_MBS = '3'
# Number of retries for delivering the data to S3.
# Minimum value of 1. Maximum value of 512.
# https://docs.aws.amazon.com/AWSCloudFormation/latest/UserGuide/aws-properties-kinesisfirehose-deliverystream-processorparameter.html
PROCESSOR_BUFFER_NUM_OF_RETRIES = '3'
PARQUET_SER_DE_COMPRESSION = 'SNAPPY'

# Constants for the Athena engine
# Directory name in S3 for the Athena query outputs
ATHENA_OUTPUT_DIRECTORY = 'athena_query_results'


# Constants for the CloudWatch dashboard
# The start of the time range to use for each widget on the dashboard. Set a 15 minutes view.
DASHBOARD_TIME_RANGE_START = '-PT15M'
# The maximum amount of horizontal grid units the widget will take up.
DASHBOARD_MAX_WIDGET_WIDTH = 24
# The amount of vertical grid units the global description widget will take up.
DASHBOARD_GLOBAL_DESCRIPTION_WIDGET_HEIGHT = 3
# The time period used for metric data aggregations.
DASHBOARD_METRICS_TIME_PERIOD = 1
# The global description for the CloudWatch dashboard
DASHBOARD_GLOBAL_DESCRIPTION = "# Metrics Dashboard  \n"\
                               "This dashboard contains near-real-time metrics sent from your client"\
                               " or dedicated server.  \n You can edit the widgets using the AWS console"\
                               " or modify your CDK application code. Please note that redeploying"\
                               " the CDK application will overwrite any changes you made directly"\
                               " via the AWS console.  \n For more information about using the AWS Metrics Gem"\
                               " and CDK application, please check the AWSMetrics gem document."
# The description for the operational health shown on the CloudWatch dashboard
DASHBOARD_OPERATIONAL_HEALTH_DESCRIPTION = "## Operational Health  \n"\
                                           "This section covers operational metrics for the data analytics pipeline "\
                                           "during metrics event ingestion, processing and analytics."
# The description for the real time analytics shown on the CloudWatch dashboard
DASHBOARD_REAL_TIME_ANALYTICS_DESCRIPTION = "## Real-time Streaming Analytics  \n"\
                                            "This section covers real-time analytics metrics sent " \
                                            "to the data analytics pipeline."
