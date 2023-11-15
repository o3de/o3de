#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import configparser
import json
import logging
import os
import subprocess

import pytest
import pathlib
from unittest.mock import patch, Mock

from o3de import android, android_support
from packaging.version import Version


@pytest.mark.parametrize(
    "agp_version, expected_gradle_version", [
        pytest.param("8.1.0", "8.0"),
        pytest.param("8.0.0", "8.0"),
    ]
)
def test_get_android_gradle_plugin_requirements(agp_version, expected_gradle_version):
    result = android_support.get_android_gradle_plugin_requirements(requested_agp_version=agp_version)
    assert result.gradle_ver == Version(expected_gradle_version)


def test_validate_java_environment_from_java_home(tmpdir):

    tmpdir.ensure(f'java_home/bin/java{android_support.EXE_EXTENSION}')
    tmpdir.join(f'java_home/bin/java{android_support.EXE_EXTENSION}').realpath()


    mock_subprocess_result = subprocess.CompletedProcess(args=['java.exe', '-version'],
                                                         returncode=0,
                                                         stdout="""
Picked up JAVA_TOOL_OPTIONS: -Dlog4j2.formatMsgNoLookups=true
java version "17.0.9" 2023-10-17 LTS
Java(TM) SE Runtime Environment (build 17.0.9+11-LTS-201)
Java HotSpot(TM) 64-Bit Server VM (build 17.0.9+11-LTS-201, mixed mode, sharing)
""")

    with patch('os.getenv', return_value=tmpdir.join(f'java_home').realpath()) as mock_os_getenv, \
         patch('subprocess.run', return_value=mock_subprocess_result) as mock_subprocess_run:
        version = android_support.validate_java_environment()
        assert version == '17.0.9'

def test_validate_java_environment_from_invalid_java_home(tmpdir):

    tmpdir.ensure(f'not_java_home/bin/java{android_support.EXE_EXTENSION}')
    tmpdir.join(f'not_java_home/bin/java{android_support.EXE_EXTENSION}').realpath()


    with patch('os.getenv', return_value=tmpdir.join(f'java_home').realpath()) as mock_os_getenv:
        try:
            android_support.validate_java_environment()
        except android_support.AndroidToolError:
            pass
        else:
            assert False, "Expected AndroidToolError(JAVA_HOME invalid)"

def test_validate_java_environment_from_java_home_error(tmpdir):

    tmpdir.ensure(f'java_home/bin/java{android_support.EXE_EXTENSION}')
    tmpdir.join(f'java_home/bin/java{android_support.EXE_EXTENSION}').realpath()

    mock_subprocess_result = subprocess.CompletedProcess(args=['java.exe', '-version'],
                                                         returncode=1,
                                                         stderr="An error occured")

    with patch('os.getenv', return_value=tmpdir.join(f'java_home').realpath()) as mock_os_getenv, \
         patch('subprocess.run', return_value=mock_subprocess_result) as mock_subprocess_run:
        try:
            android_support.validate_java_environment()
        except android_support.AndroidToolError as err:
            assert "An error occured" in str(err)
        else:
            assert False, "Expected AndroidToolError(JAVA_HOME invalid)"

def test_validate_java_environment_from_path(tmpdir):

    mock_subprocess_result = subprocess.CompletedProcess(args=['java.exe', '-version'],
                                                         returncode=0,
                                                         stdout="""
    Picked up JAVA_TOOL_OPTIONS: -Dlog4j2.formatMsgNoLookups=true
    java version "17.0.9" 2023-10-17 LTS
    Java(TM) SE Runtime Environment (build 17.0.9+11-LTS-201)
    Java HotSpot(TM) 64-Bit Server VM (build 17.0.9+11-LTS-201, mixed mode, sharing)
    """)

    with patch('os.getenv', return_value=None) as mock_os_getenv, \
         patch('subprocess.run', return_value=mock_subprocess_result) as mock_subprocess_run:
        version = android_support.validate_java_environment()
        assert version == '17.0.9'


@pytest.mark.parametrize(
    "agp_version, java_version", [
        pytest.param("8.0.0", "17"),
        pytest.param("8.0.1", "17.0"),
        pytest.param("8.0.2", "17.0.2"),
        pytest.param("8.1.0", "17.21.43"),
    ]
)
def test_android_gradle_plugin_requirements_java_pass(agp_version, java_version):
    agp_pass = android_support.get_android_gradle_plugin_requirements(agp_version)
    agp_pass.validate_java_version(java_version)


@pytest.mark.parametrize(
    "agp_version, java_version", [
        pytest.param("8.0.0", "11"),
        pytest.param("8.0.1", "11.0"),
        pytest.param("8.0.2", "8.0.2"),
        pytest.param("8.1.0", "11.21.43"),
        pytest.param("7.1.0", "17"),
    ]
)
def test_android_gradle_plugin_requirements_java_fail(agp_version, java_version):
    try:
        agp_fail = android_support.get_android_gradle_plugin_requirements(agp_version)
        agp_fail.validate_java_version(java_version)
    except android_support.AndroidToolError:
        pass
    else:
        assert False, "Excepted validation failure"


@pytest.mark.parametrize(
    "agp_version, gradle_version", [
        pytest.param("8.0.0", "8.0"),
        pytest.param("8.0.1", "8.1"),
        pytest.param("8.0.2", "8.1.2"),
        pytest.param("8.1.0", "8.3"),
    ]
)
def test_android_gradle_plugin_requirements_gradle_pass(agp_version, gradle_version):
    agp_pass = android_support.get_android_gradle_plugin_requirements(agp_version)
    agp_pass.validate_gradle_version(gradle_version)


@pytest.mark.parametrize(
    "agp_version, gradle_version", [
        pytest.param("8.0.0", "7.0"),
        pytest.param("8.0.1", "7.1"),
        pytest.param("8.1.0", "7.2.1"),
        pytest.param("7.1.1", "11.21.43"),
    ]
)
def test_android_gradle_plugin_requirements_gradle_fail(agp_version, gradle_version):
    try:
        agp_fail = android_support.get_android_gradle_plugin_requirements(agp_version)
        agp_fail.validate_gradle_version(gradle_version)
    except android_support.AndroidToolError:
        pass
    else:
        assert False, "Excepted validation failure"


@pytest.mark.parametrize(
    "test_path", [
        pytest.param(None),
        pytest.param("android_foo"),
        pytest.param("android_sdk_empty"),
        pytest.param("android_sdk")
    ]
)
def test_android_sdk_environment_failed_settings(tmpdir, test_path):
    tmpdir.ensure(f'android_sdk_empty', dir=True)
    tmpdir.ensure(f'android_sdk/cmdline-tools/bin/sdkmanager{android_support.SDKMANAGER_EXTENSION}')
    test_path = tmpdir.join(test_path).realpath() if test_path is not None else None
    try:
        mock_attrs = { 'get_value.return_value' : test_path }
        mock_settings = Mock(**mock_attrs)
        _,_ = android_support.AndroidSDKManager.validate_android_sdk_environment("17", mock_settings)
    except android_support.AndroidToolError:
        pass
    else:
        assert False, "Expected failure"


def test_android_sdk_environment_failed_validation_test_bad_java_version(tmpdir):

    tmpdir.ensure(f'android_sdk/cmdline-tools/latest/bin/sdkmanager{android_support.SDKMANAGER_EXTENSION}')
    test_path = tmpdir.join('android_sdk').realpath()

    try:
        mock_attrs = { 'get_value.return_value' : test_path }
        mock_settings = Mock(**mock_attrs)
        subprocess_result = subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
                                                        returncode=1,
                                                        stdout='Exception in thread "main" java.lang.UnsupportedClassVersionError: '
                                                               'com/android/sdklib/tool/sdkmanager/SdkManagerCli has been compiled '
                                                               'by a more recent version of the Java Runtime (class file version 61.0), '
                                                               'this version of the Java Runtime only recognizes class file versions up to 52.0')
        with patch('subprocess.run', return_value=subprocess_result):
            _, _ = android_support.AndroidSDKManager.validate_android_sdk_environment("17", mock_settings)
    except android_support.AndroidToolError as err:
        assert 'line tool requires java' in str(err), "Expected specific java error message"
    else:
        assert False, "Expected failure"

def test_android_sdk_environment_failed_validation_test_bad_android_sdk_cmd_tools_location(tmpdir):

    tmpdir.ensure(f'android_sdk/cmdline-tools/latest/bin/sdkmanager{android_support.SDKMANAGER_EXTENSION}')
    test_path = tmpdir.join('android_sdk').realpath()

    try:
        mock_attrs = { 'get_value.return_value' : test_path }
        mock_settings = Mock(**mock_attrs)
        subprocess_result = subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
                                                        returncode=1,
                                                        stdout='Could not determine SDK root')
        with patch('subprocess.run', return_value=subprocess_result):
            _, _ = android_support.AndroidSDKManager.validate_android_sdk_environment("17", mock_settings)
    except android_support.AndroidToolError as err:
        assert 'not located under a valid Android SDK root' in str(err), "Expected specific sdk command line error message"
    else:
        assert False, "Expected failure"

def test_android_sdk_environment_failed_validation_test_general_error(tmpdir):

    tmpdir.ensure(f'android_sdk/cmdline-tools/latest/bin/sdkmanager{android_support.SDKMANAGER_EXTENSION}')
    test_path = tmpdir.join('android_sdk').realpath()

    try:
        mock_attrs = { 'get_value.return_value' : test_path }
        mock_settings = Mock(**mock_attrs)
        subprocess_result = subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
                                                        returncode=1,
                                                        stdout='Something went wrong.')
        with patch('subprocess.run', return_value=subprocess_result):
            _, _ = android_support.AndroidSDKManager.validate_android_sdk_environment("17", mock_settings)
    except android_support.AndroidToolError as err:
        assert 'An error occurred attempt to run the android command line tool' in str(err), "Expected specific sdk command line error message"
    else:
        assert False, "Expected failure"

def test_android_sdk_manager_get_installed_packages():

    installed_packages_cmd_result = """
    Installed packages:
  Path                               | Version      | Description                       | Location
  -------                            | -------      | -------                           | -------
  build-tools;30.0.2                 | 30.0.2       | Android SDK Build-Tools 30.0.2    | build-tools\30.0.2
  platforms;android-33               | 3            | Android SDK Platform 33           | platforms\android-33

Available Packages:
  add-ons;addon-google_apis-google-15                                                      | 3             | Google APIs
  add-ons;addon-google_apis-google-23                                                      | 1             | Google APIs
  
Available Updates:
  platforms;android-33                                                                     | 4             | Google APIs
    :return: 
    :rtype: 
    """

    subprocess_result = subprocess.CompletedProcess(
        args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
        returncode=0,
        stdout=installed_packages_cmd_result)

    with patch('o3de.android_support.AndroidSDKManager.validate_android_sdk_environment', return_value=("mock", "mock")), \
         patch('subprocess.run', return_value=subprocess_result):

        test = android_support.AndroidSDKManager(None, None)

        result_installed = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.INSTALLED)
        assert len(result_installed) == 2
        assert result_installed[0].path == 'build-tools;30.0.2'
        assert result_installed[1].path == 'platforms;android-33'

        result_available = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.AVAILABLE)
        assert len(result_available) == 2
        assert result_available[0].path == 'add-ons;addon-google_apis-google-15'
        assert result_available[1].path == 'add-ons;addon-google_apis-google-23'

        result_updateable = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.UPDATABLE)
        assert len(result_updateable) == 1
        assert result_updateable[0].path == 'platforms;android-33'


def test_android_sdk_manager_install_package_already_installed():
    installed_packages_cmd_result = """
    Installed packages:
  Path                               | Version      | Description                       | Location
  -------                            | -------      | -------                           | -------
  build-tools;30.0.2                 | 30.0.2       | Android SDK Build-Tools 30.0.2    | build-tools\30.0.2
  platforms;android-33               | 3            | Android SDK Platform 33           | platforms\android-33

Available Packages:
  add-ons;addon-google_apis-google-15                                                      | 3             | Google APIs
  add-ons;addon-google_apis-google-23                                                      | 1             | Google APIs

Available Updates:
  platforms;android-33                                                                     | 4             | Google APIs
    :return: 
    :rtype: 
    """

    subprocess_result = subprocess.CompletedProcess(
        args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
        returncode=0,
        stdout=installed_packages_cmd_result)

    with patch('o3de.android_support.AndroidSDKManager.validate_android_sdk_environment',
               return_value=("mock", "mock")), \
            patch('subprocess.run', return_value=subprocess_result):

        test = android_support.AndroidSDKManager(None, None)
        result = test.install_package('platforms;android-33', "Android SDK Platform 33")
        assert result is not None

        result_installed = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.INSTALLED)
        assert len(result_installed) == 2
        assert result_installed[0].path == 'build-tools;30.0.2'
        assert result_installed[1].path == 'platforms;android-33'

        result_available = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.AVAILABLE)
        assert len(result_available) == 2
        assert result_available[0].path == 'add-ons;addon-google_apis-google-15'
        assert result_available[1].path == 'add-ons;addon-google_apis-google-23'

        result_updateable = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.UPDATABLE)
        assert len(result_updateable) == 1
        assert result_updateable[0].path == 'platforms;android-33'


def test_android_sdk_manager_install_package_new():

    installed_packages_cmd_result_call1 = """
    Installed packages:
  Path                               | Version      | Description                       | Location
  -------                            | -------      | -------                           | -------
  build-tools;30.0.2                 | 30.0.2       | Android SDK Build-Tools 30.0.2    | build-tools\30.0.2
  platforms;android-33               | 3            | Android SDK Platform 33           | platforms\android-33

Available Packages:
  add-ons;addon-google_apis-google-15                                                      | 3             | Google APIs
  add-ons;addon-google_apis-google-23                                                      | 1             | Google APIs

Available Updates:
  platforms;android-33                                                                     | 4             | Google APIs
    :return: 
    :rtype: 
    """

    installed_packages_cmd_result_call2 = """
        Installed packages:
      Path                                  | Version      | Description                       | Location
      -------                               | -------      | -------                           | -------
      build-tools;30.0.2                    | 30.0.2       | Android SDK Build-Tools 30.0.2    | build-tools\30.0.2
      platforms;android-33                  | 3            | Android SDK Platform 33           | platforms\android-33
      add-ons;addon-google_apis-google-15   | 1            | Platform API 15                   | platforms\android-15

    Available Packages:
      add-ons;addon-google_apis-google-23                                                      | 1             | Google APIs

    Available Updates:
      platforms;android-33                                                                     | 4             | Google APIs
        :return: 
        :rtype: 
        """
    iter = 0
    def _return_1(*popenargs, input=None, capture_output=False, timeout=None, check=False, **kwargs):
        nonlocal iter
        iter += 1
        if iter == 1:
            return subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
                                               returncode=0,
                                               stdout=installed_packages_cmd_result_call1)
        elif iter == 2:
            return subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
                                               returncode=0,
                                               stdout="")
        elif iter == 3:
            return subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
                                               returncode=0,
                                               stdout=installed_packages_cmd_result_call2)
        else:
            assert False, "Unexpected call to subprocess.run"

    with patch(target='o3de.android_support.AndroidSDKManager.validate_android_sdk_environment',
               return_value=("mock", "mock")), \
         patch(target='subprocess.run', new_callable=Mock(side_effect=[_return_1])):

        test = android_support.AndroidSDKManager(None, None)

        result = test.install_package('add-ons;addon-google_apis-google-15', "API 25")
        assert result is not None

        result_installed = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.INSTALLED)
        assert result_installed[0].path == 'build-tools;30.0.2'
        assert result_installed[1].path == 'platforms;android-33'
        assert result_installed[2].path == 'add-ons;addon-google_apis-google-15'


def test_android_sdk_manager_install_bad_package_name():
    installed_packages_cmd_result = """
Installed packages:
  Path                               | Version      | Description                       | Location
  -------                            | -------      | -------                           | -------
  build-tools;30.0.2                 | 30.0.2       | Android SDK Build-Tools 30.0.2    | build-tools\30.0.2
  platforms;android-33               | 3            | Android SDK Platform 33           | platforms\android-33

Available Packages:
  add-ons;addon-google_apis-google-15                                                      | 3             | Google APIs
  add-ons;addon-google_apis-google-23                                                      | 1             | Google APIs

Available Updates:
  platforms;android-33                                                                     | 4             | Google APIs
    :return: 
    :rtype: 
    """

    subprocess_result = subprocess.CompletedProcess(
        args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
        returncode=0,
        stdout=installed_packages_cmd_result)

    with patch('o3de.android_support.AndroidSDKManager.validate_android_sdk_environment',
               return_value=("mock", "mock")), \
            patch('subprocess.run', return_value=subprocess_result):

        test = android_support.AndroidSDKManager(None, None)
        try:
            test.install_package('foo', 'bar')
        except android_support.AndroidToolError:
            pass
        else:
            assert False, "Error expected"

def test_android_sdk_manager_check_licenses_not_accepted():

    iter = 0
    def _return_(*popenargs, input=None, capture_output=False, timeout=None, check=False, **kwargs):
        nonlocal iter
        iter += 1
        if iter == 1:
            return subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
                                               returncode=0,
                                               stdout="""
Installed packages:

Available Packages:

Available Updates:
""")
        elif iter == 2:
            return subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--licenses'],
                                               returncode=0,
                                               stdout="7 of 7 SDK package licenses not accepted")
        else:
            assert False, "Unexpected call to subprocess.run"

    with patch(target='o3de.android_support.AndroidSDKManager.validate_android_sdk_environment',
               return_value=("mock", "mock")), \
         patch(target='subprocess.run', new_callable=Mock(side_effect=[_return_])):

        test = android_support.AndroidSDKManager(None, None)
        try:
            test.check_licenses()
        except android_support.AndroidToolError as e:
            assert 'licenses not accepted' in str(e)
        else:
            assert False, "Error expected"

def test_android_sdk_manager_check_licenses_error():

    iter = 0

    def _return_(*popenargs, input=None, capture_output=False, timeout=None, check=False, **kwargs):
        nonlocal iter
        iter += 1
        if iter == 1:
            return subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
                                               returncode=0,
                                               stdout="""
    Installed packages:

    Available Packages:

    Available Updates:
    """)
        elif iter == 2:
            return subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--licenses'],
                                               returncode=0,
                                               stdout="Invalid java")
        else:
            assert False, "Unexpected call to subprocess.run"

    with patch(target='o3de.android_support.AndroidSDKManager.validate_android_sdk_environment',
               return_value=("mock", "mock")), \
            patch(target='subprocess.run', new_callable=Mock(side_effect=[_return_])):

        test = android_support.AndroidSDKManager(None, None)
        try:
            test.check_licenses()
        except android_support.AndroidToolError as e:
            assert 'Unable to determine the Android SDK Package license state' in str(e)
        else:
            assert False, "Error expected"


def test_android_sdk_manager_check_licenses_accepted():

    iter = 0

    def _return_(*popenargs, input=None, capture_output=False, timeout=None, check=False, **kwargs):
        nonlocal iter
        iter += 1
        if iter == 1:
            return subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--version'],
                                               returncode=0,
                                               stdout="""
    Installed packages:

    Available Packages:

    Available Updates:
    """)
        elif iter == 2:
            return subprocess.CompletedProcess(args=[f'sdkmanager{android_support.SDKMANAGER_EXTENSION}', '--licenses'],
                                               returncode=0,
                                               stdout="All SDK package licenses accepted.")
        else:
            assert False, "Unexpected call to subprocess.run"

    with patch(target='o3de.android_support.AndroidSDKManager.validate_android_sdk_environment',
               return_value=("mock", "mock")), \
            patch(target='subprocess.run', new_callable=Mock(side_effect=[_return_])):

        test = android_support.AndroidSDKManager(None, None)

        test.check_licenses()

