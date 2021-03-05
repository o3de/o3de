"""

 All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 its licensors.

 For complete copyright and license terms please see the LICENSE at the root of this
 distribution (the "License"). All use of this software is governed by the License,
 or, if provided, by the license below or the license accompanying this file. Do not
 remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""

import ast
import boto3
import datetime
import urllib2
import os
import time
import subprocess
import sys
import tempfile
import traceback
import shutil
import platform
import stat

IAM_ROLE_NAME = 'ec2-jenkins-node'

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
        return os.path.isdir(path) and (ctypes.windll.kernel32.GetFileAttributesW(unicode(path)) & FILE_ATTRIBUTE_REPARSE_POINT)

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


def get_iam_role_credentials(role_name):
    security_metadata = None
    try:
        response = urllib2.urlopen(
            'http://169.254.169.254/latest/meta-data/iam/security-credentials/{0}'.format(role_name)).read()
        security_metadata = ast.literal_eval(response)
    except:
        print 'Unable to get iam role credentials'
        print traceback.print_exc()

    return security_metadata


def create_volume(ec2_client, availability_zone, project_name, volume_counter):
    response = ec2_client.create_volume(
        AvailabilityZone=availability_zone,
        Size=300,
        VolumeType='gp2',
        TagSpecifications=
        [
            {
                'ResourceType': 'volume',
                'Tags':
                    [
                        {
                            'Key': 'Name',
                            'Value': '{0}'.format(project_name)
                        },
                        {
                            'Key': 'VolumeCounter',
                            'Value': str(volume_counter)
                        }
                    ]
            }
        ]
    )
    print response
    volume_id = response['VolumeId']

    # give some time for the creation call to complete
    time.sleep(1)

    response = ec2_client.describe_volumes(VolumeIds=[volume_id, ])
    while (response['Volumes'][0]['State'] != 'available'):
        time.sleep(1)
        response = ec2_client.describe_volumes(VolumeIds=[volume_id, ])

    return volume_id


def delete_volume(ec2_client, volume_id):
    response = ec2_client.delete_volume(VolumeId=volume_id)


def unmount_build_volume_from_node():
    if os.name == 'nt':
        f = tempfile.NamedTemporaryFile(delete=False)
        f.write("""
          select disk 1
          offline disk
          """)
        f.close()

        subprocess.call('diskpart /s %s' % f.name)

        os.unlink(f.name)
    else:
        subprocess.call(['umount', '/data'])


def detach_volume_from_node(ec2_client, volume, instance_id, force):
    ec2_client.delete_tags(Resources=[volume.volume_id],
                           Tags=[
                               {
                                   'Key': 'jenkins_attachment_node',
                               },
                               {
                                   'Key': 'jenkins_attachment_time',
                               },
                               {
                                   'Key': 'jenkins_attachment_build'
                               }
                           ])

    incremental_keys = ['jenkins_attachment_node', 'jenkins_attachment_time', 'jenkins_attachment_build']

    volume.load()

    print 'searching for keys adding during incremental build: {}'.format(incremental_keys)

    while len(incremental_keys):
        tag_keys = set()
        for tag in volume.tags:
            tag_keys.add(tag['Key'])

        print 'found tags on instace {}'.format(tag_keys)

        for incremental_key in list(incremental_keys):
            if incremental_key not in tag_keys:
                print 'incremental key {} has been successfully removed'.format(incremental_key)
                incremental_keys.remove(incremental_key)

        volume.load()

    volume.detach_from_instance(Device='xvdf',
                                Force=force,
                                InstanceId=instance_id,
                                VolumeId=volume.volume_id)

    while (len(volume.attachments) and volume.attachments[0]['State'] != 'detached'):
        time.sleep(1)
        volume.load()

    volume.load()

    if (len(volume.attachments)):
        print 'Volume still has attachments'
        for attachment in volume.attachments:
            print 'Volume {} {} to instance {}'.format(attachment['VolumeId'], attachment['State'], attachment['InstanceId'])


def cleanup_node(workspace_name):
    if os.name == 'nt':
        jenkins_base = os.getenv('BASE')
        dev_path = '{}\\workspace\\{}\\dev'.format(jenkins_base, workspace_name)
    else:
        dev_path = '/home/lybuilder/ly/workspace/{}/dev'.format(workspace_name)

    if os.path.exists(dev_path):
        if os.name == 'nt':
            if is_dir_symlink(dev_path):
                print "removing symlink path {}".format(dev_path)
                os.rmdir(dev_path)
            else:
                # this shouldn't happen, but is here for sanity's sake, if we sync to the build node erroneously we want to clean it up if we can
                print "given symlink path was not a symlink, deleting the full tree to prevent future build failures"
                retcode = os.system('rmdir /S /Q {}'.format(dev_path))
                if retcode != 0:
                    raise Exception("rmdir failed to remove directory: {}".format(dev_path))
                return True
        else:
            if os.path.islink(dev_path):
                print "unlinking symlink path {}".format(dev_path)
                os.unlink(dev_path)
            else:
                print "given symlink path was not a symlink, deleting the full tree to prevent future build failures"
                os.chmod(dev_path, stat.S_IWUSR)
                shutil.rmtree(dev_path, ignore_errors=True)
                return True
        # check to make sure the directory was actually deleted
        if os.path.exists(dev_path):
            raise Exception("Failed to remove directory: {}".format(dev_path))
    return False


def setup_volume(workspace_name, created):
    if os.name == 'nt':
        f = tempfile.NamedTemporaryFile(delete=False)
        f.write("""
      select disk 1
      online disk 
      attribute disk clear readonly
      """) # assume disk # for now

        if created:
            f.write("""create partition primary
          select partition 1
          format quick fs=ntfs
          assign
          active
          """)

        f.close()

        subprocess.call(['diskpart', '/s', f.name])

        time.sleep(2)

        drives_after = win32api.GetLogicalDriveStrings()
        drives_after = drives_after.split('\000')[:-1]

        print drives_after

        #drive_letter = next(item for item in drives_after if item not in drives_before)
        drive_letter = 'D:\\'

        os.unlink(f.name)

        time.sleep(1)

        dev_path = '{}ly\workspace\{}\dev'.format(drive_letter, workspace_name)

    else:
        subprocess.call(['file', '-s', '/dev/xvdf'])
        if created:
            subprocess.call(['mkfs', '-t', 'ext4', '/dev/xvdf'])
        subprocess.call(['mount', '/dev/xvdf', '/data'])

        dev_path = '/data/ly/workspace/{}/dev'.format(workspace_name)

    return dev_path


def attach_volume_to_instance(volume, volume_id, instance_id, instance_name):
    volume.attach_to_instance(Device='xvdf',
                              InstanceId=instance_id,
                              VolumeId=volume_id)
    # give a little bit of time for the aws call to process
    time.sleep(2)

    # reload the volume just in case
    volume.load()

    while (len(volume.attachments) and volume.attachments[0]['State'] != 'attached'):
        time.sleep(1)
        volume.load()

    volume.create_tags(Tags=[
        {
            'Key':'last_attachment_time',
            'Value':datetime.datetime.utcnow().isoformat()
        }
    ])

    volume.create_tags(Tags=[
        {
            'Key':'jenkins_attachment_node',
            'Value':instance_name,
        },
        {
            'Key':'jenkins_attachment_time',
            'Value':datetime.datetime.utcnow().isoformat()
        },
        {
            'Key':'jenkins_attachment_build',
            'Value':os.getenv('BUILD_TAG')
        }
    ])


def prepare_incremental_build(workspace_name):
    job_name = os.getenv('JOB_NAME', None)
    clean_build = os.getenv('CLEAN_BUILD', 'false').lower() == 'true'

    android_home = os.getenv('ANDROID_HOME', None)
    if android_home is not None:
        path = os.getenv('PATH').split(';')
        print path

        java_home = os.getenv('JAVA_HOME', None)
        print java_home

        os.environ['LY_NDK_PATH'] = 'C:\\ly\\3rdParty\\android-ndk\\r12'
        print os.getenv('LY_NDK_PATH')

        path = [x for x in path if not (java_home in x or android_home in x)]
        print path

        path.append(java_home)
        path.append(android_home)
        path.append(os.getenv('LY_NDK_PATH'))
        print path

        os.environ['PATH'] = ';'.join(path)

    credentials = get_iam_role_credentials(IAM_ROLE_NAME)

    aws_access_key_id = None
    aws_secret_access_key = None
    aws_session_token = None

    if credentials is not None:
        keys = ['AccessKeyId', 'SecretAccessKey', 'Token']
        for key in keys:
            if key not in credentials:
                print 'Unable to find {0} in get_iam_role_credentials response {1}'.format(key, credentials)
                return

        aws_access_key_id = credentials['AccessKeyId']
        aws_secret_access_key = credentials['SecretAccessKey']
        aws_session_token = credentials['Token']

    session = boto3.session.Session()
    region = session.region_name

    try:
        instance_id = urllib2.urlopen('http://169.254.169.254/latest/meta-data/instance-id').read()
    except:
        # this likely means we're not an ec2 instance
        raise Exception('No EC2 metadata!')


    try:
        availability_zone = urllib2.urlopen(
            'http://169.254.169.254/latest/meta-data/placement/availability-zone').read()
    except:
        # also likely means we're not an ec2 instance
        raise Exception('No EC2 metadata')


    if region is None:
        region = 'us-west-2'

    client = boto3.client('ec2', region_name=region, aws_access_key_id=aws_access_key_id,
                          aws_secret_access_key=aws_secret_access_key,
                          aws_session_token=aws_session_token)

    project_name = job_name

    ec2_resource = boto3.resource('ec2', region_name=region)
    instance = ec2_resource.Instance(instance_id)

    volume_counter = 0

    for volume in instance.volumes.all():
        for attachment in volume.attachments:
            print 'attachment device: {}'.format(attachment['Device'])
            if 'xvdf' in attachment['Device'] and attachment['State'] != 'detached':
                print 'A device is already attached to xvdf. This likely means a previous build failed to detach it\'s' \
                      'build volume. This volume is considered orphaned and will be force detached from this instance.'
                unmount_build_volume_from_node()
                detach_volume_from_node(client, volume, instance_id, True)

    if cleanup_node(workspace_name):
        clean_build = True

    response = client.describe_volumes(Filters=
    [
        {
            'Name': 'tag:Name',
            'Values':
                [
                    '{0}'.format(project_name)
                ]
        }
    ])

    created = False

    if 'Volumes' in response and not len(response['Volumes']):
        print 'Volume for {0} doesn\'t exist creating it...'.format(project_name)
        # volume doesn't exist, create it
        volume_id = create_volume(client, availability_zone, project_name, volume_counter)
        created = True

    elif len(response['Volumes']) > 1:
        latest_volume = None
        max_counter = 0

        for volume in response['Volumes']:
            for tag in volume['Tags']:
                if tag['Key'] == 'VolumeCounter':
                    if int(tag['Value']) > max_counter:
                        max_counter = int(tag['Value'])
                        latest_volume = volume

        volume_counter = max_counter
        volume_id = latest_volume['VolumeId']
    else:
        volume = response['Volumes'][0]
        if len(volume['Attachments']):
            # this is bad we shouldn't be attached, we should have detached at the end of a build
            attachment = volume['Attachments'][0]
            print ('Volume already has attachment {}'.format(attachment))
            print 'Creating new volume for {} and orphaning previous volume'.format(project_name)

            for tag in volume['Tags']:
                if tag['Key'] == 'VolumeCounter':
                    volume_counter = int(tag['Value']) + 1
                    break

            volume_id = create_volume(client, availability_zone, project_name, volume_counter)
            created = True
        else:
            volume_id = volume['VolumeId']

    if clean_build and not created:
        print 'CLEAN_BUILD option was set, deleting volume {0}'.format(volume_id)
        revert_workspace(job_name)
        delete_volume(client, volume_id)
        volume_id = create_volume(client, availability_zone, project_name, volume_counter)
        created = True

    print 'attaching volume {} to instance {}'.format(volume_id, instance_id)
    volume = ec2_resource.Volume(volume_id)

    instance_name = next(tag['Value'] for tag in instance.tags if tag['Key'] == 'Name')

    if os.name == 'nt':
        drives_before = win32api.GetLogicalDriveStrings()
        drives_before = drives_before.split('\000')[:-1]

        print drives_before

    attach_volume_to_instance(volume, volume_id, instance_id, instance_name)

    dev_path = setup_volume(workspace_name, created)

    dev_existed = True

    if os.name == 'nt':
        free_space_path =  'D:\\'
    else:
        free_space_path = '/data/'

    if get_free_space_mb(free_space_path) < 1024:
        print 'Volume is running low on disk space. Recreating volume and running clean build.'
        unmount_build_volume_from_node()
        detach_volume_from_node(client, volume, instance_id, False)
        delete_volume(client, volume_id)

        volume_id = create_volume(client, availability_zone, project_name, volume_counter)
        volume = ec2_resource.Volume(volume_id)
        attach_volume_to_instance(volume, volume_id, instance_id, instance_name)
        setup_volume(workspace_name, True)

    if not os.path.exists(dev_path):
        print 'creating directory structure for {}'.format(dev_path)
        os.makedirs(dev_path)
        if os.name != 'nt':
            print 'taking ownership of {}'.format(dev_path)
            subprocess.call(['chown', '-R', 'lybuilder:root', dev_path])
        dev_existed = False

    if os.name == 'nt':
        jenkins_base = os.getenv('BASE')
        try:
            symlink_path = '{}\\workspace\\{}\\dev'.format(jenkins_base, workspace_name)
            print 'creating symlink to path: {}'.format(symlink_path)
            subprocess.call(['cmd', '/c', 'mklink', '/J', symlink_path, dev_path])
            #subprocess.call(['cmd', '/c', 'mklink', '/J', '{}\\3rdParty'.format(jenkins_base), 'E:\\3rdParty'])
        except Exception as e:
            print e
    else:
        subprocess.call(['ln', '-s', '-f', dev_path, '/home/lybuilder/ly/workspace/{}'.format(workspace_name)])
        subprocess.call(['ln', '-s', '-f', '/home/lybuilder/ly/workspace/3rdParty', '/data/ly/workspace'])

    if not dev_existed:
        print 'flushing perforce #have revision'
        subprocess.call(['p4', 'trust'])
        subprocess.call(['p4', 'flush', '-f', '//ly_jenkins_{}/dev/...#none'.format(job_name)])
        #subprocess.call(['p4', 'sync', '-f', '//ly_jenkins_{}/dev/...'.format(job_name)])


def revert_workspace(job_name):
    try:
        # Workaround for LY-86789: Revert bootstrap.cfg checkout.
        print "REVERTING workspace {}".format(job_name)
        subprocess.check_call(['p4', 'revert', '//ly_jenkins_{}/dev/...'.format(job_name)])
    except subprocess.CalledProcessError as e:
        print e.output
        raise e
    except Exception as e:
        print e
        raise e


def teardown_incremental_build(workspace_name):
    job_name = os.getenv('JOB_NAME', None)

    if os.path.isfile('envinject.properties'):
        os.remove('envinject.properties')

    credentials = get_iam_role_credentials(IAM_ROLE_NAME)

    aws_access_key_id = None
    aws_secret_access_key = None
    aws_session_token = None

    if credentials is not None:
        keys = ['AccessKeyId', 'SecretAccessKey', 'Token']
        for key in keys:
            if key not in credentials:
                raise Exception('Unable to find {0} in get_iam_role_credentials response {1}'.format(key, credentials))

        aws_access_key_id = credentials['AccessKeyId']
        aws_secret_access_key = credentials['SecretAccessKey']
        aws_session_token = credentials['Token']

    session = boto3.session.Session()
    region = session.region_name

    try:
        instance_id = urllib2.urlopen('http://169.254.169.254/latest/meta-data/instance-id').read()
    except:
        # this likely means we're not an ec2 instance
        raise Exception('No EC2 metadata!')

    if region is None:
        region = 'us-west-2'

    client = boto3.client('ec2', region_name=region, aws_access_key_id=aws_access_key_id,
                          aws_secret_access_key=aws_secret_access_key,
                          aws_session_token=aws_session_token)

    project_name = job_name
    response = client.describe_volumes(Filters=
    [
        {
            'Name': 'tag:Name',
            'Values':
                [
                    '{0}'.format(project_name)
                ]
        }
    ])

    ec2_resource = boto3.resource('ec2', region_name=region)
    instance = ec2_resource.Instance(instance_id)

    volume = None

    for attached_volume in instance.volumes.all():
        for attachment in attached_volume.attachments:
            print 'attachment device: {}'.format(attachment['Device'])
            if attachment['Device'] == 'xvdf':
                volume = attached_volume

    if volume is None:
        # volume doesn't exist, do nothing
        print 'Volume for {} does not exist or is not attached to the current instance. This probably isn\'t an issue but should be reported.'.format(project_name)
        return
    else:
        revert_workspace(job_name)

        unmount_build_volume_from_node()

        detach_volume_from_node(client, volume, instance_id, False)

        cleanup_node(workspace_name)


def prepare_incremental_build_mac(workspace_name):
    job_name = os.getenv('JOB_NAME', None)
    clean_build = os.getenv('CLEAN_BUILD', 'false').lower() == 'true'

    subprocess.call(['mount', '-t', 'smbfs', '//lybuilder:Builder99@gt-sna11-nas-01.local/inc-build/ly', '/data/ly'])

    dev_path = '/data/ly/workspace/{}/dev'.format(workspace_name)

    dev_existed = True

    if clean_build:
        print 'cleaning {}'.format(dev_path)
        subprocess.call(['rm', '-rf', dev_path])
    if not os.path.exists(dev_path):
        print 'creating directory structure for {}'.format(dev_path)
        os.makedirs(dev_path)
        dev_existed = False

    #subprocess.call(['ln', '-s', '-f', '/data/ly/workspace', '/Users/lybuilder'])

    #subprocess.call(['ln', '-s', '-f', '/Users/lybuilder/workspace/3rdParty', '/data/ly/workspace'])

    if not dev_existed:
        print 'flushing perforce #have revision'
        subprocess.call(['p4', 'trust'])
        subprocess.call(['p4', 'flush', '-f', '//ly_jenkins_{}/dev/...#none'.format(job_name)])


def main():
    action = sys.argv[1]
    workspace_name = sys.argv[2]

    if action.lower() == 'prepare':
        if platform.system().lower() == 'darwin':
            prepare_incremental_build_mac(workspace_name)
        else:
            prepare_incremental_build(workspace_name)
    elif action.lower() == 'teardown':
        if platform.system().lower() == 'darwin':
            pass
        else:
            teardown_incremental_build(workspace_name)
    else:
        'Invalid command. Valid actions are either "prepare" or "teardown."'


if __name__ == '__main__':
    main()
