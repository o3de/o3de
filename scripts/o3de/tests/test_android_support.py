#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import subprocess

import pytest
from unittest.mock import patch, Mock, PropertyMock

from o3de import android_support, command_utils
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

    # Test Data
    test_java_version = '17.0.9'
    test_result_java_version = f"""
        Picked up JAVA_TOOL_OPTIONS: -Dlog4j2.formatMsgNoLookups=true
        java version "{test_java_version}" 2023-10-17 LTS
        Java(TM) SE Runtime Environment (build 17.0.9+11-LTS-201)
        Java HotSpot(TM) 64-Bit Server VM (build 17.0.9+11-LTS-201, mixed mode, sharing)
    """

    tmpdir.ensure(f'java_home/bin/java{android_support.EXE_EXTENSION}')
    tmpdir.join(f'java_home/bin/java{android_support.EXE_EXTENSION}').realpath()

    # Mocks
    with patch('os.getenv') as mock_os_getenv, \
         patch('subprocess.run') as mock_subprocess_run:

        mock_os_getenv.return_value = tmpdir.join(f'java_home').realpath()

        mock_completed_process = PropertyMock()
        mock_completed_process.returncode = 0
        mock_completed_process.stdout = test_result_java_version

        mock_subprocess_run.return_value = mock_completed_process

        # Test the method
        version = android_support.validate_java_environment()

        # Validation
        mock_os_getenv.assert_called_once()
        assert version == test_java_version


@pytest.mark.xfail(raises=android_support.AndroidToolError)
def test_validate_java_environment_from_invalid_java_home(tmpdir):

    # Mocks
    with patch('os.getenv') as mock_os_getenv:

        mock_os_getenv.return_value = tmpdir.join(f'empty')

        # Test the method
        android_support.validate_java_environment()


def test_validate_java_environment_from_java_home_error(tmpdir):

    tmpdir.ensure(f'java_home/bin/java{android_support.EXE_EXTENSION}')
    tmpdir.join(f'java_home/bin/java{android_support.EXE_EXTENSION}').realpath()

    # Test Data
    test_error = "An error Occured"
    test_java_home_val = tmpdir.join(f'java_home').realpath()

    # Mocks
    with patch('os.getenv') as mock_os_getenv, \
         patch('subprocess.run') as mock_subprocess_run:

        mock_os_getenv.return_value = test_java_home_val

        mock_completed_process = PropertyMock()
        mock_completed_process.returncode = 1
        mock_completed_process.stderr = test_error

        mock_subprocess_run.return_value = mock_completed_process

        try:
            # Test the method
            android_support.validate_java_environment()
        except android_support.AndroidToolError as err:
            assert test_error in str(err)
        else:
            assert False, "Expected AndroidToolError(JAVA_HOME invalid)"


def test_validate_java_environment_from_path(tmpdir):

    # Test Data
    test_java_version = '17.0.9'
    test_result_java_version = f"""
        Picked up JAVA_TOOL_OPTIONS: -Dlog4j2.formatMsgNoLookups=true
        java version "{test_java_version}" 2023-10-17 LTS
        Java(TM) SE Runtime Environment (build 17.0.9+11-LTS-201)
        Java HotSpot(TM) 64-Bit Server VM (build 17.0.9+11-LTS-201, mixed mode, sharing)
    """

    # Mocks
    with patch('os.getenv') as mock_os_getenv, \
         patch('subprocess.run') as mock_subprocess_run:

        mock_os_getenv.return_value = None

        mock_subprocess_result = PropertyMock
        mock_subprocess_result.returncode = 0
        mock_subprocess_result.stdout = test_result_java_version
        mock_subprocess_run.return_value = mock_subprocess_result

        # Test the method
        version = android_support.validate_java_environment()

        # Validation
        assert version == test_java_version
        mock_os_getenv.assert_called_once()
        mock_subprocess_run.assert_called_once()


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
@pytest.mark.xfail(raises=android_support.AndroidToolError)
def test_android_gradle_plugin_requirements_java_fail(agp_version, java_version):

    agp_fail = android_support.get_android_gradle_plugin_requirements(agp_version)
    agp_fail.validate_java_version(java_version)


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
@pytest.mark.xfail(raises=android_support.AndroidToolError)
def test_android_gradle_plugin_requirements_gradle_fail(agp_version, gradle_version):

    agp_fail = android_support.get_android_gradle_plugin_requirements(agp_version)
    agp_fail.validate_gradle_version(gradle_version)


@pytest.mark.parametrize(
    "test_path", [
        pytest.param(None),
        pytest.param("android_foo"),
        pytest.param("android_sdk_empty"),
        pytest.param("android_sdk")
    ]
)
@pytest.mark.xfail(raises=android_support.AndroidToolError)
def test_android_sdk_environment_failed_settings(tmpdir, test_path):

    tmpdir.ensure(f'android_sdk_empty', dir=True)
    tmpdir.ensure(f'android_sdk/cmdline-tools/bin/sdkmanager{android_support.SDKMANAGER_EXTENSION}')

    # Test Data
    test_java_version = "17"

    # Mocks
    mock_settings = Mock(spec=command_utils.O3DEConfig)
    mock_settings.get_value.return_value = tmpdir.join(test_path).realpath() if test_path is not None else None

    # Test the method
    android_support.AndroidSDKManager.validate_android_sdk_environment(test_java_version, mock_settings)


def test_android_sdk_environment_failed_validation_test_bad_java_version(tmpdir):

    tmpdir.ensure(f'android_sdk/cmdline-tools/latest/bin/sdkmanager{android_support.SDKMANAGER_EXTENSION}')

    # Test Data
    test_java_version = "17"
    test_path = tmpdir.join('android_sdk').realpath()
    test_err_msg = """
Exception in thread \"main\" java.lang.UnsupportedClassVersionError: com/android/sdklib/tool/sdkmanager/SdkManagerCli has been compiled by a more recent version of the Java Runtime (class file version 61.0),  this version of the Java Runtime only recognizes class file versions up to 52.0
"""
    # Mocks
    with patch('subprocess.run') as mock_subprocess_run:
        mock_settings = Mock(spec=command_utils.O3DEConfig)
        mock_settings.get_value.return_value = test_path

        mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
        mock_completed_process.returncode = 1
        mock_completed_process.stdout = ""
        mock_completed_process.stderr = test_err_msg
        mock_subprocess_run.return_value = mock_completed_process
        try:
            # Test the method
            android_support.AndroidSDKManager.validate_android_sdk_environment(test_java_version, mock_settings)
        except android_support.AndroidToolError as err:
            # Validate the specific error
            assert 'command line tool requires java version' in str(err), "Expected specific java error message"
        else:
            assert False, "Error expected"


def test_android_sdk_environment_failed_validation_test_bad_android_sdk_cmd_tools_location(tmpdir):

    tmpdir.ensure(f'android_sdk/cmdline-tools/latest/bin/sdkmanager{android_support.SDKMANAGER_EXTENSION}')

    # Test Data
    test_path = tmpdir.join('android_sdk').realpath()
    test_err_msg = 'Could not determine SDK root'
    test_java_version = "17"

    # Mocks
    with patch('subprocess.run') as mock_subprocess_run:

        mock_settings = Mock(spec=command_utils.O3DEConfig)
        mock_settings.get_value.return_value = test_path

        mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
        mock_completed_process.returncode = 1
        mock_completed_process.stdout = ""
        mock_completed_process.stderr = test_err_msg
        mock_subprocess_run.return_value = mock_completed_process

        try:
            # Test the method
            android_support.AndroidSDKManager.validate_android_sdk_environment(test_java_version, mock_settings)

        except android_support.AndroidToolError as err:
            # Validate the error
            assert 'not located under a valid Android SDK root' in str(err), "Expected specific sdk command line error message"
        else:
            assert False, "Expected failure"


def test_android_sdk_environment_failed_validation_test_general_error(tmpdir):

    tmpdir.ensure(f'android_sdk/cmdline-tools/latest/bin/sdkmanager{android_support.SDKMANAGER_EXTENSION}')

    # Test Data
    test_path = tmpdir.join('android_sdk').realpath()
    test_err_msg = 'Something went wrong.'
    test_java_version = "17"

    # Mocks
    with patch('subprocess.run') as mock_subprocess_run:

        mock_settings = Mock(spec=command_utils.O3DEConfig)
        mock_settings.get_value.return_value = test_path

        mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
        mock_completed_process.returncode = 1
        mock_completed_process.stdout = ""
        mock_completed_process.stderr = test_err_msg
        mock_subprocess_run.return_value = mock_completed_process

        try:
            # Test the method
            android_support.AndroidSDKManager.validate_android_sdk_environment(test_java_version, mock_settings)
        except android_support.AndroidToolError as err:
            # Validate the error
            assert 'An error occurred attempt to run the android command line tool' in str(err), "Expected specific sdk command line error message"
        else:
            assert False, "Expected failure"


def test_android_sdk_manager_get_installed_packages():

    # Test Data
    installed_packages_cmd_result = """
    Installed packages:
  Path                               | Version      | Description                       | Location
  -------                            | -------      | -------                           | -------
  build-tools;30.0.2                 | 30.0.2       | Android SDK Build-Tools 30.0.2    | build-tools\\30.0.2
  platforms;android-33               | 3            | Android SDK Platform 33           | platforms\\android-33

Available Packages:
  add-ons;addon-google_apis-google-15                                                      | 3             | Google APIs
  add-ons;addon-google_apis-google-23                                                      | 1             | Google APIs
  
Available Updates:
  platforms;android-33                                                                     | 4             | Google APIs
    :return: 
    :rtype: 
    """

    # Mocks
    with patch('o3de.android_support.AndroidSDKManager.validate_android_sdk_environment') as mock_validate_android_sdk_environment, \
         patch('subprocess.run') as mock_subprocess_run:

        mock_validate_android_sdk_environment.return_value = (None, None)

        mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
        mock_completed_process.returncode = 0
        mock_completed_process.stdout = installed_packages_cmd_result
        mock_subprocess_run.return_value = mock_completed_process

        test = android_support.AndroidSDKManager(None, None)

        # Test the method (android_support.AndroidSDKManager.PackageCategory.INSTALLED)
        result_installed = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.INSTALLED)

        # Validation
        assert len(result_installed) == 2
        assert result_installed[0].path == 'build-tools;30.0.2'
        assert result_installed[1].path == 'platforms;android-33'

        # Test the method (android_support.AndroidSDKManager.PackageCategory.AVAILABLE)
        result_available = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.AVAILABLE)

        # Validation
        assert len(result_available) == 2
        assert result_available[0].path == 'add-ons;addon-google_apis-google-15'
        assert result_available[1].path == 'add-ons;addon-google_apis-google-23'

        # Test the method (android_support.AndroidSDKManager.PackageCategory.UPDATABLE)
        result_updateable = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.UPDATABLE)

        # Validation
        assert len(result_updateable) == 1
        assert result_updateable[0].path == 'platforms;android-33'


def test_android_sdk_manager_install_package_already_installed():

    # Test Data
    installed_packages_cmd_result = """
    
Installed packages:
  Path                               | Version      | Description                       | Location
  -------                            | -------      | -------                           | -------
  build-tools;30.0.2                 | 30.0.2       | Android SDK Build-Tools 30.0.2    | build-tools\\30.0.2
  platforms;android-33               | 3            | Android SDK Platform 33           | platforms\\android-33

Available Packages:
  add-ons;addon-google_apis-google-15                                                      | 3             | Google APIs
  add-ons;addon-google_apis-google-23                                                      | 1             | Google APIs

Available Updates:
  platforms;android-33                                                                     | 4             | Google APIs
    :return: 
    :rtype: 
    """

    # Mocks
    with patch('o3de.android_support.AndroidSDKManager.validate_android_sdk_environment') as mock_validate_android_sdk_environment, \
         patch('subprocess.run') as mock_subprocess_run:

        mock_validate_android_sdk_environment.return_value = (None, None)

        mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
        mock_completed_process.returncode = 0
        mock_completed_process.stdout = installed_packages_cmd_result
        mock_subprocess_run.return_value = mock_completed_process

        test = android_support.AndroidSDKManager(None, None)

        # Test the method (Missing)
        result = test.install_package('platforms;android-33', "Android SDK Platform 33")
        # Validation
        assert result is not None

        # Test the method (android_support.AndroidSDKManager.PackageCategory.INSTALLED)
        result_installed = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.INSTALLED)
        # Validation
        assert len(result_installed) == 2
        assert result_installed[0].path == 'build-tools;30.0.2'
        assert result_installed[1].path == 'platforms;android-33'

        # Test the method (android_support.AndroidSDKManager.PackageCategory.AVAILABLE)
        result_available = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.AVAILABLE)
        # Validation
        assert len(result_available) == 2
        assert result_available[0].path == 'add-ons;addon-google_apis-google-15'
        assert result_available[1].path == 'add-ons;addon-google_apis-google-23'

        # Test the method (android_support.AndroidSDKManager.PackageCategory.UPDATABLE)
        result_updateable = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.UPDATABLE)
        # Validation
        assert len(result_updateable) == 1
        assert result_updateable[0].path == 'platforms;android-33'


def test_android_sdk_manager_install_package_new():

    # Test Data
    cmd_result_calls = ["""
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
    """,
    "",
    """
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
        """]

    # Mocks
    with patch('o3de.android_support.AndroidSDKManager.validate_android_sdk_environment') as mock_validate_android_sdk_environment, \
         patch('subprocess.run') as mock_subprocess_run:

        mock_validate_android_sdk_environment.return_value = (None, None)

        mock_index = 0  # Track a counter to simulate multiple calls to the same function, producing multiple results

        def _mock_subprocess_run_(*popenargs, input=None, capture_output=False, timeout=None, check=False, **kwargs):
            nonlocal mock_index
            assert mock_index < 3
            message = cmd_result_calls[mock_index]
            mock_index += 1

            mock_completed_process = Mock(spec=subprocess.CompletedProcess)
            mock_completed_process.returncode = 0
            mock_completed_process.stdout = message
            return mock_completed_process

        mock_subprocess_run.side_effect = _mock_subprocess_run_

        test = android_support.AndroidSDKManager(None, None)
        # Test the install_package
        result = test.install_package('add-ons;addon-google_apis-google-15', "API 25")
        # Validation (that it was already installed)
        assert result is not None

        # Test the package list
        result_installed = test.get_package_list('*', android_support.AndroidSDKManager.PackageCategory.INSTALLED)

        # Validation
        assert result_installed[0].path == 'build-tools;30.0.2'
        assert result_installed[1].path == 'platforms;android-33'
        assert result_installed[2].path == 'add-ons;addon-google_apis-google-15'


@pytest.mark.xfail(raises=android_support.AndroidToolError)
def test_android_sdk_manager_install_bad_package_name():
    # Test Data
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

    # Mocks
    with patch('o3de.android_support.AndroidSDKManager.validate_android_sdk_environment') as mock_validate_android_sdk_environment, \
         patch('subprocess.run') as mock_subprocess_run:

        mock_validate_android_sdk_environment.return_value = (None, None)
        mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
        mock_completed_process.returncode = 0
        mock_completed_process.stdout = installed_packages_cmd_result
        mock_subprocess_run.return_value = mock_completed_process

        # Test installing a bad package
        test = android_support.AndroidSDKManager(None, None)
        test.install_package('foo', 'bar')


def test_android_sdk_manager_check_licenses_not_accepted():

    # Test Data
    cmd_call_results = ["""
Installed packages:

Available Packages:

Available Updates:
        """,
        "7 of 7 SDK package licenses not accepted"]

    # Mocks
    with patch('o3de.android_support.AndroidSDKManager.validate_android_sdk_environment') as mock_validate_android_sdk_environment, \
         patch('subprocess.run') as mock_subprocess_run:

        mock_validate_android_sdk_environment.return_value = (None, None)

        mock_index = 0

        def _mock_subprocess_run_(*popenargs, input=None, capture_output=False, timeout=None, check=False, **kwargs):
            nonlocal mock_index
            assert mock_index < 2
            message = cmd_call_results[mock_index]
            mock_index += 1

            mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
            mock_completed_process.returncode = 0
            mock_completed_process.stdout = message
            mock_completed_process.stderr = ""

            return mock_completed_process

        mock_subprocess_run.side_effect = _mock_subprocess_run_

        test = android_support.AndroidSDKManager(None, None)
        try:
            test.check_licenses()
        except android_support.AndroidToolError as e:
            assert 'licenses not accepted' in str(e)
        else:
            assert False, "Error expected"


def test_android_sdk_manager_check_licenses_error():

    # Test Data
    cmd_call_results = ["""
Installed packages:

Available Packages:

Available Updates:
                        """,
                        "Invalid java"
                        ]

    # Mocks
    with patch('o3de.android_support.AndroidSDKManager.validate_android_sdk_environment') as mock_validate_android_sdk_environment, \
         patch('subprocess.run') as mock_subprocess_run:

        mock_validate_android_sdk_environment.return_value = (None, None)

        mock_index = 0

        def _mock_subprocess_run_(*popenargs, input=None, capture_output=False, timeout=None, check=False, **kwargs):
            nonlocal mock_index
            assert mock_index < 2
            message = cmd_call_results[mock_index]
            mock_index += 1

            mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
            mock_completed_process.returncode = 0
            mock_completed_process.stdout = message
            mock_completed_process.stderr = ""

            return mock_completed_process

        mock_subprocess_run.side_effect = _mock_subprocess_run_

        test = android_support.AndroidSDKManager(None, None)
        try:
            # Test the method
            test.check_licenses()
        except android_support.AndroidToolError as e:
            # Validate the error
            assert 'Unable to determine the Android SDK Package license state' in str(e)
        else:
            assert False, "Error expected"


def test_android_sdk_manager_check_licenses_accepted():
    cmd_call_results = ["""
Installed packages:

Available Packages:

Available Updates:
                        """,
                        "All SDK package licenses accepted."
                        ]

    iter = 0

    def _return_(*popenargs, input=None, capture_output=False, timeout=None, check=False, **kwargs):
        nonlocal iter
        assert iter < 2
        message = cmd_call_results[iter]
        iter += 1

        mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
        mock_completed_process.returncode = 0
        mock_completed_process.stdout = message
        mock_completed_process.stderr = ""

        return mock_completed_process

    # Mocks
    with patch('o3de.android_support.AndroidSDKManager.validate_android_sdk_environment') as mock_validate_android_sdk_environment, \
         patch('subprocess.run') as mock_subprocess_run:

        mock_validate_android_sdk_environment.return_value = (None, None)

        mock_subprocess_run.side_effect = _return_

        # Test the method
        test = android_support.AndroidSDKManager(None, None)
        test.check_licenses()


@pytest.mark.parametrize(
    "test_version_query, test_version_result, test_version_regex, test_expected_version", [
        pytest.param("-version", "Version 1.4", r'(Version)\s*([\d\.]*)', '1.4'),
        pytest.param("--version", "Unknown", r'(Version)\s*([\d\.]*)', None),
        pytest.param("/version", "Version 1.6", r'(Version)\s*([\d\.]*)', '1.6')
    ]
)
def test_validate_build_tool_from_config_key(test_version_query, test_version_result, test_version_regex, test_expected_version):

    # Test Data
    tool_name = 'test'
    tool_cmd = 'validate.exe'
    tool_config_subpath = 'bin'
    tool_config_key = 'validate.home'
    tool_config_value = f'{os.sep}home{os.sep}path{os.sep}validator'
    tool_config_full_path = os.path.join(tool_config_value, tool_config_subpath, tool_cmd)

    tool_version_arg = test_version_query
    tool_version_regex = test_version_regex

    # Mocks
    with patch('o3de.command_utils.O3DEConfig') as mock_android_config, \
         patch('o3de.android_support.subprocess.run') as mock_subprocess_run:

        mock_android_config.get_value.return_value = tool_config_value

        def _mock_subprocess_run_(*popenargs, input=None, capture_output=False, timeout=None, check=False, **kwargs):

            full_cmd_line_args = popenargs[0]
            full_cmd_line = ' '.join(full_cmd_line_args)

            expected_cmd_line = f'{tool_config_full_path} {tool_version_arg}'
            assert full_cmd_line == expected_cmd_line

            mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
            mock_completed_process.returncode = 0
            mock_completed_process.stdout = test_version_result
            mock_completed_process.stderr = ""

            return mock_completed_process

        mock_subprocess_run.side_effect = _mock_subprocess_run_

        try:
            # Test the method
            result_location, result_version = android_support.validate_build_tool(tool_name=tool_name,
                                                                                  tool_command=tool_cmd,
                                                                                  tool_config_key=tool_config_key,
                                                                                  tool_environment_var='',
                                                                                  tool_config_sub_path=tool_config_subpath,
                                                                                  tool_version_arg=tool_version_arg,
                                                                                  version_regex=tool_version_regex,
                                                                                  android_config=mock_android_config)
            # Validate success results
            assert str(result_location) == tool_config_full_path
            assert result_version == test_expected_version

        except android_support.AndroidToolError:
            # Validate error scenarios
            assert test_expected_version is None


@pytest.mark.parametrize(
    "test_version_query, test_version_result, test_version_regex, test_expected_version", [
        pytest.param("-version", "Version 1.4", r'(Version)\s*([\d\.]*)', '1.4'),
        pytest.param("--version", "Unknown", r'(Version)\s*([\d\.]*)', None),
        pytest.param("/version", "Version 1.6", r'(Version)\s*([\d\.]*)', '1.6')
    ]
)
def test_validate_build_tool_from_env(test_version_query, test_version_result, test_version_regex, test_expected_version):

    # Test Data
    tool_name = 'test'
    tool_config_subpath = 'bin'

    tool_cmd = 'validate.exe'
    tool_config_key = 'validate.home'
    tool_config_value = f'{os.sep}home{os.sep}path{os.sep}invalid_validator'
    tool_config_full_path = os.path.join(tool_config_value, tool_config_subpath, tool_cmd)

    tool_env_key = 'VALIDATE_HOME'
    tool_env_value = f'{os.sep}home{os.sep}path{os.sep}validator'
    tool_env_full_path = os.path.join(tool_env_value, tool_config_subpath, tool_cmd)

    tool_version_arg = test_version_query
    tool_version_regex = test_version_regex

    # Mocks
    with patch('o3de.command_utils.O3DEConfig') as mock_android_config, \
         patch('os.getenv') as mock_os_getenv, \
         patch('o3de.android_support.subprocess.run') as mock_subprocess_run:

        mock_android_config.get_value.return_value = tool_config_value

        mock_os_getenv.return_value = tool_env_value

        def _mock_subprocess_run_(*popenargs, input=None, capture_output=False, timeout=None, check=False, **kwargs):
            full_cmd_line_args = popenargs[0]
            if full_cmd_line_args[0] == tool_config_full_path:
                return subprocess.CompletedProcess(args=full_cmd_line_args, returncode=1, stdout="")

            full_cmd_line = ' '.join(full_cmd_line_args)

            # nonlocal tool_config_value, tool_config_subpath, tool_cmd
            expected_cmd_line = f'{tool_env_full_path} {tool_version_arg}'
            assert full_cmd_line == expected_cmd_line

            mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
            mock_completed_process.returncode = 0
            mock_completed_process.stdout = test_version_result
            mock_completed_process.stderr = ""

            return mock_completed_process

        mock_subprocess_run.side_effect = _mock_subprocess_run_

        try:
            result_location, result_version = android_support.validate_build_tool(tool_name=tool_name,
                                                                                  tool_command=tool_cmd,
                                                                                  tool_config_key=tool_config_key,
                                                                                  tool_environment_var=tool_env_key,
                                                                                  tool_config_sub_path=tool_config_subpath,
                                                                                  tool_version_arg=tool_version_arg,
                                                                                  version_regex=tool_version_regex,
                                                                                  android_config=mock_android_config)
            assert str(result_location) == tool_env_full_path
            assert result_version == test_expected_version
        except android_support.AndroidToolError:
            assert test_expected_version is None
        finally:
            mock_os_getenv.assert_called_once_with(tool_env_key)


@pytest.mark.parametrize(
    "test_version_query, test_version_result, test_version_regex, test_expected_version", [
        pytest.param("-version", "Version 1.4", r'(Version)\s*([\d\.]*)', '1.4'),
        pytest.param("--version", "Unknown", r'(Version)\s*([\d\.]*)', None),
        pytest.param("/version", "Version 1.6", r'(Version)\s*([\d\.]*)', '1.6')
    ]
)
def test_validate_build_tool_from_path(test_version_query, test_version_result, test_version_regex, test_expected_version):

    # Test Data
    tool_name = 'test'
    tool_config_subpath = 'bin'
    tool_cmd = 'validate.exe'
    tool_config_key = 'validate.home'
    tool_config_value = f'{os.sep}home{os.sep}path{os.sep}invalid_validator'
    tool_config_full_path = os.path.join(tool_config_value, tool_config_subpath, tool_cmd)
    tool_env_key = 'VALIDATE_HOME'
    tool_env_value = f'{os.sep}home{os.sep}path{os.sep}validator'
    tool_env_full_path = os.path.join(tool_env_value, tool_config_subpath, tool_cmd)
    path_value = f'{os.sep}system{os.sep}bin'
    full_path = os.path.join(path_value, tool_cmd)
    tool_version_arg = test_version_query
    tool_version_regex = test_version_regex

    # Mocks
    with patch('o3de.command_utils.O3DEConfig') as mock_android_config, \
         patch('os.getenv') as mock_os_getenv, \
         patch('shutil.which') as mock_shutil_which, \
         patch('o3de.android_support.subprocess.run') as mock_subprocess_run:

        mock_android_config.get_value.return_value = tool_config_value

        mock_os_getenv.return_value = tool_env_value
        def _mock_subprocess_run_(*popenargs, input=None, capture_output=False, timeout=None, check=False, **kwargs):

            full_cmd_line_args = popenargs[0]

            if full_cmd_line_args[0] in (tool_config_full_path, tool_env_full_path):

                mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
                mock_completed_process.args = full_cmd_line_args
                mock_completed_process.returncode = 1
                mock_completed_process.stdout = ""
                mock_completed_process.stderr = ""
                return mock_completed_process

            full_cmd_line = ' '.join(full_cmd_line_args)

            # nonlocal tool_config_value, tool_config_subpath, tool_cmd
            expected_cmd_line = f'{tool_cmd} {tool_version_arg}'
            assert full_cmd_line == expected_cmd_line

            mock_completed_process = PropertyMock(spec=subprocess.CompletedProcess)
            mock_completed_process.args = full_cmd_line_args
            mock_completed_process.returncode = 0
            mock_completed_process.stdout = test_version_result
            mock_completed_process.stderr = ""
            return mock_completed_process

        mock_subprocess_run.side_effect = _mock_subprocess_run_

        mock_shutil_which.return_value = os.path.join(path_value, tool_cmd)

        try:
            # Test the method call
            result_location, result_version = android_support.validate_build_tool(tool_name=tool_name,
                                                                                  tool_command=tool_cmd,
                                                                                  tool_config_key=tool_config_key,
                                                                                  tool_environment_var=tool_env_key,
                                                                                  tool_config_sub_path=tool_config_subpath,
                                                                                  tool_version_arg=tool_version_arg,
                                                                                  version_regex=tool_version_regex,
                                                                                  android_config=mock_android_config)
            # Validate the successful calls
            assert str(result_location) == full_path
            assert result_version == test_expected_version
        except android_support.AndroidToolError:
            # Validate the error condition
            assert test_expected_version is None
        finally:
            mock_os_getenv.assert_called_once_with(tool_env_key)
            mock_shutil_which.assert_called_once_with(tool_cmd)