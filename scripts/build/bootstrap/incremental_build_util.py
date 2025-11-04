# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import argparse
import boto3
from botocore.utils import IMDSFetcher
import datetime
import os
import psutil
import time
import subprocess
import sys
import tempfile
from contextlib import contextmanager
import threading
import _thread

from botocore.config import Config

DEFAULT_REGION = 'us-west-2'
DEFAULT_DISK_SIZE = 300
DEFAULT_DISK_TYPE = 'gp3'
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
        return os.path.isdir(path) and (
                    ctypes.windll.kernel32.GetFileAttributesW(str(path)) & FILE_ATTRIBUTE_REPARSE_POINT)


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

        return free.value / 1024 / 1024  # for now
else:
    def get_free_space_mb(dirname):
        st = os.statvfs(dirname)
        return st.f_bavail * st.f_frsize / 1024 / 1024


def error(message):
    print(message)
    exit(1)


@contextmanager
def timeout(duration, timeout_message):
    timer = threading.Timer(duration, lambda: _thread.interrupt_main())
    timer.start()
    try:
        yield
    except KeyboardInterrupt:
        print(timeout_message)
        raise TimeoutError
    finally:
        # If the action ends in specified time, timer is canceled
        timer.cancel()


def print_drives():
    if os.name == 'nt':
        drives_before = win32api.GetLogicalDriveStrings()
        drives_before = drives_before.split('\000')[:-1]
        print(drives_before)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-a', '--action', dest="action", help="Action (mount|unmount|delete)")
    parser.add_argument('-snapshot-hint', '--snapshot-hint', dest="snapshot_hint", help="Build snapshot to attempt to use")
    parser.add_argument('-repository_name', '--repository_name', dest="repository_name", help="Repository name")
    parser.add_argument('-project', '--project', dest="project", help="Project")
    parser.add_argument('-pipe', '--pipeline', dest="pipeline", help="Pipeline")
    parser.add_argument('-b', '--branch', dest="branch", help="Branch")
    parser.add_argument('-plat', '--platform', dest="platform", help="Platform")
    parser.add_argument('-c', '--build_type', dest="build_type", help="Build type")
    parser.add_argument('-ds', '--disk_size', dest="disk_size",
                        help=f"Disk size in Gigabytes (defaults to {DEFAULT_DISK_SIZE})", default=DEFAULT_DISK_SIZE)
    parser.add_argument('-dt', '--disk_type', dest="disk_type", help=f"Disk type (defaults to {DEFAULT_DISK_TYPE})",
                        default=DEFAULT_DISK_TYPE)
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
    mount_name = f"{repository_name}_{project}_{pipeline}_{branch}_{platform}_{build_type}"
    mount_name = mount_name.replace('/', '_').replace('\\', '_')
    return mount_name


def get_pipeline_and_branch(pipeline, branch):
    pipeline_and_branch = f"{pipeline}_{branch}"
    pipeline_and_branch = pipeline_and_branch.replace('/', '_').replace('\\', '_')
    return pipeline_and_branch


def get_region_name():
    session = boto3.session.Session()
    region = session.region_name
    if region is None:
        region = DEFAULT_REGION
    return region


def get_ec2_client(region):
    client_config = Config(
        region_name=region,
        retries={
            'mode': 'standard'
        }
    )
    client = boto3.client('ec2', config=client_config)
    return client


def get_ec2_resource(region):
    resource_config = Config(
        region_name=region,
        retries={
            'mode': 'standard'
        }
    )
    resource = boto3.resource('ec2', config=resource_config)
    return resource


def get_ec2_instance_id():
    try:
        token = IMDSFetcher()._fetch_metadata_token()
        instance_id = IMDSFetcher()._get_request("/latest/meta-data/instance-id", None, token).text
        return instance_id
    except Exception as e:
        print(e)
        error('No EC2 metadata! Check if you are running this script on an EC2 instance.')


def get_availability_zone():
    try:
        token = IMDSFetcher()._fetch_metadata_token()
        availability_zone = IMDSFetcher()._get_request("/latest/meta-data/placement/availability-zone", None, token).text
        return availability_zone
    except Exception as e:
        print(e)
        error('No EC2 metadata! Check if you are running this script on an EC2 instance.')


def kill_processes(workspace='/data'):
    """
    Kills all processes that have open file paths associated with the workspace.
    Uses PSUtil for cross-platform compatibility
    """
    print('Checking for any stuck processes...')
    for proc in psutil.process_iter():
        try:
            if workspace in str(proc.open_files()):
                print(f"{proc.name()} has open files in {proc.open_files()}. Terminating")
                proc.kill()
                time.sleep(1)  # Just to make sure a parent process has time to close
        except (psutil.NoSuchProcess, psutil.AccessDenied, psutil.ZombieProcess):
            continue


def delete_volume(ec2_client, volume_id):
    response = ec2_client.delete_volume(VolumeId=volume_id)
    print(f'Volume {volume_id} deleted')

def find_snapshot_id(ec2_client, snapshot_hint, repository_name, project, pipeline, platform, build_type, disk_size):
    mount_name = get_mount_name(repository_name, project, pipeline, snapshot_hint, platform, build_type)
    response = ec2_client.describe_snapshots(Filters= [{
        'Name': 'tag:Name', 'Values': [mount_name]
    }])

    snapshot_id = None
    if 'Snapshots' in response and len(response['Snapshots']) > 0:
        snapshot_start_time_max = None  # find the latest snapshot
        for snapshot in response['Snapshots']:
            if snapshot['State'] == 'completed' and snapshot['VolumeSize'] == disk_size:
                snapshot_start_time = snapshot['StartTime']
                if not snapshot_start_time_max or snapshot_start_time > snapshot_start_time_max:
                    snapshot_start_time_max = snapshot_start_time
                    snapshot_id = snapshot['SnapshotId']
    return snapshot_id


def offline_drive(disk_number=1):
    """Use diskpart to offline a Windows drive"""
    with tempfile.NamedTemporaryFile(delete=False) as f:
        f.write(f"""
        select disk {disk_number}
        offline disk
        """.encode('utf-8'))
    subprocess.run(['diskpart', '/s', f.name])
    os.unlink(f.name)


def create_volume(ec2_client, availability_zone, snapshot_hint, repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type):
    # The actual EBS default calculation for IOps is a floating point number, the closest approxmiation is 4x of the disk size for simplicity
    mount_name = get_mount_name(repository_name, project, pipeline, branch, platform, build_type)
    pipeline_and_branch = get_pipeline_and_branch(pipeline, branch)
    parameters = dict(
        AvailabilityZone=availability_zone,
        VolumeType=disk_type,
        Encrypted=True,
        TagSpecifications= [{
            'ResourceType': 'volume',
            'Tags': [
                {'Key': 'Name', 'Value': mount_name},
                {'Key': 'RepositoryName', 'Value': repository_name},
                {'Key': 'Project', 'Value': project},
                {'Key': 'Pipeline', 'Value': pipeline},
                {'Key': 'BranchName', 'Value': branch},
                {'Key': 'Platform', 'Value': platform},
                {'Key': 'BuildType', 'Value': build_type},
                # Used so the snapshoting easily identifies which volumes to snapshot
                {'Key': 'PipelineAndBranch', 'Value': pipeline_and_branch},

            ]
        }]
    )
    # The actual EBS default calculation for IOps is a floating point number,
    # the closest approxmiation is 4x of the disk size for simplicity
    if 'io1' in disk_type.lower():
        parameters['Iops'] = (4 * disk_size)

    snapshot_id = find_snapshot_id(ec2_client, snapshot_hint, repository_name, project, pipeline, platform, build_type, disk_size)
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
    with timeout(DEFAULT_TIMEOUT, 'ERROR: Timeout reached trying to create EBS.'):
        while response['Volumes'][0]['State'] != 'available':
            time.sleep(1)
            response = ec2_client.describe_volumes(VolumeIds=[volume_id, ])

    print(f"Volume {volume_id} created\n\tSnapshot: {snapshot_id}\n\tRepository {repository_name}\n\t"
          f"Project {project}\n\tPipeline {pipeline}\n\tBranch {branch}\n\tPlatform: {platform}\n\tBuild type: {build_type}")
    return volume_id, created


def mount_volume_to_device(created):
    print('Mounting volume...')
    if os.name == 'nt':
        # Verify drive is in an offline state.
        # Some Windows configs will automatically set new drives as online causing diskpart setup script to fail.
        offline_drive()

        with tempfile.NamedTemporaryFile(delete=False) as f:
            f.write("""
            select disk 1
            online disk
            attribute disk clear readonly
            """.encode('utf-8'))  # assume disk # for now

            if created:
                print('Creating filesystem on new volume')
                f.write("""
                create partition primary
                select partition 1
                format quick fs=ntfs
                assign
                active
                """.encode('utf-8'))

        subprocess.call(['diskpart', '/s', f.name])

        time.sleep(5)

        print_drives()

        os.unlink(f.name)

        time.sleep(1)

    else:
        device_name = '/dev/xvdf'
        nvme_device_name = '/dev/nvme1n1'
        if os.path.exists(nvme_device_name):
            device_name = nvme_device_name
        subprocess.call(['file', '-s', device_name])
        if created:
            subprocess.call(['mkfs', '-t', 'ext4', device_name])
        subprocess.call(['mount', device_name, MOUNT_PATH])


def attach_volume_to_ec2_instance(volume, volume_id, instance_id, timeout_duration=DEFAULT_TIMEOUT):
    print(f'Attaching volume {volume_id} to instance {instance_id}')
    volume.attach_to_instance(Device='xvdf',
                              InstanceId=instance_id,
                              VolumeId=volume_id)
    # give a little bit of time for the aws call to process
    time.sleep(2)
    # reload the volume just in case
    volume.load()
    with timeout(timeout_duration, 'ERROR: Timeout reached trying to mount EBS.'):
        while len(volume.attachments) and volume.attachments[0]['State'] != 'attached':
            time.sleep(1)
            volume.load()
    volume.create_tags(
        Tags=[
            {
                'Key': 'AttachDate',
                'Value': str(datetime.datetime.today().date())
            },
        ]
    )
    print(f'Volume {volume_id} has been attached to instance {instance_id}')


def unmount_volume_from_device():
    print('Unmounting EBS volume from device...')
    if os.name == 'nt':
        kill_processes(MOUNT_PATH + 'workspace')
        offline_drive()
    else:
        kill_processes(MOUNT_PATH)
        subprocess.call(['umount', '-fl', MOUNT_PATH])


def detach_volume_from_ec2_instance(volume, ec2_instance_id, force, timeout_duration=DEFAULT_TIMEOUT):
    print(f'Detaching volume {volume.volume_id} from instance {ec2_instance_id}')
    volume.detach_from_instance(Device='xvdf',
                                Force=force,
                                InstanceId=ec2_instance_id,
                                VolumeId=volume.volume_id)
    try:
        with timeout(timeout_duration, 'ERROR: Timeout reached trying to unmount EBS.'):
            while len(volume.attachments) and volume.attachments[0]['State'] != 'detached':
                time.sleep(1)
                volume.load()
    except TimeoutError:
        print('Force detaching EBS.')
        volume.detach_from_instance(Device='xvdf', Force=True, InstanceId=ec2_instance_id, VolumeId=volume.volume_id)

    print(f'Volume {volume.volume_id} has been detached from instance {ec2_instance_id}')
    volume.load()
    if len(volume.attachments):
        print('Volume still has attachments')
        for attachment in volume.attachments:
            print(f"Volume {attachment['VolumeId']} {attachment['State']} to instance {attachment['InstanceId']}")


def mount_ebs(snapshot_hint, repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type):
    region = get_region_name()
    ec2_client = get_ec2_client(region)
    ec2_instance_id = get_ec2_instance_id()
    ec2_availability_zone = get_availability_zone()
    ec2_resource = get_ec2_resource(region)
    ec2_instance = ec2_resource.Instance(ec2_instance_id)

    for volume in ec2_instance.volumes.all():
        for attachment in volume.attachments:
            print(f"attachment device: {attachment['Device']}")
            if 'xvdf' in attachment['Device'] and attachment['State'] != 'detached':
                print('A device is already attached to xvdf. This likely means a previous build failed to detach its '
                      'build volume. This volume is considered orphaned and will be detached from this instance.')
                unmount_volume_from_device()
                detach_volume_from_ec2_instance(volume, ec2_instance_id,
                                                False)  # Force unmounts should not be used, as that will cause the EBS block device driver to fail the remount

    mount_name = get_mount_name(repository_name, project, pipeline, branch, platform, build_type)
    response = ec2_client.describe_volumes(Filters=[{
        'Name': 'tag:Name', 'Values': [mount_name]
    }])

    created = False
    if 'Volumes' in response and not len(response['Volumes']):
        print(f'Volume for {mount_name} doesn\'t exist creating it...')
        # volume doesn't exist, create it
        volume_id, created = create_volume(ec2_client, ec2_availability_zone, snapshot_hint, repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type)
    else:
        volume = response['Volumes'][0]
        volume_id = volume['VolumeId']
        print(f"Current volume {volume_id} is a {volume['Size']} GB {volume['VolumeType']}")
        if volume['Size'] != disk_size or volume['VolumeType'] != disk_type:
            print(
                f'Override disk attributes does not match the existing volume, deleting {volume_id} and replacing the volume')
            delete_volume(ec2_client, volume_id)
            volume_id, created = create_volume(ec2_client, ec2_availability_zone, snapshot_hint, repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type)
        if len(volume['Attachments']):
            # this is bad we shouldn't be attached, we should have detached at the end of a build
            attachment = volume['Attachments'][0]
            print(f'Volume already has attachment {attachment}, detaching...')
            detach_volume_from_ec2_instance(ec2_resource.Volume(volume_id), attachment['InstanceId'], True)

    volume = ec2_resource.Volume(volume_id)

    print_drives()
    attach_volume_to_ec2_instance(volume, volume_id, ec2_instance_id)
    mount_volume_to_device(created)
    print_drives()

    free_space_mb = get_free_space_mb(MOUNT_PATH)
    print(f'Free disk space {free_space_mb}MB')

    if free_space_mb < LOW_EBS_DISK_SPACE_LIMIT:
        print(f'Volume is running below EBS free disk space treshhold {LOW_EBS_DISK_SPACE_LIMIT}MB. Recreating volume and running clean build.')
        unmount_volume_from_device()
        detach_volume_from_ec2_instance(volume, ec2_instance_id, False)
        delete_volume(ec2_client, volume_id)
        new_disk_size = int(volume.size * 1.25)
        if new_disk_size > MAX_EBS_DISK_SIZE:
            print(f'Error: EBS disk size reached to the allowed maximum disk size {MAX_EBS_DISK_SIZE}MB, please contact ly-infra@ and ly-build@ to investigate.')
            exit(1)
        print('Recreating the EBS with disk size {}'.format(new_disk_size))
        volume_id, created = create_volume(ec2_client, ec2_availability_zone, snapshot_hint, repository_name, project, pipeline, branch, platform, build_type, new_disk_size, disk_type)
        volume = ec2_resource.Volume(volume_id)
        attach_volume_to_ec2_instance(volume, volume_id, ec2_instance_id)
        mount_volume_to_device(created)


def unmount_ebs():
    region = get_region_name()
    ec2_instance_id = get_ec2_instance_id()
    ec2_resource = get_ec2_resource(region)
    ec2_instance = ec2_resource.Instance(ec2_instance_id)

    if os.path.isfile('envinject.properties'):
        os.remove('envinject.properties')

    volume = None

    for attached_volume in ec2_instance.volumes.all():
        for attachment in attached_volume.attachments:
            print(f"attachment device: {attachment['Device']}")
            if attachment['Device'] == 'xvdf':
                volume = attached_volume

    if not volume:
        # volume is not mounted
        print('Volume is not mounted')
    else:
        unmount_volume_from_device()
        detach_volume_from_ec2_instance(volume, ec2_instance_id, False)


def delete_ebs(repository_name, project, pipeline, branch, platform, build_type):
    unmount_ebs()
    region = get_region_name()
    ec2_client = get_ec2_client(region)

    mount_name = get_mount_name(repository_name, project, pipeline, branch, platform, build_type)
    response = ec2_client.describe_volumes(Filters=[
        {'Name': 'tag:Name', 'Values': [mount_name]}
    ])

    if 'Volumes' in response and len(response['Volumes']):
        volume = response['Volumes'][0]
        volume_id = volume['VolumeId']
        delete_volume(ec2_client, volume_id)


def main(action, snapshot_hint, repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type):
    if action == 'mount':
        mount_ebs(snapshot_hint, repository_name, project, pipeline, branch, platform, build_type, disk_size, disk_type)
    elif action == 'unmount':
        unmount_ebs()
    elif action == 'delete':
        delete_ebs(repository_name, project, pipeline, branch, platform, build_type)


if __name__ == "__main__":
    args = parse_args()
    ret = main(args.action, args.snapshot_hint, args.repository_name, args.project, args.pipeline, args.branch, args.platform, args.build_type, args.disk_size, args.disk_type)
    sys.exit(ret)
