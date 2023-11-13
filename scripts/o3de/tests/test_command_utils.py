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

    tmpdir.ensure('.o3de', dir=True)

    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'
    default_settings = [("foo", "1"),
                        ("bar", "2")]

    with patch(target='o3de.manifest.get_o3de_folder',
               return_value=pathlib.Path(tmpdir.join('.o3de').realpath())):

        result_settings_file = command_utils.O3DEConfig.apply_default_global_settings(settings_filename=settings_filename,
                                                                                      settings_section_name=settings_section_name,
                                                                                      default_settings=default_settings)
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

    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'
    default_settings = [("foo", "1"),
                        ("bar", "2")]

    existing_settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    existing_settings_file.write(f"""
[{settings_section_name}]
foo = 3
crew = 7
""")

    with patch(target='o3de.manifest.get_o3de_folder',
               return_value=pathlib.Path(tmpdir.join('.o3de').realpath())):

        result_settings_file = command_utils.O3DEConfig.apply_default_global_settings(settings_filename=settings_filename,
                                                                                      settings_section_name=settings_section_name,
                                                                                      default_settings=default_settings)
        expected_settings_file_path = pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())

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

    with patch(target='o3de.manifest.get_all_projects',
               return_value=[tmpdir.join('my_project').realpath()]):
        try:
            resolved_name, resolved_path = command_utils.resolve_project_name_and_path(tmpdir.join(test_path).realpath())

        except command_utils.O3DEConfigError:
            assert expect_error
        else:
            assert not expect_error


@pytest.mark.parametrize(
    "project_name", [
        pytest.param("MyProject", id="with project name"),
        pytest.param(None, id="without project name")
    ]
)
def test_resolve_settings_file_by_project(tmpdir, project_name):
    tmpdir.ensure('not_a_project/foo/')
    tmpdir.ensure('my_project/Cache/windows')
    tmpdir.ensure('my_project/Cache/windows')
    tmpdir.ensure('my_project/Gem/Source/foo.cpp')
    dummy_project_file = tmpdir.join('my_project/project.json')
    dummy_project_file.write("""
        {
            "project_name": "MyProject",
            "project_id": "{11111111-1111-AAAA-AA11-111111111111}"
        }
    """)
    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    expected_settings_file = tmpdir.join(f'my_project/{settings_filename}').realpath()
    with patch(target='o3de.manifest.get_registered',
               return_value=pathlib.Path(tmpdir.join('my_project').realpath())), \
         patch(target='o3de.command_utils.resolve_project_name_and_path',
               return_value=('MyProject',pathlib.Path(tmpdir.join('my_project').realpath()))):
        settings_file = command_utils.O3DEConfig.resolve_project_settings_file(project_name=project_name,
                                                                               settings_filename=settings_filename,
                                                                               settings_section_name=settings_section_name,
                                                                               create_default_if_missing=False)
        assert settings_file == expected_settings_file


def test_read_config_global_only(tmpdir):

    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    tmpdir.ensure('.o3de', dir=True)

    settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = ZZZZ
extra1 = XXXX
""")

    global_settings_file = pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())
    with patch(target="o3de.command_utils.O3DEConfig.apply_default_global_settings",
               return_value=global_settings_file):

        config = command_utils.O3DEConfig(project_name=None,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          default_settings=[])

        result_sdk_api_level = config.get_value('sdk_api_level')
        result_extra1 = config.get_value('extra1')

        assert result_sdk_api_level == 'ZZZZ'
        assert result_extra1 == 'XXXX'


def test_read_config_value_with_override(tmpdir):

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

    global_settings_file = pathlib.Path(global_settings_file.realpath())
    project_settings_file = pathlib.Path(project_settings_file.realpath())

    with patch(target="o3de.command_utils.O3DEConfig.apply_default_global_settings",
               return_value=global_settings_file), \
         patch(target="o3de.command_utils.O3DEConfig.resolve_project_settings_file",
               return_value=project_settings_file):
        config = command_utils.O3DEConfig(project_name=project_name,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          default_settings=[])
        result_sdk_api_level = config.get_value('sdk_api_level')
        assert result_sdk_api_level == 'FFFF'

        result_extra1 = config.get_value('extra1')
        assert result_extra1 == 'XXXX'


def test_read_config_value_and_source_with_override(tmpdir):

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

    with patch(target="o3de.command_utils.O3DEConfig.apply_default_global_settings",
               return_value=pathlib.Path(global_settings_file.realpath())), \
         patch(target="o3de.command_utils.O3DEConfig.resolve_project_settings_file",
               return_value=pathlib.Path(project_settings_file.realpath())):

        config = command_utils.O3DEConfig(project_name=project_name,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          default_settings=[])

        result_sdk_api_level, result_sdk_api_level_project = config.get_value_and_source('sdk_api_level')
        assert result_sdk_api_level == 'FFFF'
        assert result_sdk_api_level_project == project_name

        result_extra1, result_extra1_project = config.get_value_and_source('extra1')
        assert result_extra1 == 'XXXX'
        assert result_extra1_project == ''


def test_set_config_global(tmpdir):

    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    tmpdir.ensure('.o3de/o3de_manifest.json')
    global_settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    global_settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = ZZZZ
extra1 = XXXX
""")

    with patch(target="o3de.command_utils.O3DEConfig.apply_default_global_settings",
               return_value=pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())):

        config = command_utils.O3DEConfig(project_name=None,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          default_settings=[])

        config.set_config_value('extra1', 'ZZZZ')

        result_extra1 = config.get_value('extra1')
        assert result_extra1 == 'ZZZZ'


def test_set_config_project(tmpdir):

    tmpdir.ensure('.o3de', dir=True)

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

    with patch(target="o3de.command_utils.O3DEConfig.apply_default_global_settings",
               return_value=pathlib.Path(test_global_settings_file.realpath())), \
         patch(target="o3de.command_utils.O3DEConfig.resolve_project_settings_file",
               return_value=pathlib.Path(test_project_settings_file.realpath())):

        config = command_utils.O3DEConfig(project_name=project_name,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          default_settings=[])
        config.set_config_value('extra1','ZZZZ')

        project_config = configparser.ConfigParser()
        project_config.read(test_project_settings_file.realpath())
        project_config_section = project_config[settings_section_name]
        assert project_config_section.get('extra1') == 'ZZZZ'

        global_config = configparser.ConfigParser()
        global_config.read(test_global_settings_file.realpath())
        global_config_section = global_config[settings_section_name]
        assert global_config_section.get('extra1') == 'XXXX'


@pytest.mark.parametrize(
    "key_and_value, expected_key, expected_value", [
        pytest.param("argument=foo", "argument", "foo", id="Simple"),
        pytest.param("argument =foo", "argument", "foo", id="Simple with space 1"),
        pytest.param("argument= foo", "argument", "foo", id="Simple with space 2"),
        pytest.param("argument = foo", "argument", "foo", id="Simple with space 3"),
        pytest.param("argument='foo* and three.'", "argument", "foo* and three.", id="Double Quotes")
    ]
)
def test_set_config_key_value(tmpdir, key_and_value, expected_key, expected_value):

    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'

    tmpdir.ensure('.o3de/o3de_manifest.json')
    settings_file = tmpdir.join(f'.o3de/{settings_filename}')
    settings_file.write(f"""
[{settings_section_name}]
sdk_api_level = ZZZZ
extra1 = XXXX
""")

    global_settings_file = pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())
    with patch(target="o3de.command_utils.O3DEConfig.apply_default_global_settings",
               return_value=global_settings_file):
        config = command_utils.O3DEConfig(project_name=None,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          default_settings=[])
        config.set_config_value_from_expression(key_and_value)

        global_config = configparser.ConfigParser()
        global_config.read(global_settings_file)
        global_config_section = global_config[settings_section_name]
        assert global_config_section.get(expected_key) == expected_value


def test_set_config_key_invalid_config_value(tmpdir):

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

    with patch(target="o3de.command_utils.O3DEConfig.apply_default_global_settings",
               return_value=pathlib.Path(tmpdir.join(f'.o3de/{settings_filename}').realpath())):
        config = command_utils.O3DEConfig(project_name=None,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          default_settings=[])
        try:
            config.set_config_value_from_expression(f"argument=\"{bad_config_value}\"")
        except command_utils.O3DEConfigError as e:
            pass
        else:
            assert False, f"Expected invalid config entry of '{bad_config_value}' to fail"


def test_get_all_config_values(tmpdir):

    settings_filename = '.unit_test_settings'
    settings_section_name = 'testing'
    project_name = 'foo'

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

    with patch(target="o3de.command_utils.O3DEConfig.apply_default_global_settings",
               return_value=pathlib.Path(global_settings_file.realpath())), \
         patch(target="o3de.command_utils.O3DEConfig.resolve_project_settings_file",
               return_value=pathlib.Path(project_settings_file.realpath())):
        config = command_utils.O3DEConfig(project_name=project_name,
                                          settings_filename=settings_filename,
                                          settings_section_name=settings_section_name,
                                          default_settings=[])

        results = config.get_all_values()
        project_to_value_list = {}
        for key, value, source in results:
            project_to_value_list.setdefault(source or "global", []).append( (key, value) )
        assert len(project_to_value_list['global']) == 2  # Note: Not 3 since 'extra1' is overridden by the project
        assert len(project_to_value_list[project_name]) == 1
