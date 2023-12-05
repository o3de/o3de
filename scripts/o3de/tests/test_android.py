#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pathlib
import os
import pytest
import re

from o3de import android, android_support, command_utils
from unittest.mock import patch, MagicMock, Mock, PropertyMock


def test_validate_android_config_happy_path(tmpdir):

    tmpdir.ensure('test.keystore', dir=False)
    test_key_store_path = tmpdir.join('test.keystore').realpath()

    # Test Data
    test_gradle_version = '8.4'
    test_validate_java_environment_result = '1.17'
    test_android_support_validate_gradle_result = ('/home/gradle-8.4/', test_gradle_version)

    test_cmake_path = "/path/cmake"
    test_cmake_version = "3.22"
    test_ninja_path = "/path/ninja"
    test_ninja_version = "1.10.1"

    # Mocks
    with patch('o3de.android_support.validate_java_environment') as mock_validate_java_environment, \
         patch('o3de.android_support.validate_gradle') as mock_validate_gradle, \
         patch('o3de.android_support.validate_cmake') as mock_validate_cmake, \
         patch('o3de.android_support.validate_ninja') as mock_validate_ninja, \
         patch('o3de.android_support.get_android_gradle_plugin_requirements') as mock_get_android_gradle_plugin_requirements, \
         patch('o3de.android_support.AndroidSDKManager') as mock_get_AndroidSDKManager, \
         patch('o3de.android.logger.warn') as mock_warn:

        mock_validate_java_environment.return_value = test_validate_java_environment_result
        mock_validate_gradle.return_value = test_android_support_validate_gradle_result
        mock_validate_cmake.return_value = (test_cmake_path, test_cmake_version)
        mock_validate_ninja.return_value = (test_ninja_path, test_ninja_version)

        mock_gradle_requirements = Mock()
        mock_get_android_gradle_plugin_requirements.return_value = mock_gradle_requirements

        mock_sdk_manager = Mock()
        mock_get_AndroidSDKManager.return_value = mock_sdk_manager

        def _android_config_get_value_(input, default=None):
            if input == android_support.SETTINGS_GRADLE_PLUGIN_VERSION.key:
                return "8.4"
            elif input == android_support.SETTINGS_SIGNING_STORE_FILE.key:
                return str(test_key_store_path)
            elif input == android_support.SETTINGS_SIGNING_STORE_PASSWORD.key:
                return "test_store_file_password"
            elif input == android_support.SETTINGS_SIGNING_KEY_ALIAS.key:
                return "test_key_alias"
            elif input == android_support.SETTINGS_SIGNING_KEY_PASSWORD.key:
                return "test_key_password"
            else:
                return default

        mock_android_config = Mock(spec=command_utils.O3DEConfig)
        mock_android_config.get_value.side_effect = _android_config_get_value_

        # Call the method
        result = android.validate_android_config(mock_android_config)

        # Validation
        mock_gradle_requirements.validate_gradle_version.assert_called_with(test_gradle_version)
        mock_gradle_requirements.validate_java_version.assert_called_with(test_validate_java_environment_result)
        mock_sdk_manager.check_licenses.assert_called()
        mock_validate_java_environment.assert_called_once()
        mock_validate_gradle.assert_called_once_with(mock_android_config)
        mock_validate_cmake.assert_called_once_with(mock_android_config)
        mock_validate_ninja.assert_called_once_with(mock_android_config)

        assert result.cmake_path == test_cmake_path
        assert result.cmake_version == test_cmake_version
        assert result.ninja_path == test_ninja_path
        assert result.ninja_version == test_ninja_version
        assert result.sdk_manager == mock_sdk_manager

        assert mock_warn.call_count == 0


@pytest.mark.xfail(raises=android_support.AndroidToolError)
def test_validate_android_config_bad_keystore_path(tmpdir):

    # Test Data
    mock_validate_java_environment_result = '1.17'
    mock_android_support_validate_gradle_result = ('/home/gradle-8.4/', '8.4')

    test_cmake_path = "/path/cmake"
    test_cmake_version = "3.22"
    test_ninja_path = "/path/ninja"
    test_ninja_version = "1.10.1"

    # Mocks
    with patch('o3de.android_support.validate_java_environment') as mock_validate_java_environment, \
         patch('o3de.android_support.validate_gradle') as mock_validate_gradle, \
         patch('o3de.android_support.validate_cmake') as mock_validate_cmake, \
         patch('o3de.android_support.validate_ninja') as mock_validate_ninja, \
         patch('o3de.android_support.get_android_gradle_plugin_requirements') as mock_get_android_gradle_plugin_requirements, \
         patch('o3de.android_support.AndroidSDKManager') as mock_get_AndroidSDKManager:

        mock_validate_java_environment.return_value = mock_validate_java_environment_result
        mock_validate_gradle.return_value = mock_android_support_validate_gradle_result
        mock_validate_cmake.return_value = (test_cmake_path, test_cmake_version)
        mock_validate_ninja.return_value = (test_ninja_path, test_ninja_version)

        mock_gradle_requirements = Mock()
        mock_get_android_gradle_plugin_requirements.return_value = mock_gradle_requirements

        mock_sdk_manager = Mock()
        mock_get_AndroidSDKManager.return_value = mock_sdk_manager

        mock_android_config = Mock(spec=command_utils.O3DEConfig)
        def _android_config_get_value_(input, default=None):
            if input == android_support.SETTINGS_GRADLE_PLUGIN_VERSION.key:
                return "8.4"
            elif input == android_support.SETTINGS_SIGNING_STORE_FILE.key:
                return "test_key_store_path"
            elif input == android_support.SETTINGS_SIGNING_STORE_PASSWORD.key:
                return "test_store_file_password"
            elif input == android_support.SETTINGS_SIGNING_KEY_ALIAS.key:
                return "test_key_alias"
            elif input == android_support.SETTINGS_SIGNING_KEY_PASSWORD.key:
                return "test_key_password"
            else:
                return default

        mock_android_config.get_value.side_effect = _android_config_get_value_

        # Test the method
        android.validate_android_config(mock_android_config)
        mock_validate_java_environment.assert_called_once()
        mock_validate_gradle.assert_called_once_with(mock_android_config)
        mock_validate_cmake.assert_called_once_with(mock_android_config)
        mock_validate_ninja.assert_called_once_with(mock_android_config)


@pytest.mark.parametrize(
    "test_sc_store_file, test_store_pw, test_key_alias, test_key_password, expected_warning", [
        pytest.param('', '', '', '', android.VALIDATION_WARNING_SIGNCONFIG_NOT_SET, id='no_signing_configs'),
        pytest.param('test.keystore', '', '', '', android.VALIDATION_WARNING_SIGNCONFIG_INCOMPLETE, id='only_keystore'),
        pytest.param('', '', 'test_key_alias', '', android.VALIDATION_WARNING_SIGNCONFIG_INCOMPLETE, id='only_keyalias'),
        pytest.param('test.keystore', '', 'test_key_alias', '', android.VALIDATION_MISSING_PASSWORD, id='no_passwords'),
        pytest.param('test.keystore', '1234', 'test_key_alias', '', android.VALIDATION_MISSING_PASSWORD, id='store_password_only'),
        pytest.param('test.keystore', '', 'test_key_alias', '1234', android.VALIDATION_MISSING_PASSWORD, id='key_passwords_only')
    ]
)
def test_validate_android_signing_config_warnings(tmpdir, test_sc_store_file, test_store_pw, test_key_alias,
                                                  test_key_password, expected_warning):
    tmpdir.ensure('test.keystore', dir=False)

    # Test Data
    test_gradle_version = '8.4'
    test_validate_java_environment_result = '1.17'
    test_android_support_validate_gradle_result = ('/home/gradle-8.4/', test_gradle_version)

    test_cmake_path = "/path/cmake"
    test_cmake_version = "3.22"
    test_ninja_path = "/path/ninja"
    test_ninja_version = "1.10.1"

    # Mocks
    with patch('o3de.android_support.validate_java_environment') as mock_validate_java_environment, \
         patch('o3de.android_support.validate_gradle') as mock_validate_gradle, \
         patch('o3de.android_support.validate_cmake') as mock_validate_cmake, \
         patch('o3de.android_support.validate_ninja') as mock_validate_ninja, \
         patch('o3de.android_support.get_android_gradle_plugin_requirements') as mock_get_android_gradle_plugin_requirements, \
         patch('o3de.android_support.AndroidSDKManager') as mock_get_AndroidSDKManager, \
         patch('o3de.android.logger.warning') as mock_warn:

        mock_validate_java_environment.return_value = test_validate_java_environment_result
        mock_validate_gradle.return_value = test_android_support_validate_gradle_result
        mock_validate_cmake.return_value = (test_cmake_path, test_cmake_version)
        mock_validate_ninja.return_value = (test_ninja_path, test_ninja_version)

        mock_gradle_requirements = Mock()
        mock_get_android_gradle_plugin_requirements.return_value = mock_gradle_requirements

        mock_sdk_manager = Mock()
        mock_get_AndroidSDKManager.return_value = mock_sdk_manager

        mock_android_config = Mock(spec=command_utils.O3DEConfig)

        def _mock_android_config_get_value_(input, default=None):

            if input == android_support.SETTINGS_GRADLE_PLUGIN_VERSION.key:
                return "8.4"
            elif input == android_support.SETTINGS_SIGNING_STORE_FILE.key:

                if test_sc_store_file:
                    tmpdir.ensure(test_sc_store_file, dir=False)
                    test_key_store_path = tmpdir.join(test_sc_store_file).realpath()
                    return str(test_key_store_path)
                else:
                    return ""
            elif input == android_support.SETTINGS_SIGNING_STORE_PASSWORD.key:
                return test_store_pw
            elif input == android_support.SETTINGS_SIGNING_KEY_ALIAS.key:
                return test_key_alias
            elif input == android_support.SETTINGS_SIGNING_KEY_PASSWORD.key:
                return test_key_password
            else:
                return default

        mock_android_config.get_value.side_effect = _mock_android_config_get_value_

        # Test the method
        android.validate_android_config(mock_android_config)

        # Validation
        mock_gradle_requirements.validate_gradle_version.assert_called_with(test_gradle_version)
        mock_gradle_requirements.validate_java_version.assert_called_with(test_validate_java_environment_result)
        mock_sdk_manager.check_licenses.assert_called()
        mock_validate_java_environment.assert_called_once()
        mock_validate_gradle.assert_called_once_with(mock_android_config)
        mock_validate_cmake.assert_called_once_with(mock_android_config)
        mock_validate_ninja.assert_called_once_with(mock_android_config)

        assert mock_warn.call_count == 1
        mock_call_args = mock_warn.mock_calls[0].args
        assert mock_call_args[0] == expected_warning


def test_list_android_config_local():

    # Test Data
    test_project_name = 'foo'
    all_settings = [
        ('one', 'uno', None),
        ('two', 'dos', None),
        ('three', 'tres', None),
        ('four', 'quattro', test_project_name)
    ]

    # Mocks
    with patch('builtins.print') as mock_print:

        mock_config = PropertyMock(spec=command_utils.O3DEConfig)
        mock_config.get_all_values.return_value = all_settings
        mock_config.is_global = False
        mock_config.project_name = test_project_name

        # Test the method
        android.list_android_config(mock_config)

        # Validation
        assert mock_print.call_count == len(all_settings) + 2  # the extra two are for the header and the non-global addendum


def test_list_android_config_global():

    # Test Data
    all_settings = [
        ('one', 'uno', None),
        ('two', 'dos', None),
        ('three', 'tres', None)
    ]

    # Mocks
    with patch('builtins.print') as mock_print:

        mock_config = PropertyMock(spec=command_utils.O3DEConfig)
        mock_config.get_all_values.return_value = all_settings
        mock_config.is_global = True
        mock_config.project_name = ""

        # Test the method
        android.list_android_config(mock_config)

        # Validation
        assert mock_print.call_count == len(all_settings) + 1  # the extra one is for the header


def test_get_android_config_from_args_global():

    # Test Data
    test_args = PropertyMock
    setattr(test_args, 'global', True)

    # Mocks
    with patch('o3de.android_support.get_android_config') as mock_get_android_config, \
         patch('o3de.android.logger.warn') as mock_warning:

        mock_get_android_config.return_value = Mock()

        # Test the method
        result, _ = android.get_android_config_from_args(test_args)

        # Validation
        assert result == mock_get_android_config.return_value
        assert mock_warning.call_count == 0
        assert mock_get_android_config.call_count == 1
        mock_call_args = mock_get_android_config.call_args_list[0]
        assert 'project_path' in mock_call_args.kwargs
        assert mock_call_args.kwargs['project_path'] is None


def test_get_android_config_from_args_global_with_warning_project_name():

    # Test Data
    test_args = PropertyMock
    setattr(test_args, 'global', True)
    setattr(test_args, 'project', "Foo")

    # Mocjs
    with patch('o3de.android_support.get_android_config') as mock_get_android_config, \
         patch('o3de.android.logger.warning') as mock_warning:

        mock_get_android_config.return_value = Mock()

        # Test the method
        result = android.get_android_config_from_args(test_args)

        # Validation
        assert mock_warning.call_count == 1
        assert mock_get_android_config.call_count == 1
        mock_call_args = mock_get_android_config.call_args_list[0]
        assert 'project_path' in mock_call_args.kwargs
        assert mock_call_args.kwargs['project_path'] is None


def test_get_android_config_no_project_name_no_project_detected():

    # Test Data
    test_args = PropertyMock
    setattr(test_args, 'global', False)
    setattr(test_args, 'project', "")

    # Mocks
    with patch('o3de.android_support.get_android_config') as mock_get_android_config, \
         patch('o3de.android.logger.info') as mock_info, \
         patch('o3de.command_utils.resolve_project_name_and_path', new_callable=MagicMock) as mock_resolve:

        mock_resolve.side_effect = command_utils.O3DEConfigError

        mock_get_android_config.return_value = Mock()

        # Test the method
        result, project_name = android.get_android_config_from_args(test_args)

        # Validation
        assert result == mock_get_android_config.return_value
        assert project_name is None
        assert mock_info.call_count == 1
        assert re.search(r'(based on the global settings)', mock_info.call_args.args[0]) is not None
        assert mock_get_android_config.call_count == 1
        assert 'project_path' in mock_get_android_config.call_args_list[0].kwargs
        assert mock_get_android_config.call_args_list[0].kwargs['project_path'] is None


def test_get_android_config_no_project_name_project_detected():

    # Test Data
    detected_project = "Foo"
    detected_path = '/foo'
    test_args = PropertyMock
    setattr(test_args, 'global', False)
    setattr(test_args, 'project', "")

    # Mocks
    with patch('o3de.android_support.get_android_config') as mock_get_android_config, \
         patch('o3de.android.logger.info') as mock_info, \
         patch('o3de.command_utils.resolve_project_name_and_path', new_callable=MagicMock) as mock_resolve:

        mock_resolve.return_value = (detected_project, detected_path)

        mock_get_android_config.return_value = Mock()

        # Test the method
        result, project_name = android.get_android_config_from_args(test_args)

        # Validation
        assert result == mock_get_android_config.return_value
        assert project_name == detected_project
        assert mock_info.call_count == 1
        assert re.search(f'(based on the currently detected project \\({detected_project}\\))', mock_info.call_args.args[0]) is not None
        assert mock_get_android_config.call_count == 1
        assert 'project_path' in mock_get_android_config.call_args_list[0].kwargs
        assert mock_get_android_config.call_args_list[0].kwargs['project_path'] == detected_path


def test_get_android_config_project_path(tmpdir):

    tmpdir.ensure("project", dir=True)


    # Test Data
    test_path = pathlib.Path(tmpdir.join('project').realpath())
    test_project = 'project'


    test_args = PropertyMock
    setattr(test_args, 'global', False)
    setattr(test_args, 'project', str(test_path))

    # Mocks
    with patch('o3de.android_support.get_android_config') as mock_get_android_config, \
         patch('o3de.android.logger.info') as mock_info, \
         patch('o3de.command_utils.resolve_project_name_and_path', new_callable=MagicMock) as mock_resolve:

        mock_resolve.return_value = (test_project, test_path)

        mock_get_android_config.return_value = Mock()

        # Test the method
        result, project_name = android.get_android_config_from_args(test_args)

        # Validation
        assert result == mock_get_android_config.return_value
        assert project_name == test_project
        assert mock_info.call_count == 1
        assert re.search(f'(will be based on project \\({test_project}\\))', mock_info.call_args.args[0]) is not None
        assert mock_get_android_config.call_count == 1
        assert 'project_path' in mock_get_android_config.call_args_list[0].kwargs
        assert mock_get_android_config.call_args_list[0].kwargs['project_path'] == test_path


def test_get_android_config_no_global_project_name_provided():

    # Test Data
    test_project = "Foo"
    test_project_path = pathlib.Path('/foo')
    test_args = PropertyMock
    setattr(test_args, 'global', False)
    setattr(test_args, 'project', test_project)

    # Mocks
    with patch('o3de.android_support.get_android_config') as mock_get_android_config, \
         patch('o3de.manifest.get_registered') as mock_get_registered, \
         patch('o3de.android.logger.info') as mock_info:

        mock_get_android_config.return_value = Mock()

        mock_get_registered.return_value = test_project_path

        # Test the method
        result, project_name = android.get_android_config_from_args(test_args)

        # Validation
        assert result == mock_get_android_config.return_value
        assert project_name == test_project
        assert mock_info.call_count == 1
        assert re.search(f'(will be based on project \\({test_project}\\))', mock_info.call_args.args[0]) is not None
        assert mock_get_android_config.call_count == 1
        assert 'project_path' in mock_get_android_config.call_args_list[0].kwargs
        assert mock_get_android_config.call_args_list[0].kwargs['project_path'] == test_project_path


def test_configure_android_options_list():

    # Test Data
    test_args = PropertyMock
    setattr(test_args, 'debug', False)
    setattr(test_args, 'list', True)
    setattr(test_args, 'validate', False)
    setattr(test_args, 'set_value', False)
    setattr(test_args, 'set_password', False)
    setattr(test_args, 'clear_value', False)

    # Mocks
    with patch('o3de.android.get_android_config_from_args') as mock_get_android_config_from_args, \
         patch('o3de.android.list_android_config') as mock_list_android, \
         patch('o3de.android.logger.setLevel') as mock_logger_set_level, \
         patch('o3de.android.logger.info') as mock_logger_info:

        mock_android_config = Mock()

        mock_get_android_config_from_args.return_value = (mock_android_config, 'foo')

        # Test the method
        android.configure_android_options(test_args)

        # Validation
        mock_get_android_config_from_args.assert_called_once_with(test_args)
        mock_list_android.assert_called_once_with(mock_android_config)


def test_configure_android_options_validate():

    # Test Data
    test_args = PropertyMock
    setattr(test_args, 'debug', False)
    setattr(test_args, 'list', False)
    setattr(test_args, 'validate', True)
    setattr(test_args, 'set_value', False)
    setattr(test_args, 'set_password', False)
    setattr(test_args, 'clear_value', False)

    # Mocks
    with patch('o3de.android.get_android_config_from_args') as mock_get_android_config_from_args, \
         patch('o3de.android.validate_android_config') as mock_validate_android_config, \
         patch('o3de.android.logger.setLevel') as mock_logger_set_level, \
         patch('o3de.android.logger.info') as mock_logger_info:

        mock_android_config = Mock()

        mock_get_android_config_from_args.return_value = (mock_android_config, 'foo')

        # Test the method
        android.configure_android_options(test_args)

        # Validation
        mock_get_android_config_from_args.assert_called_once_with(test_args)
        mock_validate_android_config.assert_called_once_with(mock_android_config)


def test_configure_android_options_set_value():

    # Test Data
    test_value = 'test.key=FooValue'
    test_args = PropertyMock
    setattr(test_args, 'debug', False)
    setattr(test_args, 'list', False)
    setattr(test_args, 'validate', False)
    setattr(test_args, 'set_value', test_value)
    setattr(test_args, 'set_password', False)
    setattr(test_args, 'clear_value', False)

    # Mocks
    with patch('o3de.android.get_android_config_from_args') as mock_get_android_config_from_args, \
         patch('o3de.android.logger.setLevel') as mock_logger_set_level, \
         patch('o3de.android.logger.info') as mock_logger_info:

        mock_android_config = Mock()

        mock_get_android_config_from_args.return_value = (mock_android_config, 'foo')

        # Test the method
        android.configure_android_options(test_args)

        # Validation
        assert mock_get_android_config_from_args.call_count == 1
        mock_android_config.set_config_value_from_expression.assert_called_once_with(test_value)


def test_configure_android_options_set_password():

    # Test Data
    test_value = 'test.key'
    test_args = PropertyMock
    setattr(test_args, 'debug', False)
    setattr(test_args, 'list', False)
    setattr(test_args, 'validate', False)
    setattr(test_args, 'set_value', False)
    setattr(test_args, 'set_password', test_value)
    setattr(test_args, 'clear_value', False)

    # Mocks
    with patch('o3de.android.get_android_config_from_args') as mock_get_android_config_from_args, \
         patch('o3de.android.logger.setLevel') as mock_logger_set_level, \
         patch('o3de.command_utils.logger.info') as mock_logger_info:

        mock_android_config = Mock(spec=command_utils.O3DEConfig)

        mock_get_android_config_from_args.return_value = (mock_android_config, 'foo')

        # Test the method
        android.configure_android_options(test_args)

        # Validation
        assert mock_get_android_config_from_args.call_count == 1
        mock_android_config.set_password.assert_called_once_with(test_value)


def test_configure_android_options_clear_value():

    # Test Data
    test_value = 'test.key'
    test_args = PropertyMock
    setattr(test_args, 'debug', False)
    setattr(test_args, 'list', False)
    setattr(test_args, 'validate', False)
    setattr(test_args, 'set_value', None)
    setattr(test_args, 'set_password', None)
    setattr(test_args, 'clear_value', test_value)

    # Mocks
    with patch('o3de.android.get_android_config_from_args') as mock_get_android_config_from_args, \
         patch('o3de.android.logger.setLevel') as mock_logger_set_level, \
         patch('o3de.android.logger.info') as mock_logger_info:

        mock_android_config = Mock()

        mock_get_android_config_from_args.return_value = (mock_android_config, 'foo')

        # Test the method
        android.configure_android_options(test_args)

        # Validation
        assert mock_get_android_config_from_args.call_count == 1
        mock_android_config.set_config_value.assert_called_once_with(key=test_value, value='', validate_value=False)
        assert mock_logger_info.call_count == 1


def test_configure_android_options_validate_error():

    # Test Data
    test_args = PropertyMock
    setattr(test_args, 'debug', False)
    setattr(test_args, 'list', False)
    setattr(test_args, 'validate', True)
    setattr(test_args, 'set_value', False)
    setattr(test_args, 'set_password', False)
    setattr(test_args, 'clear_value', False)

    # Mocks
    with patch('o3de.android.get_android_config_from_args') as mock_get_android_config_from_args, \
         patch('o3de.android.validate_android_config') as mock_validate_android_config, \
         patch('o3de.android.logger.setLevel') as mock_logger_set_level, \
         patch('o3de.android.logger.error') as mock_logger_error:

        mock_android_config = Mock()
        mock_get_android_config_from_args.return_value = (mock_android_config, 'foo')
        mock_validate_android_config.side_effect = android_support.AndroidToolError("BAD")

        result = android.configure_android_options(test_args)

        # Validation
        assert result == 1
        assert mock_logger_error.call_count == 1


def test_prompt_password_empty_error():

    # Test Data
    test_key_name = 'Foo'

    # Mocks
    with patch('o3de.android.getpass') as mock_get_pass:

        mock_get_pass.return_value = ""

        try:
            # Test the method
            android.prompt_validated_password(test_key_name)

        except android_support.AndroidToolError as err:
            # Validate the error
            assert f"Password for {test_key_name} required." == str(err)
        else:
            assert False, "Error expected"


def test_prompt_password_mismatch_error():

    # Test Data
    test_key_name = 'Foo'

    # Mocks
    with patch('o3de.android.getpass') as mock_get_pass:

        get_pass_call_count = 0
        def _mock_get_pass_(prompt='Password: ', stream=None):
            nonlocal get_pass_call_count
            get_pass_call_count += 1
            return f"foo{get_pass_call_count}"

        mock_get_pass.side_effect = _mock_get_pass_

        # Test the method
        try:
            android.prompt_validated_password(test_key_name)
        except android_support.AndroidToolError as err:
            # Validate the error
            assert f"Passwords for {test_key_name} do not match." == str(err)
        else:
            assert False, "Error expected"


def test_prompt_password_success():

    # Test Data
    test_key_name = 'Foo'
    test_password = '1111'

    # Mocks
    with patch('o3de.android.getpass') as mock_get_pass:

        mock_get_pass.return_value = test_password

        # Test the method
        result = android.prompt_validated_password(test_key_name)

        # Validation
        assert result == test_password
        assert mock_get_pass.call_count == 2


def test_generate_android_project_success(tmpdir):

    # Test Data
    test_project_name = "Foo"
    test_project_root = f"{os.sep}foo{os.sep}bar{os.sep}Foo"
    test_java_version = '17.1'
    test_gradle_path = f"{os.sep}usr{os.sep}bin{os.sep}gradle"
    test_gradle_version = '8.4'
    test_android_grade_plugin_version = '8.1'
    test_cmake_path = f"{os.sep}usr{os.sep}bin{os.sep}cmake"
    test_cmake_version = '3.25'
    test_ninja_path = f"{os.sep}usr{os.sep}bin{os.sep}ninja"
    test_ninja_version = '1.10'
    test_sdk_build_tools_version = "1.10.1"
    test_extra_cmake_args = '-DLY_FOO=ON'
    test_custom_jvm_args = '-Xm1024m'
    test_build_dir = f'{os.sep}home{os.sep}project{os.sep}android'
    test_sc_store_file = 'o3de.keystore'
    test_sc_key_alias = 'test_key_alias'
    test_store_pw = '1111'
    test_key_password = '2222'
    test_android_sdk_path = f'{os.sep}usr{os.sep}android{os.sep}sdk'
    test_platform_api_level = '31'
    test_asset_mode = 'LOOSE'
    test_bundle_subpath = f'AssetBundling{os.sep}Bundles'
    test_ndk_version = '25.*'

    tmpdir.ensure(test_sc_store_file, dir=False)
    test_key_store_path = tmpdir.join(test_sc_store_file).realpath()

    test_args = PropertyMock
    setattr(test_args, 'debug', False)
    setattr(test_args, 'extra_cmake_args', test_extra_cmake_args)
    setattr(test_args, 'custom_jvm_args', test_custom_jvm_args)
    setattr(test_args, 'build_dir', test_build_dir)
    setattr(test_args, 'signconfig_store_file', str(test_key_store_path))
    setattr(test_args, 'signconfig_key_alias', test_sc_key_alias)
    setattr(test_args, 'platform_sdk_api_level', test_platform_api_level)
    setattr(test_args, 'asset_mode', test_asset_mode)
    setattr(test_args, 'ndk_version', test_ndk_version)

    # Mocks
    with patch('o3de.android.get_android_config_from_args') as mock_get_android_config_from_args, \
         patch('o3de.manifest.get_registered') as mock_manifest_get_registered, \
         patch('o3de.android_support.read_android_settings_for_project') as mock_read_android_settings_for_project, \
         patch('o3de.android.validate_android_config') as mock_validate_android_config, \
         patch('o3de.android_support.AndroidProjectGenerator') as mock_get_android_project_generator, \
         patch('o3de.android_support.AndroidSigningConfig') as mock_get_signing_config, \
         patch('o3de.android.logger.setLevel') as mock_logger_set_level, \
         patch('o3de.android.logger.error') as mock_logger_error:

        def _mock_android_config_get_value_(key, default=None):
            if key == android_support.SETTINGS_GRADLE_PLUGIN_VERSION.key:
                return test_android_grade_plugin_version
            elif key == android_support.SETTINGS_ASSET_BUNDLE_SUBPATH.key:
                return test_bundle_subpath
            elif key == android_support.SETTINGS_SIGNING_STORE_PASSWORD.key:
                return test_store_pw
            elif key == android_support.SETTINGS_SIGNING_KEY_PASSWORD.key:
                return test_key_password
            else:
                assert False

        def _mock_android_config_get_boolean_value_(key: str, default: bool = False):
            if key == android_support.SETTINGS_STRIP_DEBUG.key:
                return True
            elif key == android_support.SETTINGS_OCULUS_PROJECT.key:
                return False
            else:
                assert False

        mock_android_config = Mock(spec=command_utils.O3DEConfig)
        mock_android_config.get_value.side_effect = _mock_android_config_get_value_
        mock_android_config.get_boolean_value = _mock_android_config_get_boolean_value_

        mock_get_android_config_from_args.return_value = (mock_android_config, test_project_name)

        mock_manifest_get_registered.return_value = test_project_root

        mock_project_settings = Mock()
        mock_android_settings = Mock()

        mock_read_android_settings_for_project.return_value = (mock_project_settings, mock_android_settings)

        mock_android_gradle_plugin_requirements = Mock()
        setattr(mock_android_gradle_plugin_requirements,'sdk_build_tools_version', test_sdk_build_tools_version)

        mock_sdk_manager = Mock()
        mock_sdk_manager.get_android_sdk_path.return_value = test_android_sdk_path

        mock_ndk_package = Mock()
        mock_sdk_manager.install_package.return_value = mock_ndk_package

        test_android_env = android.ValidatedEnv(java_version=test_java_version,
                                                gradle_home=test_gradle_path,
                                                gradle_version=test_gradle_version,
                                                cmake_path=test_cmake_path,
                                                cmake_version=test_cmake_version,
                                                ninja_path=test_ninja_path,
                                                ninja_version=test_ninja_version,
                                                android_gradle_plugin_ver=test_android_grade_plugin_version,
                                                sdk_build_tools_version=test_sdk_build_tools_version,
                                                sdk_manager=mock_sdk_manager)
        mock_validate_android_config.return_value = test_android_env


        mock_android_project_generator = Mock()
        mock_get_android_project_generator.return_value = mock_android_project_generator

        mock_signing_config = Mock()
        mock_get_signing_config.return_value = mock_signing_config

        # Test the method
        result = android.generate_android_project(test_args)

        # Validation
        mock_manifest_get_registered.assert_called_once_with(project_name=test_project_name)

        mock_read_android_settings_for_project.assert_called_once_with(test_project_root)

        mock_validate_android_config.assert_called_once()

        assert mock_sdk_manager.install_package.call_count == 5

        mock_get_signing_config.assert_called_once_with(store_file=pathlib.Path(test_key_store_path),
                                                        store_password=test_store_pw,
                                                        key_alias=test_sc_key_alias,
                                                        key_password=test_key_password)

        mock_get_android_project_generator.assert_called_once_with(engine_root=android.ENGINE_PATH,
                                                                   android_build_dir=pathlib.Path(test_build_dir),
                                                                   android_sdk_path=test_android_sdk_path,
                                                                   android_build_tool_version=test_sdk_build_tools_version,
                                                                   android_platform_sdk_api_level=test_platform_api_level,
                                                                   android_ndk_package=mock_ndk_package,
                                                                   project_name=test_project_name,
                                                                   project_path=test_project_root,
                                                                   project_general_settings=mock_project_settings,
                                                                   project_android_settings=mock_android_settings,
                                                                   cmake_version=test_cmake_version,
                                                                   cmake_path=test_cmake_path,
                                                                   gradle_path=test_gradle_path,
                                                                   gradle_version=test_gradle_version,
                                                                   gradle_custom_jvm_args=test_custom_jvm_args,
                                                                   android_gradle_plugin_version=test_android_grade_plugin_version,
                                                                   ninja_path=test_ninja_path,
                                                                   asset_mode=test_asset_mode,
                                                                   signing_config=mock_signing_config,
                                                                   extra_cmake_configure_args=test_extra_cmake_args,
                                                                   overwrite_existing=True,
                                                                   strip_debug_symbols=True,
                                                                   src_pak_file_path=test_bundle_subpath,
                                                                   oculus_project=False)

        mock_android_project_generator.execute.assert_called_once()
