#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import boto3
import logging
import sys
from botocore.config import Config

log = logging.getLogger(__name__)
log.setLevel(logging.INFO)
log.addHandler(logging.StreamHandler())

DEFAULT_REGION = 'us-west-2'
DEFAULT_SNAPSHOT_RETAIN = 2
DEFAULT_SNAPSHOT_DESCRIPTION = 'Created for Build Artifact Snapshots'
DEFAULT_DRYRUN = True

def _kv_to_dict(kv_string):
    """
    Simple splitting of a key value string to dictionary in "Name: <Key>, Values: [<value>]" form

    :param kv_string: String in the form of "key:value"
    :return Dictionary of values
    """
    dict = {}

    if ":" not in kv_string:
        log.error(f'Keyvalue parameter not in the form of "key:value"')
        raise ValueError
    
    kv = kv_string.split(':')
    dict['Name'] = f'tag:{kv[0]}'
    dict['Values'] = [kv[1]]

    return dict

def _format_tags(tag_keyvalue):
    """
    Format tags in list form
    
    :param tag_keyvalue: String of comma separated key value pairs
    :return List of dictionary values
    """
    tag_filter = []
    
    for keyvalue in tag_keyvalue:
        tag_filter.append(_kv_to_dict(keyvalue))

    return tag_filter

def get_ec2_resource():
    """
    Get the AWS EC2 resource object, with appropriate region

    :return The EC2 resource object
    """
    session = boto3.session.Session()
    region = session.region_name
    if region is None:
        region = DEFAULT_REGION

    resource_config = Config(
        region_name=region,
        retries={
            'mode': 'standard'
        }
    )
    resource = boto3.resource('ec2', config=resource_config)
    return resource

def create_snapshot(ec2_resource, tag_keyvalue, snap_description, snap_dryrun=DEFAULT_DRYRUN):
    """
    Find and snapshot all EBS volumes that have a matching tag value. Injects all volume tags into the snapshot, 
    including the name and adds a description

    :param ec2_resource: The EC2 resource object
    :param tag_keyvalue: List of Strings with tag keyvalues in the form of "key:value"
    :param snap_description: String with the snapshot description to write
    :param snap_dryrun: Boolean to dryrun the action. Set to true by default (always dryrun)
    :return: Number of EBS volumes that are snapshotted successfully, number of EBS volumes that failed to be snapshotted
    """
    success = 0
    failure = 0

    tags = _format_tags(tag_keyvalue)

    for tag in tags:
        response = ec2_resource.volumes.filter(Filters=[tag])
        log.info(f'Snapshotting EBS volumes with tags that match {tag}...')
        for volume in response:
            try:
                log.info(f'Snapshotting volume {volume.volume_id}')
                volume.create_snapshot(Description=snap_description, TagSpecifications=[{'ResourceType': 'snapshot', 'Tags': volume.tags}], DryRun=snap_dryrun)
                success += 1
            except Exception as e:
                log.error(f'Failed to snapshot volume {volume.volume_id}.')
                log.error(e)
                failure += 1

    return success, failure

def delete_snapshot(ec2_resource, tag_keyvalue, snap_description, snap_retention, snap_dryrun=DEFAULT_DRYRUN):
    """
    Find all EBS snapshots that have a matching tag value AND description. If the number of snapshots exceeds a retention amount, 
    delete the oldest snapshot until retention is achived.

    :param ec2_resource: The EC2 resource object
    :param tag_keyvalue: List of Strings with tag keyvalues in the form of "key:value"
    :param snap_description: String with the snapshot description to search
    :param snap_retention: Integer with the number of snapshots to retain
    :param snap_dryrun: Boolean to dryrun the action. Set to true by default (always dryrun)
    :return: Number of EBS snapshots deleted successfully, number of EBS snapshots that failed to be deleted
    """
    success = 0
    failure = 0
    description_filter = {"Name": "description", "Values": [snap_description]}
    
    tags = _format_tags(tag_keyvalue)

    for tag in tags:
        response = list(ec2_resource.snapshots.filter(Filters=[tag,description_filter]))
        log.info(f'Getting snapshots with tags that match {tag}...')
        num_snaps = len(response)
        log.info(f'Tag {tag} has {num_snaps} snapshots')

        if num_snaps > snap_retention:
            log.info(f'Deleting oldest snapshots to keep retention of {snap_retention}')
            snap_list = sorted(response, key=lambda k: k.start_time) # Get a sorted list of snapshots by start time in descending order
            diff_snap = num_snaps - snap_retention
            for n in range(diff_snap):
                try:
                    log.info(f'Deleting snapshot {snap_list[n].snapshot_id}')
                    snap_list[n].delete(DryRun=snap_dryrun)
                    success += 1
                except Exception as e:
                    log.error(f'Failed to delete snapshot {snap_list[n].snapshot_id}.')
                    log.error(e)
                    failure += 1

    return success, failure

def list_snapshot(ec2_resource, tag_keyvalue, snap_description):
    """
    Find all EBS snapshots that have a matching tag value AND description. Prints snap id, description, tags, and start time.

    :param ec2_resource: The EC2 resource object
    :param tag_keyvalue: List of Strings with tag keyvalues in the form of "key:value"
    :param snap_description: String with the snapshot description to search
    :return: None
    """

    description_filter = {"Name": "description", "Values": [snap_description]}
    
    tags = _format_tags(tag_keyvalue)

    for tag in tags:
        response = ec2_resource.snapshots.filter(Filters=[tag,description_filter])
        log.info(f'Getting snapshots with tags that match {tag}...')
        num_snaps = len(list(response))
        log.info(f'Tag {tag} has {num_snaps} snapshots')
        snap_list = sorted(response, key=lambda k: k.start_time)
        for n in range(num_snaps):
            print(f'Snap ID: {snap_list[n].snapshot_id} \n Description: {snap_list[n].description} \n Tags: {snap_list[n].tags} \n Start Time: {snap_list[n].start_time}')

    return None

def parse_args():
    parser = argparse.ArgumentParser(description='Script to manage EBS snapshots for build artifacts')
    parser.add_argument('--action', '-a', type=str, help='(create|delete|list) Creates, deletes, or lists EBS snapshots based on tag. Requires --tags argument')
    parser.add_argument('--tags', '-t', type=str, required=True, help='Comma separated key value tags to search for in the form of "key:value", for example, "PipelineAndBranch:default_development","PipelineAndBranch:default_development"')
    parser.add_argument('--description', '-d', default=DEFAULT_SNAPSHOT_DESCRIPTION, help=f'Snapshot description to write or search for. Defaults to "{DEFAULT_SNAPSHOT_DESCRIPTION}"')
    parser.add_argument('--retention', '-r', nargs="?", const=DEFAULT_SNAPSHOT_RETAIN, type=int, help=f'Integer with the number of snapshots to retain. Defaults to {DEFAULT_SNAPSHOT_RETAIN}')
    parser.add_argument('--execute', '-e', action='store_false', help=f'Execute the snapshot commands. This needs to be set, otherwise it will always dryrun')
    return parser.parse_args()

def main():
    args = parse_args()
    tag_list = args.tags.split(",")
    ec2_resource = get_ec2_resource()
    if 'create' in args.action:
        ret = create_snapshot(ec2_resource, tag_list, args.description, args.execute)
        log.info(f'{ret[0]} snapshots created, {ret[1]} snapshots failed')
    elif 'delete' in args.action:
        ret = delete_snapshot(ec2_resource, tag_list, args.description, args.retention, args.execute)
        log.info(f'{ret[0]} snapshots deleted, {ret[1]} snapshot deletions failed')
    elif 'list' in args.action:
        ret = list_snapshot(ec2_resource, tag_list, args.description)

if __name__ == "__main__":
    sys.exit(main())
    