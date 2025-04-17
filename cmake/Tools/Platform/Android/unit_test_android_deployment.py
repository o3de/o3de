#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import datetime
import os
import pathlib
import platform
import pytest
import time

from unittest.mock import patch, Mock
from cmake.Tools.Platform.Android import android_deployment

TEST_GAME_NAME = "Foo"
TEST_DEV_ROOT = pathlib.Path("Foo")
TEST_ASSET_MODE = 'LOOSE'
TEST_ASSET_TYPE = 'android'
TEST_ANDROID_SDK_PATH = pathlib.Path('c:\\AndroidSDK')
TEST_BUILD_DIR = 'android_gradle_test'
TEST_DEVICE_ID = '9A201FFAZ000ER'


def match_arg_list(input_args, expected_args):
    if len(input_args) != len(expected_args):
        return False
    for index in range(len(input_args)):
        if input_args[index] != expected_args[index]:
            return False
    return True


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.read_android_settings', return_value={})
@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.resolve_adb_tool', return_value=pathlib.Path("Foo"))
def test_Initialize(mock_resolve_adb_tool, mock_read_android_settings):

    attrs = {'glob.return_value': ["foo.bar"]}
    mock_local_asset_path = Mock(**attrs)
    inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                build_dir=TEST_BUILD_DIR,
                                                configuration='profile',
                                                game_name=TEST_GAME_NAME,
                                                asset_mode=TEST_ASSET_MODE,
                                                asset_type=TEST_ASSET_TYPE,
                                                embedded_assets=True,
                                                android_device_filter=None,
                                                clean_deploy=False,
                                                android_sdk_path=TEST_ANDROID_SDK_PATH,
                                                deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH)

    mock_resolve_adb_tool.assert_called_once_with(TEST_ANDROID_SDK_PATH)
    mock_read_android_settings.assert_called_once_with(TEST_DEV_ROOT, TEST_GAME_NAME)
    assert inst


def test_read_android_settings(tmpdir):

    game_name = "Foo"
    package_name = "o3de.org.Foo"
    tmpdir.ensure(f'dev_root/{game_name}/Platform/Android/android_project.json')
    game_project_json_file = tmpdir.join(f'dev_root/{game_name}/Platform/Android/android_project.json')
    game_project_json_file.write(f'{{"android_settings": {{"package_name": "{package_name}" }} }}')

    result = android_deployment.AndroidDeployment.read_android_settings(pathlib.Path(tmpdir.join('dev_root').realpath()), game_name)
    assert result['package_name'] == package_name


def test_resolve_adb_tool(tmpdir):

    sdk_path = 'android_sdk'
    adb_target = 'adb.exe' if platform.system() == 'Windows' else 'adb'

    tmpdir.ensure(f'{sdk_path}/platform-tools/{adb_target}')
    dummy_adb_file = tmpdir.join(f'{sdk_path}/platform-tools/{adb_target}')
    dummy_adb_file.write('adb')

    result = android_deployment.AndroidDeployment.resolve_adb_tool(pathlib.Path(tmpdir.join(sdk_path).realpath()))
    adb_name = os.path.basename(result).lower()
    assert adb_name == adb_target


@patch('subprocess.check_output', return_value=b'PASS')
def test_adb_call(mock_check_output):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH)

        result = inst.adb_call(arg_list=['foo'], device_id='123')
        mock_check_output.assert_called_once()
        assert result == 'PASS'


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_call', return_value='PASS')
def test_adb_shell(mock_adb_call):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH)

        result = inst.adb_shell(command='foo me', device_id='123')
        expected_args = ['shell', 'foo me']
        mock_adb_call.assert_called_once_with(expected_args, device_id='123')
        assert result == 'PASS'


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_shell', return_value='output')
def test_adb_ls_success(mock_adb_shell):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH)

        result, output = inst.adb_ls(path='/foo/bar', device_id='123')

        mock_adb_shell.assert_called_once_with(command='ls /foo/bar', device_id='123')
        assert result
        assert output == 'output'


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_shell', return_value='')
def test_adb_ls_error_no_output(mock_adb_shell):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH)

        result, output = inst.adb_ls(path='/foo/bar', device_id='123')

        mock_adb_shell.assert_called_once_with(command='ls /foo/bar', device_id='123')
        assert not result
        assert output is None


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_shell', return_value='No such file or directory')
def test_adb_ls_error_no_such_file(mock_adb_shell):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH)

        result, output = inst.adb_ls(path='/foo/bar', device_id='123')

        mock_adb_shell.assert_called_once_with(command='ls /foo/bar', device_id='123')
        assert not result
        assert output == 'No such file or directory'


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_shell', return_value='Permission denied')
def test_adb_ls_error_permission_denied(mock_adb_shell):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        result, output = inst.adb_ls(path='/foo/bar', device_id='123')

        mock_adb_shell.assert_called_once_with(command='ls /foo/bar', device_id='123')
        assert not result
        assert output == 'Permission denied'


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_call',
       return_value=f'List of devices attached{os.linesep}9A201FFAZ000ER  device{os.linesep}1A201FFAZ000ER  device{os.linesep}9A201FFAZ456ER  unauthorized')
def test_get_target_android_devices(mock_adb_call):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=f'{TEST_DEVICE_ID},AAAAASDSFGG',
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        result = inst.get_target_android_devices()

        mock_adb_call.assert_called_once_with("devices")
        assert len(result) == 1
        assert result[0] == TEST_DEVICE_ID


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_ls', return_value=(True,"file"))
def test_check_known_android_paths_success(mock_adb_ls):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        result = inst.check_known_android_paths(device_id='123')

        mock_adb_ls.assert_called_once()
        assert result == android_deployment.KNOWN_ANDROID_EXTERNAL_STORAGE_PATHS[0][:-1]


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_ls', return_value=(False,None))
def test_check_known_android_paths_fail(mock_adb_ls):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        result = inst.check_known_android_paths(device_id='123')

        assert not result
        assert mock_adb_ls.call_count == len(android_deployment.KNOWN_ANDROID_EXTERNAL_STORAGE_PATHS)


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_shell', return_value=None)
@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.check_known_android_paths', return_value="PATH")
def test_detect_device_storage_path_no_external_storage_env(mock_check_known_android_paths, mock_adb_shell):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")),\
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        result = inst.detect_device_storage_path(device_id=TEST_DEVICE_ID)
        assert result == "PATH"
        mock_adb_shell.assert_called_once()
        mock_check_known_android_paths.assert_called_once_with(TEST_DEVICE_ID)


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_shell', return_value="NotSet")
@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.check_known_android_paths', return_value="PATH")
def test_detect_device_storage_path_invalid_external_storage_env(mock_check_known_android_paths, mock_adb_shell):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")),\
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        result = inst.detect_device_storage_path(device_id=TEST_DEVICE_ID)
        assert result == "PATH"
        mock_adb_shell.assert_called_once()
        mock_check_known_android_paths.assert_called_once_with(TEST_DEVICE_ID)


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_shell', return_value="EXTERNAL_STORAGE=/foo/bar")
@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_ls', return_value=(True, "foo.bar"))
def test_detect_device_storage_path_valid_external_storage_env(mock_adb_ls, mock_adb_shell):

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")),\
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        result = inst.detect_device_storage_path(device_id=TEST_DEVICE_ID)
        assert result == "/foo/bar"
        mock_adb_shell.assert_called_once()
        mock_adb_ls.assert_called_once_with(path='/foo/bar', device_id=TEST_DEVICE_ID)


def test_detect_device_storage_path_real_path():

    def _mock_adb_shell(command, device_id):
        if command == "set | grep EXTERNAL_STORAGE":
            return "EXTERNAL_STORAGE=/foo/bar"
        elif command == f'realpath /foo/bar':
            return "/foo_reals"
        else:
            raise AssertionError

    def _mock_adb_ls(path, device_id, args=None):
        if path == "/foo/bar":
            return False, None
        elif path == '/foo_reals':
            return True, "foo.bar"
        else:
            raise AssertionError

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(android_deployment.AndroidDeployment, 'adb_shell', wraps=_mock_adb_shell), \
         patch.object(android_deployment.AndroidDeployment, 'adb_ls', wraps=_mock_adb_ls), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        result = inst.detect_device_storage_path(device_id=TEST_DEVICE_ID)
        assert result == "/foo_reals"


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.check_known_android_paths', return_value="PATH")
def test_detect_device_storage_path_real_path_fail(mock_check_known_android_paths):

    def _mock_adb_shell(command, device_id):
        if command == "set | grep EXTERNAL_STORAGE":
            return "EXTERNAL_STORAGE=/foo/bar"
        elif command == f'realpath /foo/bar':
            return "/foo_reals"
        else:
            raise AssertionError

    def _mock_adb_ls(path, device_id, args=None):
        if path == "/foo/bar":
            return False, None
        elif path == '/foo_reals':
            return False, None
        else:
            raise AssertionError

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(android_deployment.AndroidDeployment, 'adb_shell', wraps=_mock_adb_shell), \
         patch.object(android_deployment.AndroidDeployment, 'adb_ls', wraps=_mock_adb_ls), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        result = inst.detect_device_storage_path(device_id=TEST_DEVICE_ID)
        assert result == "PATH"
        mock_check_known_android_paths.assert_called_once_with(TEST_DEVICE_ID)


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_shell', return_value="2020-04-30 09:20:00.0000")
def test_get_device_file_timestamp_success(mock_adb_shell):
    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        remote_path = "/foo/bar/timestamp.txt"
        result = inst.get_device_file_timestamp(remote_file_path=remote_path,
                                                device_id=TEST_DEVICE_ID)

        assert result == time.mktime(time.strptime("2020-04-30 09:20:00.0000", '%Y-%m-%d %H:%M:%S.%f'))

        mock_adb_shell.assert_called_once_with(command=f'cat {remote_path}',
                                               device_id=TEST_DEVICE_ID)


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_shell', return_value=None)
def test_get_device_file_timestamp_no_file(mock_adb_shell):
    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        remote_path = "/foo/bar/timestamp.txt"
        result = inst.get_device_file_timestamp(remote_file_path=remote_path,
                                                device_id=TEST_DEVICE_ID)

        assert not result

        mock_adb_shell.assert_called_once_with(command=f'cat {remote_path}',
                                               device_id=TEST_DEVICE_ID)


@patch('cmake.Tools.Platform.Android.android_deployment.AndroidDeployment.adb_shell', return_value="ZZZZXXXX")
def test_get_device_file_timestamp_bad_timestamp_file(mock_adb_shell):
    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(pathlib.Path, 'glob', return_value=["foo.bar"]):

        inst = android_deployment.AndroidDeployment(dev_root=TEST_DEV_ROOT,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=True,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        remote_path = "/foo/bar/timestamp.txt"
        result = inst.get_device_file_timestamp(remote_file_path=remote_path,
                                                device_id=TEST_DEVICE_ID)

        assert not result

        mock_adb_shell.assert_called_once_with(command=f'cat {remote_path}',
                                               device_id=TEST_DEVICE_ID)


def test_update_device_file_timestamp(tmpdir):

    tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/src/assets/deploy.timestamp").ensure()

    mock_dev_root = tmpdir.join(TEST_DEV_ROOT).realpath()

    with patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={}), \
            patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
            patch.object(android_deployment.AndroidDeployment, 'adb_call', return_value="") as mock_adb_call:

        inst = android_deployment.AndroidDeployment(dev_root=mock_dev_root,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration='profile',
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=False,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_ASSETS_ONLY,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        remote_path = "/foo/bar/timestamp.txt"
        inst.update_device_file_timestamp(relative_assets_path=remote_path,
                                          device_id=TEST_DEVICE_ID)

        mock_adb_call.assert_called()


@pytest.mark.parametrize(
    "test_config, test_package_name, test_device_storage_path", [
        pytest.param('profile', 'org.o3de.foo', '/data/fool_storage'),
        pytest.param('debug', 'org.o3de.foo', '/data/fool_storage'),
        pytest.param('profile', 'org.o3de.bar', '/data/fool_storage'),
        pytest.param('debug', 'org.o3de.bar', '/data/fool_storage'),
        pytest.param('profile', 'org.o3de.foo', '/data/fool_storage2'),
        pytest.param('debug', 'org.o3de.foo', '/data/fool_storage2'),
        pytest.param('profile', 'org.o3de.bar', '/data/fool_storage2'),
        pytest.param('debug', 'org.o3de.bar', '/data/fool_storage2')
    ]
)
def test_execute_success(tmpdir, test_config, test_package_name, test_device_storage_path):

    mock_dev_root = tmpdir.join(TEST_DEV_ROOT).realpath()

    tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/build/outputs/apk/{test_config}/app-{test_config}.apk").ensure()
    expected_apk_path = str(tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/build/outputs/apk/{test_config}/app-{test_config}.apk").realpath())

    tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/src/assets/dummy.txt").ensure()
    expected_asset_path = str(tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/src/assets").realpath())

    tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/src/main/assets/Registry/dummy.txt").ensure()
    expected_registry_path = str(tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/src/main/assets/Registry").realpath())

    expected_storage_registry_path = f'{test_device_storage_path}/Android/data/{test_package_name}/files/Registry'

    def _mock_adb_call(arg_list, device_id=None):
        if arg_list == "start-server":
            return "SUCCESS"
        if arg_list == "kill-server":
            return "SUCCESS"
        elif isinstance(arg_list, list):

            if match_arg_list(arg_list, ['install', '-r', expected_apk_path]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['install', '-t', '-r', expected_apk_path]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['push', f'{expected_asset_path}/.', f'{test_device_storage_path}/Android/data/{test_package_name}/files']):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['push', expected_registry_path, expected_storage_registry_path]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['shell', 'cmd', 'package', 'list', 'packages', test_package_name]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['shell', f'mkdir {test_device_storage_path}/Android/data/{test_package_name}']):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['shell', f'mkdir {test_device_storage_path}/Android/data/{test_package_name}/files']):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['shell', f'mkdir {test_device_storage_path}/Android/data/{test_package_name}/files']):
                return "SUCCESS"

        raise AssertionError

    with patch.object(android_deployment.AndroidDeployment, 'get_target_android_devices', return_value=[TEST_DEVICE_ID]), \
         patch.object(android_deployment.AndroidDeployment, 'detect_device_storage_path', return_value=test_device_storage_path), \
         patch.object(android_deployment.AndroidDeployment, 'get_device_file_timestamp', return_value=None), \
         patch.object(android_deployment.AndroidDeployment, 'update_device_file_timestamp') as mock_update_device_file_timestamp, \
         patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={'package_name': test_package_name}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(android_deployment.AndroidDeployment, 'adb_call', wraps=_mock_adb_call) as mock_adb_call, \
         patch.object(pathlib.Path, 'glob', return_value=[pathlib.Path("foo.bar")]):

        inst = android_deployment.AndroidDeployment(dev_root=mock_dev_root,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration=test_config,
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=False,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        inst.execute()

        mock_update_device_file_timestamp.assert_called_once()
        assert mock_adb_call.call_count == 6


@pytest.mark.parametrize(
    "test_game_name, test_config, test_package_name, test_device_storage_path, test_asset_type", [
        pytest.param('game1','profile', 'org.o3de.foo', '/data/fool_storage', 'android'),
        pytest.param('game1','debug', 'org.o3de.foo', '/data/fool_storage', 'android'),
        pytest.param('game2','profile', 'org.o3de.bar', '/data/fool_storage', 'android'),
        pytest.param('game2','debug', 'org.o3de.bar', '/data/fool_storage', 'android'),
        pytest.param('game3','profile', 'org.o3de.foo', '/data/fool_storage2', 'pc'),
        pytest.param('game3','debug', 'org.o3de.foo', '/data/fool_storage2', 'pc'),
        pytest.param('game4','profile', 'org.o3de.bar', '/data/fool_storage2', 'pc'),
        pytest.param('game4','debug', 'org.o3de.bar', '/data/fool_storage2', 'pc')
    ]
)
def test_execute_clean_deploy_success(tmpdir, test_game_name, test_config, test_package_name, test_device_storage_path, test_asset_type):

    mock_dev_root = tmpdir.join(TEST_DEV_ROOT).realpath()

    tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/build/outputs/apk/{test_config}/app-{test_config}.apk").ensure()
    expected_apk_path = str(tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/build/outputs/apk/{test_config}/app-{test_config}.apk").realpath())

    tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/src/assets/Registry/dummy.txt").ensure()
    expected_asset_path = str(tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/src/assets").realpath())

    expected_registry_path = str(tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/src/main/assets/Registry").realpath())

    expected_storage_path = f'{test_device_storage_path}/Android/data/{test_package_name}/files'

    expected_storage_registry_path = f'{test_device_storage_path}/Android/data/{test_package_name}/files/Registry'

    def _mock_adb_call(arg_list, device_id=None):
        if arg_list == "start-server":
            return "SUCCESS"
        if arg_list == "kill-server":
            return "SUCCESS"
        elif isinstance(arg_list,list):
            if match_arg_list(arg_list, ['install', '-r', expected_apk_path]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['install', '-t', '-r', expected_apk_path]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['push', expected_asset_path, '-r', expected_apk_path, expected_storage_path]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['shell', 'cmd', 'package', 'list', 'packages', test_package_name]):
                return test_package_name
            elif match_arg_list(arg_list, ['uninstall', test_package_name]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['push', f'{expected_asset_path}/.', expected_storage_path]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['push', expected_registry_path, expected_storage_registry_path]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['push', expected_registry_path, expected_storage_registry_path]):
                return "SUCCESS"
        raise AssertionError

    def _mock_adb_shell(command, device_id):
        assert command.startswith('rm -rf') or command.startswith('mkdir')

    def _mock_adb_ls(path, device_id, args=None):
        if path == "/foo/bar":
            return False, None
        elif path == '/foo_reals':
            return False, None
        else:
            raise AssertionError

    with patch.object(android_deployment.AndroidDeployment, 'get_target_android_devices', return_value=[TEST_DEVICE_ID]) as mock_get_target_android_devices, \
         patch.object(android_deployment.AndroidDeployment, 'detect_device_storage_path', return_value=test_device_storage_path) as mock_detect_device_storage_path, \
         patch.object(android_deployment.AndroidDeployment, 'get_device_file_timestamp', return_value=None) as mock_get_device_file_timestamp, \
         patch.object(android_deployment.AndroidDeployment, 'update_device_file_timestamp') as mock_update_device_file_timestamp, \
         patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={'package_name': test_package_name}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(android_deployment.AndroidDeployment, 'adb_call', wraps=_mock_adb_call) as mock_adb_call, \
         patch.object(android_deployment.AndroidDeployment, 'adb_shell', wraps=_mock_adb_shell) as mock_adb_shell, \
         patch.object(android_deployment.AndroidDeployment, 'adb_ls', wraps=_mock_adb_ls), \
         patch.object(pathlib.Path, 'glob', return_value=[pathlib.Path("foo.bar")]):

        inst = android_deployment.AndroidDeployment(dev_root=mock_dev_root,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration=test_config,
                                                    game_name=test_game_name,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=test_asset_type,
                                                    embedded_assets=False,
                                                    android_device_filter=None,
                                                    clean_deploy=True,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        inst.execute()

        assert mock_adb_call.call_count == 6
        mock_adb_shell.assert_called()
        mock_update_device_file_timestamp.assert_called_once()
        mock_get_device_file_timestamp.assert_called_once()
        mock_detect_device_storage_path.assert_called_once()
        mock_get_target_android_devices.assert_called_once()


@pytest.mark.parametrize(
    "test_config, test_package_name, test_device_storage_path", [
        pytest.param('profile', 'org.o3de.foo', '/data/fool_storage'),
        pytest.param('debug', 'org.o3de.foo', '/data/fool_storage'),
        pytest.param('profile', 'org.o3de.bar', '/data/fool_storage'),
        pytest.param('debug', 'org.o3de.bar', '/data/fool_storage'),
        pytest.param('profile', 'org.o3de.foo', '/data/fool_storage2'),
        pytest.param('debug', 'org.o3de.foo', '/data/fool_storage2'),
        pytest.param('profile', 'org.o3de.bar', '/data/fool_storage2'),
        pytest.param('debug', 'org.o3de.bar', '/data/fool_storage2')
    ]
)
def test_execute_incremental_deploy_success(tmpdir, test_config, test_package_name, test_device_storage_path):

    mock_dev_root = tmpdir.join(TEST_DEV_ROOT).realpath()

    tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/build/outputs/apk/{test_config}/app-{test_config}.apk").ensure()
    expected_apk_path = str(tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/build/outputs/apk/{test_config}/app-{test_config}.apk").realpath())

    tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/src/assets/Registry/dummy.txt").ensure()
    expected_registry_path = str(tmpdir.join(f"{TEST_DEV_ROOT}/{TEST_BUILD_DIR}/app/src/main/assets/Registry").realpath())

    expected_storage_registry_path = f'{test_device_storage_path}/Android/data/{test_package_name}/files/Registry'

    def _mock_adb_call(arg_list, device_id=None):
        if arg_list == "start-server":
            return "SUCCESS"
        if arg_list == "kill-server":
            return "SUCCESS"
        elif isinstance(arg_list,list):
            if match_arg_list(arg_list, ['install', '-r', expected_apk_path]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['install', '-t', '-r', expected_apk_path]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['push', expected_registry_path, expected_storage_registry_path]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['shell', 'cmd', 'package', 'list', 'packages', test_package_name]):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['shell', f'mkdir {test_device_storage_path}/Android/data/{test_package_name}']):
                return "SUCCESS"
            elif match_arg_list(arg_list, ['shell', f'mkdir {test_device_storage_path}/Android/data/{test_package_name}/files']):
                return "SUCCESS"
            elif len(arg_list) == 3 and arg_list[0] == 'push' and arg_list[1] == 'foo.bar':
                return "SUCCESS"

        raise AssertionError

    def _mock_should_copy_file(check_path, check_time):
        return check_path == 'foo.bar'

    with patch.object(android_deployment.AndroidDeployment, 'get_target_android_devices', return_value=[TEST_DEVICE_ID]) as mock_get_target_android_devices, \
         patch.object(android_deployment.AndroidDeployment, 'detect_device_storage_path', return_value=test_device_storage_path) as mock_detect_device_storage_path, \
         patch.object(android_deployment.AndroidDeployment, 'get_device_file_timestamp', return_value=datetime.datetime.now()) as mock_get_device_file_timestamp, \
         patch.object(android_deployment.AndroidDeployment, 'update_device_file_timestamp') as mock_update_device_file_timestamp, \
         patch.object(android_deployment.AndroidDeployment, 'read_android_settings', return_value={'package_name': test_package_name}), \
         patch.object(android_deployment.AndroidDeployment, 'resolve_adb_tool', return_value=pathlib.Path("Foo")), \
         patch.object(android_deployment.AndroidDeployment, 'adb_call', wraps=_mock_adb_call) as mock_adb_call, \
         patch.object(android_deployment.AndroidDeployment, 'should_copy_file', wraps=_mock_should_copy_file), \
         patch.object(pathlib.Path, 'glob', return_value=[pathlib.Path("foo.bar"), pathlib.Path("no.bar")]):

        inst = android_deployment.AndroidDeployment(dev_root=mock_dev_root,
                                                    build_dir=TEST_BUILD_DIR,
                                                    configuration=test_config,
                                                    game_name=TEST_GAME_NAME,
                                                    asset_mode=TEST_ASSET_MODE,
                                                    asset_type=TEST_ASSET_TYPE,
                                                    embedded_assets=False,
                                                    android_device_filter=None,
                                                    clean_deploy=False,
                                                    deployment_type=android_deployment.AndroidDeployment.DEPLOY_BOTH,
                                                    android_sdk_path=TEST_ANDROID_SDK_PATH)

        inst.execute()

        assert mock_adb_call.call_count == 5
        mock_update_device_file_timestamp.assert_called_once()
        mock_get_device_file_timestamp.assert_called_once()
        mock_detect_device_storage_path.assert_called_once()
        mock_get_target_android_devices.assert_called_once()


