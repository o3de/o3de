"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Provides query functionality for .csv metrics from S3 buckets
"""

import argparse
import csv
import os

from boto3.session import Session

BUCKET_NAME = "o3de-metrics"

SESSION = None
DOWNLOADED_FILES = []


def _get_session():
    """
    Gets an AWS session via boto3 or returns the existing one
    """
    global SESSION
    if SESSION is None:
        # get session
        id_ = os.environ['AWS_ACCESS_KEY_ID']
        secret = os.environ['AWS_SECRET_ACCESS_KEY']
        token = os.environ['AWS_SESSION_TOKEN']

        SESSION = Session(
            aws_access_key_id=id_,
            aws_secret_access_key=secret,
            aws_session_token=token,
            region_name='us-west-2'
        )
    return SESSION


def _download_csvs(branch, start_week, end_week, download_folder):
    """
    Downloads the desired csvs based on branch, start_week, and end_week to the download_folder
    :param branch: name of the desired branch
    :param start_week: number of the first week to include
    :param end_week: number of the last week to include
    :param download_folder: destination folder for the .csvs to be stored while they are being parsed
    """
    bucket = _get_session().resource('s3').Bucket(BUCKET_NAME)

    prefix = "csv/" + branch

    objs = list(bucket.objects.filter(Prefix=prefix))

    file_type = ".csv"
    for obj in objs:
        if file_type == obj.key[-4:]:
            try:
                # Get the key file values
                filter_values = obj.key.split('/')
                branch = filter_values[-3]
                week_number = int((filter_values[-2])[4:])
                file_name = filter_values[-1]

                # Determine if the file should be downloaded
                should_download_file = True
                if branch != branch:
                    should_download_file = False
                if start_week and week_number < start_week:
                    should_download_file = False
                elif end_week and week_number > end_week:
                    should_download_file = False

                # Download the file
                if should_download_file:
                    week_folder = os.path.join(download_folder, str(week_number))
                    if not os.path.exists(week_folder):
                        os.mkdir(week_folder)
                    download_path = os.path.join(download_folder, str(week_number), file_name)
                    DOWNLOADED_FILES.append(download_path)
                    bucket.download_file(obj.key, download_path)

            # If a .csv is in the wrong location, it'll break the functionality above. We're just going to skip it.
            except ValueError as value_error:
                print("Found value error. Likely means a csv is in the wrong location. Skipping and continuing. Error "
                      "{0} ".format(str(value_error)))


def query_csvs(test_case_ids=None, test_case_metrics=None):
    """
    Queries the desired information from the downloaded csvs
    :param test_case_ids: the desired test case ids
    :param test_case_metrics: the desired test case metrics
    """
    query_results = []
    for downloaded_csv in DOWNLOADED_FILES:
        with open(downloaded_csv, 'r') as csv_file:
            reader = csv.reader(csv_file)
            parsed_header_row = False
            desired_columns = []
            for row in reader:

                # Get desired columns if test_case_metrics were passed
                if not parsed_header_row:
                    if test_case_metrics is not None:
                        for column_index, column_header in enumerate(row):
                            if column_header in test_case_metrics:
                                desired_columns.append(column_index)
                    parsed_header_row = True

                # Actually get the data
                if test_case_ids is not None and row[0] in test_case_ids:
                    query_results.append(_parse_row(test_case_metrics, row, desired_columns))
                elif test_case_ids is None:
                    query_results.append(_parse_row(test_case_metrics, row, desired_columns))
                    print("row " + str(row))
            csv_file.close()
    return query_results


def _parse_row(test_case_metrics, row, desired_columns):
    """
    Queries the desired information from the downloaded csvs
    :param test_case_metrics: the desired test case metrics
    :param row: the row being parsed
    :param desired_columns: the columns to include
    """
    row_to_add = []
    if test_case_metrics is not None:
        for column_number in desired_columns:
            row_to_add.append(row[int(column_number)])
        pass
    else:
        row_to_add = row
    return row_to_add


if __name__ == '__main__':
    # parse parameters
    parser = argparse.ArgumentParser(prog='MetricsQueryTool', description='Queries the metrics.')
    parser.add_argument('-b', '--branch', required=True)
    parser.add_argument('-sw', '--startweek', required=False)
    parser.add_argument('-ew', '--endweek', required=False)
    parser.add_argument('-df', '--downloadfolder', required=False)
    parser.add_argument('-tcids', '--testcaseids', nargs='+', required=False)
    parser.add_argument('-tcms', '--testcasemetrics', nargs='+', required=False)
    args = parser.parse_args()

    start_week = int(args.startweek) if args.startweek else None
    end_week = int(args.endweek) if args.endweek else None

    # download csvs
    _download_csvs(args.branch, start_week, end_week, args.downloadfolder)

    # query csvs for data
    query_csvs(args.testcaseids, args.testcasemetrics)
