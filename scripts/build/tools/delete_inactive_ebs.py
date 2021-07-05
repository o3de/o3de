#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import boto3
from datetime import datetime, timedelta
import logging
log = logging.getLogger(__name__)
log.setLevel(logging.INFO)

ATTACH_DATE_TAG_NAME = 'AttachDate'
INACTIVE_DAY_THRESHOLD = 5


def get_tag_values(tag_name):
    """
    Call Resource Group Tagging API to get all values of a certain tag
    :param tag_name: Tag name
    :return: List of all values of the tag
    """
    client = boto3.client('resourcegroupstaggingapi')
    tag_values = []
    try:
        response = client.get_tag_values(
            Key=tag_name
        )
        tag_values += response['TagValues']
        while response and response.get('PaginationToken', ''):
            response = client.get_tag_values(
                PaginationToken=response['PaginationToken'],
                Key=tag_name
            )
            tag_values += response['TagValues']
        return tag_values
    except Exception as e:
        log.error(f'Failed to get tag values for {tag_name}.')
        log.error(e)
        return []


def delete_volumes(inactive_dates):
    """
    Find and delete all EBS volumes that have AttachDate tag value in inactive_dates
    :param inactive_dates: List of String that considered as inactive dates, for example, ["2021-04-27", "2021-04-26"]
    :return: Number of EBS volumes that are deleted successfully, number of EBS volumes that are not deleted.
    """
    success = 0
    failure = 0
    ec2_client = boto3.resource('ec2')
    response = ec2_client.volumes.filter(Filters=[
        {
            'Name': f'tag:{ATTACH_DATE_TAG_NAME}',
            'Values': inactive_dates
        },
        {
            'Name': 'status',
            'Values': ['available']
        },
    ])
    log.info(f'Deleting inactive EBS volumes that haven\'t been used for {INACTIVE_DAY_THRESHOLD} days...')
    for volume in response:
        if volume.attachments:
            # Double check if volume has an attachment in case the volume is being used.
            continue
        try:
            log.info(f'Deleting volume {volume.volume_id}')
            volume.delete()
            success += 1
        except Exception as e:
            log.error(f'Failed to delete volume {volume.volume_id}.')
            log.error(e)
            failure += 1
    return success, failure


def main():
    tag_values = get_tag_values(ATTACH_DATE_TAG_NAME)
    date_today = datetime.today().date()
    # Exclude dates of last {INACTIVE_DAY_THRESHOLD} days from tag values, the rest values are considered as inactive dates.
    dates_to_exclude = []
    for i in range(INACTIVE_DAY_THRESHOLD):
        dates_to_exclude.append(str(date_today - timedelta(i)))
    inactive_dates = set(tag_values).difference(dates_to_exclude)
    if inactive_dates:
        success, failure = delete_volumes(list(inactive_dates))
        log.info(f'{success} EBS volumes are deleted successfully.')
        if failure:
            log.info(f'{failure} EBS volumes are not deleted due to errors. See logs for more information.')


if __name__ == "__main__":
    main()
