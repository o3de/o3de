#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
# Simple dynamodb seeder script
#
# Writes x random values into the example dynamodb in the example stack, with an id item<number>
# to ensure table has values to experiment with
import argparse
import sys
import logging
import random
import time
from typing import List

import boto3
from boto3.dynamodb.table import TableResource
from botocore.exceptions import ClientError

logging.basicConfig(stream=sys.stdout, format="%(asctime)s %(process)d %(message)s")
logger = logging.getLogger()
logger.setLevel(logging.INFO)

DEFAULT_ENTRIES = 10


def _write_table_data(table: boto3.dynamodb.table.TableResource, table_data: List):
    """
    Write a list of items to a DynamoDB table using the batch_writer.

    Each item must contain at least the keys required by the schema, which for the example
    is just 'id'

    :param table: The table to fill
    :param table_data: The data to put in the table.
    """
    try:
        with table.batch_writer() as writer:
            for item in table_data:
                writer.put_item(Item=item)
        logger.info(f'Loaded data into table {table.name}')
    except ClientError:
        logger.exception(f'Failed to load data into table {table.name}')
        raise


def generate_entries(table_name: str, num_entries: int, session: boto3.Session) -> None:
    """
    Populates a number of entries with simple random data

    :param table_name: The name of the table to write data into
    :param num_entries: The number of entries to create
    :param session: An existing boto3 session to use to create a dynamodb client from
    """
    dynamodb = session.resource('dynamodb')
    table = dynamodb.Table(table_name)

    if num_entries < 0:
        num_entries = DEFAULT_ENTRIES

    items = []
    for entry in range(0, num_entries):
        item = {
            'id': f'Item{entry}',
            'value': random.randrange(10000),
            'timestamp': int(time.time())

        }
        logger.info(f'Generated {item}')
        items.append(item)

    _write_table_data(table, items)
    logger.info(f'Wrote {num_entries} values to {table_name}')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Pre-populate a DynamoDB Table')
    parser.add_argument("--table_name", action="store", required=True)
    parser.add_argument("--region", action="store", dest='region', default="us-west-2")

    args = parser.parse_args()
    _session = boto3.Session(region_name=args.region)
    generate_entries(table_name=args.table_name, num_entries=10, session=_session)
