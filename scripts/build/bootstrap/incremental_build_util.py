# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import argparse
import ast
import boto3
import datetime
import urllib.request, urllib.error, urllib.parse
import os
import psutil
import time
import requests
import subprocess
import sys
import tempfile
import traceback

DEFAULT_REGION = 'us-west-2'
DEFAULT_DISK_SIZE = 300
DEFAULT_DISK_TYPE = 'gp2'
DEFAULT_TIMEOUT = 300
MAX_EBS_MOUNTING_ATTEMPT = 3
LOW_EBS_DISK_SPACE_LIMIT = 10240
MAX_EBS_DISK_SIZE = DEFAULT_DISK_SIZE * 2

if os.name == 'nt':
    MOUNT_PATH = 'D:\\'
else:
    MOUNT_PATH = '/data'

if os.name == 'nt':
    import ctypes
    import win32api
    import collections
    import locale

    locale.setlocale(locale.LC_ALL, '')  # set locale to default to get thousands separators

    PULARGE_INTEGER = ctypes.POINTER(ctypes.c_ulonglong)  # Pointer to large unsigned integer
    kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel32.GetDiskFreeSpaceExW.argtypes = (ctypes.c_wchar_p,) + (PULARGE_INTEGER,) * 3

    class UsageTuple(collections.namedtuple('UsageTuple', 'total, used, free')):
        def __str__(self):
            # Add thousands separator to numbers displayed
            return self.__class__.__name__ + '(total={:n}, used={:n}, free={:n})'.format(*self)

    def is_dir_symlink(path):
        FILE_ATTRIBUTE_REPARSE_POINT = 0x0400
        return os.path.isdir(path) and (ctypes.windll.kernel32.GetFileAttributesW(str(path)) & FILE_ATTRIBUTE_REPARSE_POINT)

    def get_free_space_mb(path):
        if sys.version_info < (3,):  # Python 2?
            saved_conversion_mode = ctypes.set_conversion_mode('mbcs', 'strict')
        else:
            try:
                path = os.fsdecode(path)  # allows str or bytes (or os.PathLike in Python 3.6+)
            except AttributeError:  # fsdecode() not added until Python 3.2
                pass

        # Define variables to receive results when passed as "by reference" arguments
        _, total, free = ctypes.c_ulonglong(), ctypes.c_ulonglong(), ctypes.c_ulonglong()

        success = kernel32.GetDiskFreeSpaceExW(
            path, ctypes.byref(_), ctypes.byref(total), ctypes.byref(free))
        if not success:
            error_code = ctypes.get_last_error()

        if sys.version_info < (3,):  # Python 2?
            ctypes.set_conversion_mode(*saved_conversion_mode)  # restore conversion mode

        if not success:
            windows_error_message = ctypes.FormatError(error_code)
            raise ctypes.WinError(error_code, '{} {!r}'.format(windows_error_message, path))

        used = total.value - free.value

        return free.value / 1024 / 1024#for now
else:
    def get_free_space_mb(dirname):
        st = os.statvfs(dirname)
        return st.f_bavail * st.f_frsize / 1024 / 1024

def error(message):
    print(message)
    exit(1)

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--action', dest="action", help="Action (mount|unmount|delete)")
    parser.add_argument('-repository_name', '--repository_name', dest="repository_name", help="Repository name")
    parser.add_argument('-project', '--project', dest="project", help="Project")
    parser.add_argument('-pipe', '--pipeline', dest="pipeline", help="Pipeline")
    parser.add_argument('-b', '--branch', dest="branch", help="Branch")
    parser.add_argument('-plat', '--platform', dest="platform", help="Platform")
    parser.add_argument('-c', '--build_type', dest="build_type", help="Build type")
    parser.add_argument('-ds', '--disk_size', dest="disk_size", help="Disk size in Gigabytes (defaults to {})".format(DEFAULT_DISK_SIZE), default=DEFAULT_DISK_SIZE)
    parser.add_argument('-dt', '--disk_type', dest="disk_type", help="Disk type (defaults to {})".format(DEFAULT_DISK_TYPE), default=DEFAULT_DISK_TYPE)
    args = parser.parse_args()

    # Input validation
    if args.action is None:
        error('No action specified')
    args.action = args.action.lower()
    if args.action != 'unmount':
        if args.repository_name is None:
            error('No repository specified')
        if args.project is None:
            error('No project specified')
        if args.pipeline is None:
            error('No pipeline specified')
        if args.branch is None:
            error('No branch specified')
        if args.platform is None:
            error('No platform specified')
        if args.build_type is None:
            error('No build_type specified')
    
    return args

def get_mount_name(repository_name, project, pipeline, branch, platform, build_type):
    mount_name = "{}_{}_{}_{}_{}_{}".format(repository_name, project, pipeline, branch, platform, build_type)
    mount_name = mount_name.replace('/','_').replace('\\','_')
    return mount_name

def get_pipeline_and_branch(pipeline, branch):
    pipeline_and_branch = "{}_{}".format(pipeline, branch)
    pipeline_and_branch = pipeline_and_branch.replace('/','_').replace('\\','_')
    return pipeline_and_branch

def get_ec2_client(region):
    client = boto3.client('ec2', region_name=region)
    return client


def get_ec2_instance_id():
    try:
        instance_id = urllib.request.urlopen('http://169.254.169.254/latest/meta-data/instance-id').read()
        return instance_id.decode("utf-8")
    except Exception as e:
        print(e.message)
        error('No EC2 metadata! Check if you are running this script on an EC2 instance.')


def get_availability_zone():
    try:
        availability_zone = urllib.request.urlopen('http://169.254.169.254/latest/meta-data/placement/availability-zone').read()
        return availability_zone.decode("utf-8")
    except Exception as e:
        print(e.message)
        error('No EC2 metadata! Check if you are running this script on an EC2 instance.')


def kill_processes(workspace='/dev/'):
    '''
    Kills all processes that have open file paths associated with the workspace.
    Uses PSUtil for cross-platform compatibility
    '''
    print('Checking for any stuck processes...')
    for proc in psutil.process_iter():
        try:
            if workspace in str(proc.open_files()):
                print("{} has open files in {}. Terminating".format(proc.name(), proc.open_files()))
                proc.kill()
                time.sleep(1) # Just to make sure a parent process has time to close
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            continue


def delete_volume(ec2_client, volume_id):
    response = ec2_client.delete_volume(VolumeId=volume_id)
    print('Volume {} deleted'.format(volume_id))

def find_snapshot_id(ec2_client, repository_name, project, pipeline, platform, build_type, disk_size):
    mount_name = get_mount_name(repository_name, project, pipeline, 'stabilization_2106', platform, build_type) # we take snapshots out of stabilization_2106
    response = ec2_client.describe_snapshots(Filters= [{
        'Name': 'tag:Name', 'Values': [mount_name]
    }])

    snapshot_id = None
    if 'Snapshots' in response and len(response['Snapshots']) > 0:
        snapshot_start_time_max = None # find the latest snapshot
        for snapshot in response['Snapshots']:
            if snapshot['State'] == 'completed' and snapshot['VolumeSize'] == disk_size:
                snapshot_start_time = snapshot['StartTime']
                if not snapshot_start_time_max or snapshot_start_time > snapshot_start_time_max:
                    snapshot_start_time_max = snapshot_start_time
                    snapshot_id = snapshot['SnapshotId']
    return snapshot_id

def create_volume(ec2_client, availability_zone, repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type):
    # The actual EBS default calculation for IOps is a floating point number, the closest approxmiation is 4x of the disk size for simplicity
    mount_name = get_mount_name(repository_name, project, pipeline, branch, platform, build_type)
    pipeline_and_branch = get_pipeline_and_branch(pipeline, branch) 
    parameters = dict(
        AvailabilityZone = availability_zone,
        VolumeType=disk_type,
        TagSpecifications= [{
            'ResourceType': 'volume',
            'Tags': [
                { 'Key': 'Name', 'Value': mount_name },
                { 'Key': 'RepositoryName', 'Value': repository_name},
                { 'Key': 'Project', 'Value': project },
                { 'Key': 'Pipeline', 'Value': pipeline },
                { 'Key': 'BranchName', 'Value': branch },
                { 'Key': 'Platform', 'Value': platform },
                { 'Key': 'BuildType', 'Value': build_type }, 
                { 'Key': 'PipelineAndBranch', 'Value': pipeline_and_branch }, # used so the snapshoting easily identifies which volumes to snapshot
            ]
        }]
    )
    if 'io1' in disk_type.lower(): 
        parameters['Iops'] = (4 * disk_size)

    snapshot_id = find_snapshot_id(ec2_client, repository_name, project, pipeline, platform, build_type, disk_size)
    if snapshot_id:
        parameters['SnapshotId'] = snapshot_id
        created = False
    else:
        # If no snapshot id, we need to specify the size
        parameters['Size'] = disk_size
        created = True

    response = ec2_client.create_volume(**parameters)
    volume_id = response['VolumeId']

    # give some time for the creation call to complete
    time.sleep(1)

    response = ec2_client.describe_volumes(VolumeIds=[volume_id, ])
    while (response['Volumes'][0]['State'] != 'available'):
        time.sleep(1)
        response = ec2_client.describe_volumes(VolumeIds=[volume_id, ])

    print(("Volume {} created\n\tSnapshot: {}\n\tRepository {}\n\tProject {}\n\tPipeline {}\n\tBranch {}\n\tPlatform: {}\n\tBuild type: {}"
        .format(volume_id, snapshot_id, repository_name, project, pipeline, branch, platform, build_type)))
    return volume_id, created


def mount_volume(created):
    print('Mounting volume...')
    if os.name == 'nt':
        f = tempfile.NamedTemporaryFile(delete=False)
        f.write("""
      select disk 1
      online disk
      attribute disk clear readonly
      """.encode('utf-8')) # assume disk # for now

        if created:
            print('Creating filesystem on new volume')
            f.write("""create partition primary
          select partition 1
          format quick fs=ntfs
          assign
          active
          """.encode('utf-8'))

        f.close()
        
        subprocess.call(['diskpart', '/s', f.name])

        time.sleep(5)

        drives_after = win32api.GetLogicalDriveStrings()
        drives_after = drives_after.split('\000')[:-1]

        print(drives_after)

        #drive_letter = next(item for item in drives_after if item not in drives_before)
        drive_letter = MOUNT_PATH

        os.unlink(f.name)

        time.sleep(1)

    else:
        subprocess.call(['file', '-s', '/dev/xvdf'])
        if created:
            subprocess.call(['mkfs', '-t', 'ext4', '/dev/xvdf'])
        subprocess.call(['mount', '/dev/xvdf', MOUNT_PATH])


def attach_volume(volume, volume_id, instance_id, timeout=DEFAULT_TIMEOUT):
    print('Attaching volume {} to instance {}'.format(volume_id, instance_id))
    volume.attach_to_instance(Device='xvdf',
                              InstanceId=instance_id,
                              VolumeId=volume_id)
    # give a little bit of time for the aws call to process
    time.sleep(2)
    # reload the volume just in case
    volume.load()
    timeout_init = time.clock()
    while (len(volume.attachments) and volume.attachments[0]['State'] != 'attached'):
        time.sleep(1)
        volume.load()
        if (time.clock() - timeout_init) > timeout:
            print('ERROR: Timeout reached trying to mount EBS')
            exit(1)
    volume.create_tags(
        Tags=[
            {
                'Key': 'AttachDate',
                'Value': str(datetime.datetime.today().date())
            },
        ]
    )
    print('Volume {} has been attached to instance {}'.format(volume_id, instance_id))


def unmount_volume():
    print('Umounting volume...')
    if os.name == 'nt':
        kill_processes(MOUNT_PATH + 'workspace')
        f = tempfile.NamedTemporaryFile(delete=False)
        f.write("""
          select disk 1
          offline disk
          """.encode('utf-8'))
        f.close()
        subprocess.call('diskpart /s %s' % f.name)
        os.unlink(f.name)
    else:
        kill_processes(MOUNT_PATH)
        subprocess.call(['umount', '-f', MOUNT_PATH])


def detach_volume(volume, ec2_instance_id, force, timeout=DEFAULT_TIMEOUT):
    print('Detaching volume {} from instance {}'.format(volume.volume_id, ec2_instance_id))
    volume.detach_from_instance(Device='xvdf',
                                Force=force,
                                InstanceId=ec2_instance_id,
                                VolumeId=volume.volume_id)
    timeout_init = time.clock()
    while len(volume.attachments) and volume.attachments[0]['State'] != 'detached':
        time.sleep(1)
        volume.load()
        if (time.clock() - timeout_init) > timeout:
            print('ERROR: Timeout reached trying to unmount EBS.')
            volume.detach_from_instance(Device='xvdf',Force=True,InstanceId=ec2_instance_id,VolumeId=volume.volume_id)
            exit(1)
            
    print('Volume {} has been detached from instance {}'.format(volume.volume_id, ec2_instance_id))
    volume.load()
    if len(volume.attachments):
        print('Volume still has attachments')
        for attachment in volume.attachments:
            print('Volume {} {} to instance {}'.format(attachment['VolumeId'], attachment['State'], attachment['InstanceId']))


def attach_ebs_and_create_partition_with_retry(volume, volume_id, ec2_instance_id, created):
    attach_volume(volume, volume_id, ec2_instance_id)
    mount_volume(created)
    attempt = 1
    while attempt <= MAX_EBS_MOUNTING_ATTEMPT:
        if os.name == 'nt':
            drives_after = win32api.GetLogicalDriveStrings()
            drives_after = drives_after.split('\000')[:-1]
            if MOUNT_PATH not in drives_after:
                print('Disk partitioning failed, retrying...')
                unmount_volume()
                detach_volume(volume, ec2_instance_id, False)
                attach_volume(volume, volume_id, ec2_instance_id)
                mount_volume(created)
        attempt += 1

def mount_ebs(repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type):
    session = boto3.session.Session()
    region = session.region_name
    if region is None:
        region = DEFAULT_REGION
    ec2_client = get_ec2_client(region)
    ec2_instance_id = get_ec2_instance_id()
    ec2_availability_zone = get_availability_zone()
    ec2_resource = boto3.resource('ec2', region_name=region)
    ec2_instance = ec2_resource.Instance(ec2_instance_id)

    for volume in ec2_instance.volumes.all():
        for attachment in volume.attachments:
            print('attachment device: {}'.format(attachment['Device']))
            if 'xvdf' in attachment['Device'] and attachment['State'] != 'detached':
                print('A device is already attached to xvdf. This likely means a previous build failed to detach its ' \
                      'build volume. This volume is considered orphaned and will be detached from this instance.')
                unmount_volume()
                detach_volume(volume, ec2_instance_id, False) # Force unmounts should not be used, as that will cause the EBS block device driver to fail the remount

    mount_name = get_mount_name(repository_name, project, pipeline, branch, platform, build_type)
    response = ec2_client.describe_volumes(Filters=[{
        'Name': 'tag:Name', 'Values': [mount_name]
        }])

    created = False
    if 'Volumes' in response and not len(response['Volumes']):
        print('Volume for {} doesn\'t exist creating it...'.format(mount_name))
        # volume doesn't exist, create it
        volume_id, created = create_volume(ec2_client, ec2_availability_zone, repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type)
    else:
        volume = response['Volumes'][0]
        volume_id = volume['VolumeId']
        print('Current volume {} is a {} GB {}'.format(volume_id, volume['Size'], volume['VolumeType']))
        if (volume['Size'] != disk_size or volume['VolumeType'] != disk_type):
            print('Override disk attributes does not match the existing volume, deleting {} and replacing the volume'.format(volume_id))
            delete_volume(ec2_client, volume_id)
            volume_id, created = create_volume(ec2_client, ec2_availability_zone, repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type)
        if len(volume['Attachments']):
            # this is bad we shouldn't be attached, we should have detached at the end of a build
            attachment = volume['Attachments'][0]
            print(('Volume already has attachment {}, detaching...'.format(attachment)))
            detach_volume(ec2_resource.Volume(volume_id), attachment['InstanceId'], True)

    volume = ec2_resource.Volume(volume_id)

    if os.name == 'nt':
        drives_before = win32api.GetLogicalDriveStrings()
        drives_before = drives_before.split('\000')[:-1]

        print(drives_before)

    attach_ebs_and_create_partition_with_retry(volume, volume_id, ec2_instance_id, created)

    free_space_mb = get_free_space_mb(MOUNT_PATH)
    print('Free disk space {}MB'.format(free_space_mb))
    
    if free_space_mb < LOW_EBS_DISK_SPACE_LIMIT:
        print('Volume is running below EBS free disk space treshhold {}MB. Recreating volume and running clean build.'.format(LOW_EBS_DISK_SPACE_LIMIT))
        unmount_volume()
        detach_volume(volume, ec2_instance_id, False)
        delete_volume(ec2_client, volume_id)
        new_disk_size = int(volume.size * 1.25)
        if new_disk_size > MAX_EBS_DISK_SIZE:
            print('Error: EBS disk size reached to the allowed maximum disk size {}MB, please contact ly-infra@ and ly-build@ to investigate.'.format(MAX_EBS_DISK_SIZE))
            exit(1)
        print('Recreating the EBS with disk size {}'.format(new_disk_size))
        volume_id, created = create_volume(ec2_client, ec2_availability_zone, repository_name, project, pipeline, branch, platform, build_type, new_disk_size, disk_type)
        volume = ec2_resource.Volume(volume_id)
        attach_ebs_and_create_partition_with_retry(volume, volume_id, ec2_instance_id, created)

def unmount_ebs():
    session = boto3.session.Session()
    region = session.region_name
    if region is None:
        region = DEFAULT_REGION
    ec2_client = get_ec2_client(region)
    ec2_instance_id = get_ec2_instance_id()
    ec2_resource = boto3.resource('ec2', region_name=region)
    ec2_instance = ec2_resource.Instance(ec2_instance_id)

    if os.path.isfile('envinject.properties'):
        os.remove('envinject.properties')

    volume = None

    for attached_volume in ec2_instance.volumes.all():
        for attachment in attached_volume.attachments:
            print('attachment device: {}'.format(attachment['Device']))
            if attachment['Device'] == 'xvdf':
                volume = attached_volume

    if not volume:
        # volume is not mounted
        print('Volume is not mounted')
    else:
        unmount_volume()
        detach_volume(volume, ec2_instance_id, False)

def delete_ebs(repository_name, project, pipeline, branch, platform, build_type):
    unmount_ebs()

    session = boto3.session.Session()
    region = session.region_name
    if region is None:
        region = DEFAULT_REGION
    ec2_client = get_ec2_client(region)
    ec2_instance_id = get_ec2_instance_id()
    ec2_resource = boto3.resource('ec2', region_name=region)
    ec2_instance = ec2_resource.Instance(ec2_instance_id)

    mount_name = get_mount_name(repository_name, project, pipeline, branch, platform, build_type)
    response = ec2_client.describe_volumes(Filters=[
        { 'Name': 'tag:Name', 'Values': [mount_name] }
    ])

    if 'Volumes' in response and len(response['Volumes']):
        volume = response['Volumes'][0]
        volume_id = volume['VolumeId']
        delete_volume(ec2_client, volume_id)


def main(action, repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type):
    if action == 'mount':
        mount_ebs(repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type)
    elif action == 'unmount':
        unmount_ebs()
    elif action == 'delete':
        delete_ebs(repository_name, project, pipeline, branch, platform, build_type)

if __name__ == "__main__":
    args = parse_args()
    ret = main(args.action, args.repository_name, args.project, args.pipeline, args.branch, args.platform, args.build_type, args.disk_size, args.disk_type)
    sys.exit(ret)