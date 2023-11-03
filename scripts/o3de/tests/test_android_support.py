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
        pytest.param("7.4.1", "7.5"),
        pytest.param("7.1.1", "7.2"),
    ]
)
def test_get_android_gradle_plugin_requirements(agp_version, expected_gradle_version):
    result = android_support.get_android_gradle_plugin_requirements(requested_agp_version=agp_version)
    assert result.gradle_ver == Version(expected_gradle_version)


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
            resolved_name, resolved_path = android_support.resolve_project_name_and_path(tmpdir.join(test_path).realpath())
        except android_support.AndroidToolError:
            assert expect_error
        else:
            assert not expect_error


def test_resolve_android_settings_file_global(tmpdir):

    expected_settings_file = tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}').realpath()
    with patch('o3de.manifest.get_o3de_folder', return_value=pathlib.Path(tmpdir.join('.o3de').realpath())) as mock_get_o3de_folder:
        android_settings_file = android_support.AndroidConfig.resolve_android_global_settings_file()
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

    expected_settings_file = tmpdir.join(f'my_project/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}').realpath()
    with patch('o3de.manifest.get_registered', return_value=pathlib.Path(tmpdir.join('my_project').realpath())) as mock_get_registered, \
         patch('o3de.android_support.resolve_project_name_and_path', return_value=('MyProject',pathlib.Path(tmpdir.join('my_project').realpath()))) as mock_resolve_project_path:
        android_settings_file = android_support.AndroidConfig.resolve_android_project_settings_file(project_name)
        assert android_settings_file == expected_settings_file


def test_apply_default_global_android_settings_no_initial_settings(tmpdir):
    tmpdir.ensure('.o3de/o3de_manifest.json')

    android_global_settings_file_path = pathlib.Path(tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}').realpath())

    with patch('o3de.manifest.get_o3de_folder', return_value=pathlib.Path(tmpdir.join('.o3de').realpath())) as mock_get_o3de_folder,\
         patch('o3de.android_support.AndroidConfig.resolve_android_global_settings_file', return_value=android_global_settings_file_path) as mock_get_o3de_folder:
        android_support.AndroidConfig.apply_default_global_android_settings()

        assert android_global_settings_file_path.is_file()

        global_config = configparser.ConfigParser()
        global_config.read(android_global_settings_file_path.absolute())
        global_section = global_config[android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION]

        for config_key, default_value in android_support.DEFAULT_ANDROID_SETTINGS:
            assert global_section[config_key] == default_value


def test_apply_default_global_android_settings_some_initial_settings(tmpdir):
    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_settings_file = tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}')
    android_settings_file.write(f"""
    [{android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)

    android_global_settings_file_path = pathlib.Path(tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}').realpath())

    with patch('o3de.manifest.get_o3de_folder', return_value=pathlib.Path(tmpdir.join('.o3de').realpath())) as mock_get_o3de_folder,\
         patch('o3de.android_support.AndroidConfig.resolve_android_global_settings_file', return_value=android_global_settings_file_path) as mock_get_o3de_folder:

        android_support.AndroidConfig.apply_default_global_android_settings()

        assert android_global_settings_file_path.is_file()

        global_config = configparser.ConfigParser()
        global_config.read(android_global_settings_file_path.absolute())
        global_section = global_config[android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION]

        assert global_section['sdk_api_level'] == 'ZZZZ'
        assert global_section['extra1'] == 'XXXX'


def test_read_android_config_global_only(tmpdir):

    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_settings_file = tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}')
    android_settings_file.write(f"""
    [{android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)

    global_settings_file = pathlib.Path(tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}').realpath())
    with patch("o3de.android_support.AndroidConfig.apply_default_global_android_settings", return_value=global_settings_file):
        android_config = android_support.AndroidConfig(project_name=None, is_global=True)
        result_sdk_api_level,_ = android_config.get_value('sdk_api_level')
        result_extra1, _ = android_config.get_value('extra1')

        assert result_sdk_api_level == 'ZZZZ'
        assert result_extra1 == 'XXXX'


def test_read_android_config_with_override(tmpdir):
    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_global_settings_file = tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}')
    android_global_settings_file.write(f"""
    [{android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)

    tmpdir.ensure('project/project.json')
    android_project_settings_file = tmpdir.join(f'project/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}')
    android_project_settings_file.write(f"""
    [{android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = FFFF
    """)

    global_settings_file = pathlib.Path(android_global_settings_file.realpath())
    project_settings_file = pathlib.Path(android_project_settings_file.realpath())

    with patch("o3de.android_support.AndroidConfig.apply_default_global_android_settings", return_value=global_settings_file), \
         patch("o3de.android_support.AndroidConfig.resolve_android_project_settings_file", return_value=project_settings_file):
        android_config = android_support.AndroidConfig(project_name='project', is_global=False)
        result_sdk_api_level, _ = android_config.get_value('sdk_api_level')
        result_extra1, _ = android_config.get_value('extra1')

        assert result_sdk_api_level == 'FFFF'
        assert result_extra1 == 'XXXX'


def test_set_android_config_global(tmpdir):
    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_global_settings_file = tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}')
    android_global_settings_file.write(f"""
    [{android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)

    global_settings_file = pathlib.Path(tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}').realpath())
    with patch("o3de.android_support.AndroidConfig.apply_default_global_android_settings", return_value=global_settings_file):
        android_config = android_support.AndroidConfig(project_name=None, is_global=True)
        android_config.set_config_value('extra1', 'ZZZZ')

        result_extra1, _ = android_config.get_value('extra1')

        assert result_extra1 == 'ZZZZ'



def test_set_android_config_project(tmpdir):

    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_global_settings_file = tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}')
    android_global_settings_file.write(f"""
    [{android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)

    tmpdir.ensure('project/project.json')
    android_project_settings_file = tmpdir.join(f'project/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}')
    android_project_settings_file.write(f"""
    [{android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = FFFF
    """)

    global_settings_file = pathlib.Path(android_global_settings_file.realpath())
    project_settings_file = pathlib.Path(android_project_settings_file.realpath())

    with patch("o3de.android_support.AndroidConfig.apply_default_global_android_settings", return_value=global_settings_file), \
         patch("o3de.android_support.AndroidConfig.resolve_android_project_settings_file", return_value=project_settings_file):

        android_config = android_support.AndroidConfig(project_name='project', is_global=False)
        android_config.set_config_value('extra1','ZZZZ')

        project_config = configparser.ConfigParser()
        project_config.read(android_project_settings_file.realpath())
        project_config_section = project_config[android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION]
        assert project_config_section.get('extra1') == 'ZZZZ'

        global_config = configparser.ConfigParser()
        global_config.read(android_global_settings_file.realpath())
        global_config_section = global_config[android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION]
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
def test_set_android_config_key_value(tmpdir, key_and_value, expected_key, expected_value):

    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_settings_file = tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}')
    android_settings_file.write(f"""
    [{android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)

    global_settings_file = pathlib.Path(tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}').realpath())
    with patch("o3de.android_support.AndroidConfig.apply_default_global_android_settings", return_value=global_settings_file):
        android_config = android_support.AndroidConfig(project_name=None, is_global=True)
        android_config.set_config_value_from_expression(key_and_value)

        global_config = configparser.ConfigParser()
        global_config.read(global_settings_file)
        global_config_section = global_config[android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION]
        assert global_config_section.get(expected_key) == expected_value


def test_set_android_config_key_invalid_config_value(tmpdir):

    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_settings_file = tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}')
    android_settings_file.write(f"""
    [{android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    extra1 = XXXX
    """)
    bad_config_value="foo? and two%"

    global_settings_file = pathlib.Path(tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}').realpath())
    with patch("o3de.android_support.AndroidConfig.apply_default_global_android_settings", return_value=global_settings_file):
        android_config = android_support.AndroidConfig(project_name=None, is_global=True)
        try:
            android_config.set_config_value_from_expression(f"argument=\"{bad_config_value}\"")
        except android_support.AndroidToolError as e:
            pass
        else:
            assert False, f"Expected invalid config entry of '{bad_config_value}' to fail"


def test_get_all_config_values(tmpdir):

    tmpdir.ensure('.o3de/o3de_manifest.json')
    android_global_settings_file = tmpdir.join(f'.o3de/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}')
    android_global_settings_file.write(f"""
    [{android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = ZZZZ
    ndk = 22.*
    extra1 = XXXX
    """)

    tmpdir.ensure('project/project.json')
    android_project_settings_file = tmpdir.join(f'project/{android_support.AndroidConfig.ANDROID_SETTINGS_FILE}')
    android_project_settings_file.write(f"""
    [{android_support.AndroidConfig.ANDROID_SETTINGS_GLOBAL_SECTION}]
    sdk_api_level = FFFF
    """)

    global_settings_file = pathlib.Path(android_global_settings_file.realpath())
    project_settings_file = pathlib.Path(android_project_settings_file.realpath())

    with patch("o3de.android_support.AndroidConfig.apply_default_global_android_settings", return_value=global_settings_file), \
         patch("o3de.android_support.AndroidConfig.resolve_android_project_settings_file", return_value=project_settings_file):
        android_config = android_support.AndroidConfig(project_name='project', is_global=False)

        results = android_config.get_all_values()
        project_to_value_list = {}
        for key, value, source in results:
            project_to_value_list.setdefault(source or "global", []).append( (key, value) )
        assert len(project_to_value_list['global']) == 2  # Note: Not 3 since 'extra1' is overridden by the project
        assert len(project_to_value_list['project']) == 1


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
