"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Provides query functionality for .csv metrics from S3 buckets

This file is in a prototype stage and is expected to change. It has many moving parts and when linking all  of them,
some adjustments will have to happen.
"""

import argparse
import csv
import os

import boto3
from boto3.session import Session

BUCKET_NAME = "o3de-metrics"

DOWNLOADED_FILES = []


def _get_session(region='us-west-2'):
    """
    Gets an AWS session via boto3
    :param region: desired AWS region for session
    """
    id_ = os.environ['AWS_ACCESS_KEY_ID']
    secret = os.environ['AWS_SECRET_ACCESS_KEY']
    token = os.environ['AWS_SESSION_TOKEN']

    session = Session(
        aws_access_key_id=id_,
        aws_secret_access_key=secret,
        aws_session_token=token,
        region_name=region
    )
    return session


def _download_csvs(desired_branch, start_week, end_week, download_folder, region, use_env_credentials):
    """
    Downloads the desired csvs based on branch, start_week, and end_week to the download_folder
    :param desired_branch: name of the desired branch
    :param start_week: number of the first week to include
    :param end_week: number of the last week to include
    :param download_folder: destination folder for the .csvs to be stored while they are being parsed
    :param region: desired AWS region for session
    :param use_env_credentials: use the env credentials instead of the configured boto3 credentials
    """
    bucket = _get_session(region, ).resource('s3').Bucket(BUCKET_NAME) if use_env_credentials else boto3.client('s3')

    prefix = "csv/" + desired_branch

    objects = list(bucket.objects.filter(Prefix=prefix))

    file_type = ".csv"
    for object in objects:
        if file_type == os.path.splitext(object.key)[1]:
            try:
                # Get the key file values
                # We are expecting files to be found in buckets/o3de-metrics/csv/branch/Week#/file_name.csv
                # If they are not, they won't be checked, and will be caught in the ValueError below
                filter_values = object.key.split('/')
                found_branch = filter_values[-3]
                week_number = int((filter_values[-2])[4:])
                file_name = filter_values[-1]
                print("found_branch: " + str(found_branch))
                print("week_number: " + str(week_number))
                print("file_name: " + str(file_name))

                # Determine if the file should be downloaded
                should_download_file = True
                if found_branch != desired_branch:
                    should_download_file = False
                if start_week and week_number < start_week:
                    should_download_file = False
                elif end_week and week_number > end_week:
                    should_download_file = False

                # Download the file
                if should_download_file:
                    print("str(download_folder): " + str(download_folder))
                    print("str(week_number): " + str(week_number))
                    week_folder = os.path.join(download_folder, str(week_number))
                    if not os.path.exists(week_folder):
                        os.mkdir(week_folder)
                    download_path = os.path.join(download_folder, str(week_number), file_name)
                    DOWNLOADED_FILES.append(download_path)
                    bucket.download_file(object.key, download_path)

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
    # This functionality is in a prototype stage, and expected to change

    # parse parameters
    parser = argparse.ArgumentParser(prog='MetricsQueryTool', description='Queries the metrics.')
    parser.add_argument('-b', '--branch', required=True)
    parser.add_argument('-sw', '--startweek', required=False)
    parser.add_argument('-ew', '--endweek', required=False, help='Max of 52.')
    parser.add_argument('-df', '--downloadfolder', required=True)
    parser.add_argument('-tcids', '--testcaseids', nargs='+', required=False)
    parser.add_argument('-tcms', '--testcasemetrics', nargs='+', required=False)
    parser.add_argument('-r', '--region', required=False)
    parser.add_argument('-env', '--envcredentials', required=False, help="Override the configured credentials to "
                                                                         "instead use environment variables.")
    args = parser.parse_args()

    start_week = int(args.startweek) if args.startweek else None
    end_week = int(args.endweek) if args.endweek else None

    # download csvs
    _download_csvs(args.branch, start_week, end_week, args.downloadfolder, args.region, args.envcredentials)

    # query csvs for data
    query_csvs(args.testcaseids, args.testcasemetrics)
