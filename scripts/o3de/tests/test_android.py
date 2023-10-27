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

import pytest
import pathlib
from unittest.mock import patch

from o3de import android

@pytest.mark.parametrize(
    "test_path, expect_error", [
        pytest.param("my_project", False, id="From same level: success"),
        pytest.param("my_project/Gem", False, id="From one level up: Success"),
        pytest.param("my_project/Gem/Source", False, id="From two levels up: Success"),
        pytest.param("not_a_project/foo", True, id="No project.json found: Error"),
    ]
)
def test_resolve_project_path_from_cwd(tmpdir, test_path, expect_error):

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
    with patch('o3de.manifest.get_all_projects', return_value=[tmpdir.join('my_project').realpath()]) as mock_get_all_projects:
        try:
            resolved_name, resolved_path = android.resolve_project_path(tmpdir.join(test_path).realpath())
        except android.AndroidToolError:
            assert expect_error
        else:
            assert not expect_error

def test_resolve_android_settings_file_global(tmpdir):

    expected_settings_file = tmpdir.join(f'.o3de/{android.ANDROID_SETTINGS_FILE}').realpath()
    with patch('o3de.manifest.get_o3de_folder', return_value=pathlib.Path(tmpdir.join('.o3de').realpath())) as mock_get_o3de_folder:
        android_settings_file = android.resolve_android_global_settings_file()
        assert android_settings_file == expected_settings_file


@pytest.mark.parametrize(
    "project_name", [
        pytest.param("MyProject", id="with project name"),
        pytest.param(None, id="without project name")
    ]
)
def test_resolve_android_settings_file_by_project(tmpdir, project_name):
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

    expected_settings_file = tmpdir.join(f'my_project/{android.ANDROID_SETTINGS_FILE}').realpath()
    with patch('o3de.manifest.get_registered', return_value=pathlib.Path(tmpdir.join('my_project').realpath())) as mock_get_registered, \
         patch('o3de.android.resolve_project_path', return_value=('MyProject',pathlib.Path(tmpdir.join('my_project').realpath()))) as mock_resolve_project_path:
        android_settings_file = android.resolve_android_project_settings_file(project_name)
        assert android_settings_file == expected_settings_file

def test_apply_default_global_android_settings_no_initial_settings(tmpdir):
    tmpdir.ensure('.o3de/o3de_manifest.json')

    android_global_settings_file_path = pathlib.Path(tmpdir.join(f'.o3de/{android.ANDROID_SETTINGS_FILE}').realpath())

    with patch('o3de.manifest.get_o3de_folder', return_value=pathlib.Path(tmpdir.join('.o3de').realpath())) as mock_get_o3de_folder,\
         patch('o3de.android.resolve_android_global_settings_file', return_value=android_global_settings_file_path) as mock_get_o3de_folder:
        android.apply_default_global_android_settings()

        assert android_global_settings_file_path.is_file()

        global_config = configparser.ConfigParser()
        global_config.read(android_global_settings_file_path.absolute())
        global_section = global_config[android.ANDROID_SETTINGS_GLOBAL_SECTION]

        for config_key, default_value in android.DEFAULT_ANDROID_SETTINGS:
            assert global_section[config_key] == default_value

def test_apply_default_global_android_settings_some_initial_settings(tmpdir):
    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_settings_file = tmpdir.join(f'.o3de/{android.ANDROID_SETTINGS_FILE}')
    android_settings_file.write(f"""
    [{android.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)

    android_global_settings_file_path = pathlib.Path(tmpdir.join(f'.o3de/{android.ANDROID_SETTINGS_FILE}').realpath())

    with patch('o3de.manifest.get_o3de_folder', return_value=pathlib.Path(tmpdir.join('.o3de').realpath())) as mock_get_o3de_folder,\
         patch('o3de.android.resolve_android_global_settings_file', return_value=android_global_settings_file_path) as mock_get_o3de_folder:
        android.apply_default_global_android_settings()

        assert android_global_settings_file_path.is_file()

        global_config = configparser.ConfigParser()
        global_config.read(android_global_settings_file_path.absolute())
        global_section = global_config[android.ANDROID_SETTINGS_GLOBAL_SECTION]

        assert global_section['sdk_api_level'] == 'ZZZZ'
        assert global_section['extra1'] == 'XXXX'

def test_read_android_config_global_only(tmpdir):
    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_settings_file = tmpdir.join(f'.o3de/{android.ANDROID_SETTINGS_FILE}')
    android_settings_file.write(f"""
    [{android.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)

    global_settings_file = pathlib.Path(tmpdir.join(f'.o3de/{android.ANDROID_SETTINGS_FILE}').realpath())
    result = android.read_android_config(base_settings_file=global_settings_file)

    assert result.get('sdk_api_level',None) == 'ZZZZ'
    assert result.get('extra1', None) == 'XXXX'



def test_read_android_config(tmpdir):
    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_global_settings_file = tmpdir.join(f'.o3de/{android.ANDROID_SETTINGS_FILE}')
    android_global_settings_file.write(f"""
    [{android.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)

    tmpdir.ensure('project/project.json')
    android_project_settings_file = tmpdir.join(f'project/{android.ANDROID_SETTINGS_FILE}')
    android_project_settings_file.write(f"""
    [{android.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = FFFF
    """)

    global_settings_file = pathlib.Path(android_global_settings_file.realpath())
    project_settings_file = pathlib.Path(android_project_settings_file.realpath())

    result = android.read_android_config(base_settings_file=global_settings_file,
                                         override_settings_file=project_settings_file)

    assert result.get('sdk_api_level',None) == 'FFFF'
    assert result.get('extra1', None) == 'XXXX'


def test_set_android_config_global(tmpdir):
    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_global_settings_file = tmpdir.join(f'.o3de/{android.ANDROID_SETTINGS_FILE}')
    android_global_settings_file.write(f"""
    [{android.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)

    android.set_android_config('extra1', 'ZZZZ', global_settings_file=pathlib.Path(android_global_settings_file.realpath()))

    global_config = configparser.ConfigParser()
    global_config.read(android_global_settings_file.realpath())
    global_config_section = global_config[android.ANDROID_SETTINGS_GLOBAL_SECTION]

    assert global_config_section.get('extra1') == 'ZZZZ'

def test_set_android_config_project(tmpdir):

    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_global_settings_file = tmpdir.join(f'.o3de/{android.ANDROID_SETTINGS_FILE}')
    android_global_settings_file.write(f"""
    [{android.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)

    tmpdir.ensure('project/project.json')
    android_project_settings_file = tmpdir.join(f'project/{android.ANDROID_SETTINGS_FILE}')
    android_project_settings_file.write(f"""
    [{android.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = FFFF
    """)

    android.set_android_config('extra1', 'ZZZZ', global_settings_file=pathlib.Path(android_global_settings_file.realpath()),
                                                 project_settings_file=pathlib.Path(android_project_settings_file.realpath()))

    project_config = configparser.ConfigParser()
    project_config.read(android_project_settings_file.realpath())
    project_config_section = project_config[android.ANDROID_SETTINGS_GLOBAL_SECTION]
    assert project_config_section.get('extra1') == 'ZZZZ'

    global_config = configparser.ConfigParser()
    global_config.read(android_global_settings_file.realpath())
    global_config_section = global_config[android.ANDROID_SETTINGS_GLOBAL_SECTION]
    assert global_config_section.get('extra1') == 'XXXX'

@pytest.mark.parametrize(
    "key_and_value, expected_key, expected_value", [
        pytest.param("argument=foo", "argument", "foo", id="Simple"),
        pytest.param("argument =foo", "argument", "foo", id="Simple with space 1"),
        pytest.param("argument= foo", "argument", "foo", id="Simple with space 2"),
        pytest.param("argument = foo", "argument", "foo", id="Simple with space 3"),
        pytest.param("argument=\"foo? and two%\"", "argument", "foo? and two%", id="Double Quotes"),
        pytest.param("argument='foo* and three.'", "argument", "foo* and three.", id="Double Quotes")
    ]
)
def test_set_android_config_key_value(key_and_value, expected_key, expected_value):
    dummy_global_config='dummy_global_config'
    dummy_project_config='dummy_project_config'
    def _mock_set_config(key:str,
                         value:str,
                         global_settings_file:str,
                         project_settings_file:str = None)->None:
        assert key == expected_key
        assert value == expected_value
        assert global_settings_file == dummy_global_config
        assert project_settings_file == dummy_project_config

    with patch('o3de.android.set_android_config', new=_mock_set_config) as mock_set_config:
        android.set_android_config_arg(key_and_value,dummy_global_config,dummy_project_config)

