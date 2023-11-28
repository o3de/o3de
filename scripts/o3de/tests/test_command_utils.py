#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import configparser
import pytest
import pathlib

from unittest.mock import patch

from o3de import command_utils


def test_apply_default_values_create_new_settings_file(tmpdir):

    # Test Data
    tmpdir.ensure('.o3de', dir=True)

    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'
    settings = [command_utils.SettingsDescription("foo", "foo desc", "1"),
                command_utils.SettingsDescription("bar", "bar desc", "2")]

    # Mocks
    with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder:

        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        # Test the method
        result_settings_file = command_utils.O3DEConfig.apply_default_global_settings(settings_filename=settings_filename,
                                                                                      settings_section_name=settings_section_name,
                                                                                      settings_descriptions=settings)
        # Validation
        mock_get_o3de_folder.assert_called()
        expected_settings_file_path = pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())
        assert result_settings_file == expected_settings_file_path
        assert pathlib.Path(expected_settings_file_path).is_file()
        config_reader = configparser.ConfigParser()
        config_reader.read(result_settings_file.absolute())
        settings = config_reader[settings_section_name]
        assert settings['foo'] == '1'
        assert settings['bar'] == '2'


def test_apply_default_values_open_existing_settings_file_new_entries(tmpdir):

    tmpdir.ensure('.o3de', dir=True)

    # Test Data
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'
    settings = [command_utils.SettingsDescription("foo", "foo desc","1"),
                command_utils.SettingsDescription("bar", "bar desc","2")]

    existing_settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    existing_settings_file.write(f"""
[{settings_section_name}]
foo = 3
crew = 7
""")

    # Mocks
    with patch('o3de.manifest.get_o3de_folder') as mock_get_o3de_folder:

        mock_get_o3de_folder.return_value = pathlib.Path(tmpdir.join('.o3de').realpath())

        # Test the methods
        result_settings_file = command_utils.O3DEConfig.apply_default_global_settings(settings_filename=settings_filename,
                                                                                      settings_section_name=settings_section_name,
                                                                                      settings_descriptions=settings)
        # Validation
        expected_settings_file_path = pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())
        mock_get_o3de_folder.assert_called()

        assert expected_settings_file_path == expected_settings_file_path
        assert pathlib.Path(expected_settings_file_path).is_file()

        config_reader = configparser.ConfigParser()
        config_reader.read(result_settings_file.absolute())
        settings = config_reader[settings_section_name]

        assert settings['foo'] == '3'   # The default setting of 1 should not be applied since 'foo' already exists
        assert settings['bar'] == '2'   # bar will be added
        assert settings['crew'] == '7'  # crew already existed


@pytest.mark.parametrize(
    "test_path, expect_error", [
        pytest.param("my_project", False, id="From same level: success"),
        pytest.param("my_project/Gem", False, id="From one level up: Success"),
        pytest.param("my_project/Gem/Source", False, id="From two levels up: Success"),
        pytest.param("not_a_project/foo", True, id="No project.json found: Error"),
    ]
)
def test_resolve_project_path_from_cwd(tmpdir, test_path, expect_error):

    # Test Data
    tmpdir.ensure('not_a_project/foo', dir=True)
    tmpdir.ensure('my_project/Cache/windows', dir=True)
    tmpdir.ensure('my_project/Gem/Source/foo.cpp')

    dummy_project_file = tmpdir.join('my_project/project.json')
    dummy_project_file.write("""
{
    "project_name": "MyProject",
    "project_id": "{11111111-1111-AAAA-AA11-111111111111}"
}
""")
    # Mocks
    with patch('o3de.manifest.get_all_projects') as mock_get_all_projects:

        mock_get_all_projects.return_value = [tmpdir.join('my_project').realpath()]
        try:
            # Test the method
            command_utils.resolve_project_name_and_path(tmpdir.join(test_path).realpath())

        except command_utils.O3DEConfigError:
            # Validate error conditions
            assert expect_error
        else:
            # Validate success conditions
            assert not expect_error




def test_read_config_global_only(tmpdir):

    # Test Data
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    tmpdir.ensure('.o3de', dir=True)

    settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = ZZZZ
extra1 = XXXX
""")
    settings = [command_utils.SettingsDescription("sdk_api_level", "sdk_api_level desc"),
                command_utils.SettingsDescription("extra1", "extra1 desc")]

    global_settings_file = pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())

    # Mocks
    with patch("o3de.command_utils.O3DEConfig.apply_default_global_settings") as mock_apply_default_global_settings:

        mock_apply_default_global_settings.return_value = global_settings_file

        # Test the method
        config = command_utils.O3DEConfig(project_path=None,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          settings_description_list=settings)

        # Validation
        mock_apply_default_global_settings.assert_called()
        result_sdk_api_level = config.get_value('sdk_api_level')
        result_extra1 = config.get_value('extra1')

        assert result_sdk_api_level == 'ZZZZ'
        assert result_extra1 == 'XXXX'


def test_read_config_value_with_override(tmpdir):

    # Test Data
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'
    project_name = 'foo'

    tmpdir.ensure('.o3de/o3de_manifest.json')
    global_settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    global_settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = ZZZZ
extra1 = XXXX
""")

    tmpdir.ensure('project/project.json')
    project_settings_file = tmpdir.join(f'project/{settings_filename}')
    project_settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = FFFF
""")
    test_project_path = pathlib.Path(tmpdir.join('project').realpath())
    test_project_name = "project"

    settings = [command_utils.SettingsDescription("sdk_api_level", "sdk_api_level desc"),
                command_utils.SettingsDescription("extra1", "extra1 desc")]
    global_settings_file = pathlib.Path(global_settings_file.realpath())
    project_settings_file = pathlib.Path(project_settings_file.realpath())

    # Mocks
    with patch("o3de.command_utils.O3DEConfig.apply_default_global_settings") as mock_apply_default_global_settings, \
         patch("o3de.command_utils.resolve_project_name_and_path") as mock_resolve_project_name_and_path:

        mock_apply_default_global_settings.return_value = global_settings_file

        mock_resolve_project_name_and_path.return_value = (test_project_name, test_project_path)

        # Test the method
        config = command_utils.O3DEConfig(project_path=test_project_path,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          settings_description_list=settings)

        # Validation
        result_sdk_api_level = config.get_value('sdk_api_level')
        assert result_sdk_api_level == 'FFFF'

        result_extra1 = config.get_value('extra1')
        assert result_extra1 == 'XXXX'

        mock_apply_default_global_settings.assert_called_once()
        mock_resolve_project_name_and_path.assert_called_once()


def test_read_config_value_and_source_with_override(tmpdir):

    # Test Data
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    tmpdir.ensure('.o3de/o3de_manifest.json')
    global_settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    global_settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = ZZZZ
extra1 = XXXX
""")

    tmpdir.ensure('project/project.json')
    project_settings_file = tmpdir.join(f'project/{settings_filename}')
    project_settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = FFFF
""")
    test_project_path = pathlib.Path(tmpdir.join('project').realpath())
    test_project_name = "project"

    settings = [command_utils.SettingsDescription("sdk_api_level", "sdk_api_level desc"),
                command_utils.SettingsDescription("extra1", "extra1 desc")]

    # Mocks
    with patch("o3de.command_utils.O3DEConfig.apply_default_global_settings") as mock_apply_default_global_settings, \
         patch("o3de.command_utils.resolve_project_name_and_path") as mock_resolve_project_name_and_path:

        mock_apply_default_global_settings.return_value = pathlib.Path(global_settings_file.realpath())

        mock_resolve_project_name_and_path.return_value = (test_project_name, test_project_path)

        # Test the method
        config = command_utils.O3DEConfig(project_path=test_project_path,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          settings_description_list=settings)

        # Validation
        result_sdk_api_level, result_sdk_api_level_project = config.get_value_and_source('sdk_api_level')
        assert result_sdk_api_level == 'FFFF'
        assert result_sdk_api_level_project == test_project_name

        result_extra1, result_extra1_project = config.get_value_and_source('extra1')
        assert result_extra1 == 'XXXX'
        assert result_extra1_project == ''

        mock_apply_default_global_settings.assert_called_once()
        mock_resolve_project_name_and_path.assert_called_once()


def test_set_config_global(tmpdir):

    # Test Data
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    tmpdir.ensure('.o3de/o3de_manifest.json')
    global_settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    global_settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = ZZZZ
extra1 = XXXX
""")

    settings = [command_utils.SettingsDescription("sdk_api_level", "sdk_api_level desc"),
                command_utils.SettingsDescription("extra1", "extra1 desc")]

    # Mocks
    with patch("o3de.command_utils.O3DEConfig.apply_default_global_settings") as mock_apply_default_global_settings:

        mock_apply_default_global_settings.return_value = pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())

        # Test the method
        config = command_utils.O3DEConfig(project_path=None,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          settings_description_list=settings)

        config.set_config_value('extra1', 'ZZZZ')

        # Validation
        result_extra1 = config.get_value('extra1')
        assert result_extra1 == 'ZZZZ'
        mock_apply_default_global_settings.assert_called_once()


def test_set_config_project(tmpdir):

    tmpdir.ensure('.o3de', dir=True)

    # Test Data
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    project_name = "foo"

    test_global_settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    test_global_settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = ZZZZ
extra1 = XXXX
""")

    tmpdir.ensure('project/project.json')
    test_project_settings_file = tmpdir.join(f'project/{settings_filename}')
    test_project_settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = FFFF
""")
    test_project_path = pathlib.Path(tmpdir.join('project').realpath())
    test_project_name = "project"

    settings = [command_utils.SettingsDescription("sdk_api_level", "sdk_api_level desc"),
                command_utils.SettingsDescription("extra1", "extra1 desc")]

    # Mocks
    with patch("o3de.command_utils.O3DEConfig.apply_default_global_settings") as mock_apply_default_global_settings, \
         patch("o3de.command_utils.resolve_project_name_and_path") as mock_resolve_project_name_and_path:

        mock_apply_default_global_settings.return_value = pathlib.Path(test_global_settings_file.realpath())
        mock_resolve_project_name_and_path.return_value = (test_project_name, test_project_path)

        # Test the method
        config = command_utils.O3DEConfig(project_path=test_project_path,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          settings_description_list=settings)
        config.set_config_value('extra1','ZZZZ')

        # Validation
        project_config = configparser.ConfigParser()
        project_config.read(test_project_settings_file.realpath())
        project_config_section = project_config[settings_section_name]
        assert project_config_section.get('extra1') == 'ZZZZ'

        global_config = configparser.ConfigParser()
        global_config.read(test_global_settings_file.realpath())
        global_config_section = global_config[settings_section_name]
        assert global_config_section.get('extra1') == 'XXXX'

        mock_apply_default_global_settings.assert_called_once()
        mock_resolve_project_name_and_path.assert_called_once()


@pytest.mark.parametrize(
    "key_and_value, expected_key, expected_value", [
        pytest.param("argument=foo", "argument", "foo", id="Simple"),
        pytest.param("argument =foo", "argument", "foo", id="Simple with space 1"),
        pytest.param("argument= foo", "argument", "foo", id="Simple with space 2"),
        pytest.param("argument = foo", "argument", "foo", id="Simple with space 3"),
        pytest.param("argument='foo* and three.'", "argument", "foo* and three.", id="Double Quotes"),
        pytest.param("argument.one = foo", "argument.one", "foo", id="Simple alpha with dot"),
        pytest.param("argument.1.foo = foo", "argument.1.foo", "foo", id="Simple alpha with number and dot")
    ]
)
def test_set_config_key_value(tmpdir, key_and_value, expected_key, expected_value):

    # Test Data
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    tmpdir.ensure('.o3de/o3de_manifest.json')
    settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = ZZZZ
extra1 = XXXX
""")

    settings = [command_utils.SettingsDescription("sdk_api_level", "sdk_api_level desc"),
                command_utils.SettingsDescription(expected_key, "argument desc"),
                command_utils.SettingsDescription("extra1", "extra1 desc")]
    global_settings_file = pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())

    # Mocks
    with patch("o3de.command_utils.O3DEConfig.apply_default_global_settings") as mock_apply_default_global_settings:

        mock_apply_default_global_settings.return_value = global_settings_file

        # Test the method
        config = command_utils.O3DEConfig(project_path=None,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          settings_description_list=settings)
        config.set_config_value_from_expression(key_and_value)

        # Validation
        global_config = configparser.ConfigParser()
        global_config.read(global_settings_file)
        global_config_section = global_config[settings_section_name]
        assert global_config_section.get(expected_key) == expected_value
        mock_apply_default_global_settings.assert_called_once()


@pytest.mark.xfail(raises=command_utils.O3DEConfigError)
def test_set_config_key_invalid_config_value(tmpdir):

    # Test Data
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    tmpdir.ensure('.o3de/o3de_manifest.json')
    settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = ZZZZ
extra1 = XXXX
""")
    bad_config_value = "foo? and two%"

    settings = [command_utils.SettingsDescription("sdk_api_level", "sdk_api_level desc"),
                command_utils.SettingsDescription("extra1", "extra1 desc")]

    # Mocks
    with patch("o3de.command_utils.O3DEConfig.apply_default_global_settings") as mock_apply_default_global_settings:

        mock_apply_default_global_settings.return_value = pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())

        # Test the method
        config = command_utils.O3DEConfig(project_path=None,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          settings_description_list=settings)

        config.set_config_value_from_expression(f"argument=\"{bad_config_value}\"")


def test_get_all_config_values(tmpdir):

    # Test Data
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    tmpdir.ensure('.o3de/o3de_manifest.json')
    global_settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    global_settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = ZZZZ
ndk = 22.*
extra1 = XXXX
""")

    tmpdir.ensure('project/project.json')
    project_settings_file = tmpdir.join(f'project/{settings_filename}')
    project_settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = FFFF
""")
    test_project_path = pathlib.Path(tmpdir.join('project').realpath())
    test_project_name = "project"

    settings = [command_utils.SettingsDescription("sdk_api_level", "sdk_api_level desc"),
                command_utils.SettingsDescription("ndk", "ndk desc"),
                command_utils.SettingsDescription("extra1", "extra1 desc")]

    # Mocks
    with patch("o3de.command_utils.O3DEConfig.apply_default_global_settings") as mock_apply_default_global_settings, \
         patch("o3de.command_utils.resolve_project_name_and_path") as mock_resolve_project_name_and_path:

        mock_apply_default_global_settings.return_value = pathlib.Path(global_settings_file.realpath())

        mock_resolve_project_name_and_path.return_value = (test_project_name, test_project_path)

        # Test the method
        config = command_utils.O3DEConfig(project_path=test_project_path,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          settings_description_list=settings)

        results = config.get_all_values()

        mock_apply_default_global_settings.assert_called()
        mock_resolve_project_name_and_path.assert_called()

        # Validation
        project_to_value_list = {}
        for key, value, source in results:
            project_to_value_list.setdefault(source or "global", []).append( (key, value) )
        assert len(project_to_value_list['global']) == 2  # Note: Not 3 since 'extra1' is overridden by the project
        assert len(project_to_value_list[test_project_name]) == 1


@pytest.mark.parametrize(
    "bool_str, expected_value, is_error", [
        pytest.param('t', True, False),
        pytest.param('true', True, False),
        pytest.param('T', True, False),
        pytest.param('True', True, False),
        pytest.param('TRUE', True, False),
        pytest.param('1', True, False),
        pytest.param('on', True, False),
        pytest.param('On', True, False),
        pytest.param('ON', True, False),
        pytest.param('yes', True, False),
        pytest.param('Bad', True, True),
        pytest.param('f', False, False),
        pytest.param('F', False, False),
        pytest.param('false', False, False),
        pytest.param('FALSE', False, False),
        pytest.param('False', False, False),
        pytest.param('0', False, False),
        pytest.param('off', False, False),
        pytest.param('OFF', False, False),
        pytest.param('Off', False, False),
        pytest.param('no', False, False)
    ]
)
def test_set_config_boolean_value(tmpdir,bool_str, expected_value, is_error):

    # Test Data
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    tmpdir.ensure('.o3de/o3de_manifest.json')
    global_settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    global_settings_file.write(f"""
[{settings_section_name}]
extra1 = XXXX
""")

    settings = [command_utils.SettingsDescription("sdk_api_level", "sdk_api_level desc"),
                command_utils.SettingsDescription("strip.debug", "strip.debug desc", is_boolean=True),
                command_utils.SettingsDescription("extra1", "extra1 desc")]

    # Mocks
    with patch("o3de.command_utils.O3DEConfig.apply_default_global_settings") as mock_apply_default_global_settings:

        mock_apply_default_global_settings.return_value = pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())

        try:
            # Test the method
            config = command_utils.O3DEConfig(project_name=None,
                                              settings_filename=settings_filename,
                                              settings_section_name=settings_section_name,
                                              settings_description_list=settings)

            config.set_config_value('strip.debug', bool_str)

            # Validate success values
            mock_apply_default_global_settings.assert_called()
            str_result = config.get_value('strip.debug')
            assert str_result == bool_str
            bool_result = config.get_boolean_value('strip.debug')
            assert bool_result == expected_value

        except command_utils.O3DEConfigError as e:
            # Validate errors
            assert is_error, f"Unexpected Error: {e}"
        else:
            assert not is_error, "Error expected"


@pytest.mark.parametrize(
    "input, restricted_regex, is_error", [
        pytest.param('LOOSE', '(LOOSE|PAK)', False),
        pytest.param('PAK', '(LOOSE|PAK)', False),
        pytest.param('VFS', '(LOOSE|PAK)', True),
    ]
)
def test_set_config_boolean_value(tmpdir, input, restricted_regex, is_error):

    # Test Data
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    tmpdir.ensure('.o3de/o3de_manifest.json')
    global_settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    global_settings_file.write(f"""
[{settings_section_name}]
extra1 = XXXX
""")
    key = 'test'
    settings = [command_utils.SettingsDescription(key, f"{key} desc", restricted_regex=restricted_regex, restricted_regex_description=f"regex: {restricted_regex}"),
                command_utils.SettingsDescription("extra1", "extra1 desc")]

    # Mocks
    with patch("o3de.command_utils.O3DEConfig.apply_default_global_settings") as mock_apply_default_global_settings:

        mock_apply_default_global_settings.return_value = pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())

        try:
            config = command_utils.O3DEConfig(project_path=None,
                                              settings_filename=settings_filename,
                                              settings_section_name=settings_section_name,
                                              settings_description_list=settings)

            config.set_config_value(key, input)

            # Validate success conditions
            mock_apply_default_global_settings.assert_called()
            str_result = config.get_value(key)
            assert str_result == input
        except command_utils.O3DEConfigError as e:
            # Validation error conditions
            assert is_error, f"Unexpected Error: {e}"
        else:
            assert not is_error, "Error expected"
