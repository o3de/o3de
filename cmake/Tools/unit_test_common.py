#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import configparser
import hashlib
import json
import os
import pathlib
import pytest
import re

from . import common


@pytest.mark.parametrize(
    "engine_json_content, expected_success", [
        pytest.param({'fake': 'foo'}, True,     id="TestSuccess"),
        pytest.param(None,            False,    id="TestFail")
    ])
def test_determine_engine_root(tmpdir, engine_json_content, expected_success):
    
    test_folder_heirarchy = 'dev/foo1/foo2/foo3/'
    tmpdir.ensure(test_folder_heirarchy)
    if engine_json_content:
        fake_engine_json_content = json.dumps(engine_json_content,
                                              sort_keys=True,
                                              separators=(',', ': '),
                                              indent=4)
        engine_json_file = tmpdir.join('dev/engine.json')
        engine_json_file.write(fake_engine_json_content)
        expected_path = str(tmpdir.join('dev/').realpath())
    else:
        expected_path = None

    starting_path = str(tmpdir.join(test_folder_heirarchy).realpath())
    result = common.determine_engine_root(starting_path)

    if expected_path:
        assert os.path.normcase(result) == os.path.normcase(expected_path)
    else:
        assert result is None


TEST_AP_CONFIG_1 = """
[Platforms]
;pc=enabled
;ios=enabled
"""

TEST_AP_CONFIG_2 = """
[Platforms]
;pc=enabled
ios=enabled
"""

TEST_AP_CONFIG_3 = """
[Platforms]
pc=disabled
ios=enabled
"""


@pytest.mark.parametrize(
    "contents, check_asset_type, expected_result", [
        pytest.param(TEST_AP_CONFIG_1, 'ios', False, id='IosDisabled'),
        pytest.param(TEST_AP_CONFIG_2, 'ios', True, id='IosEnabled'),
        pytest.param(TEST_AP_CONFIG_3, 'ios', True, id='IosEnabled'),
        pytest.param(TEST_AP_CONFIG_1, 'pc', True, id='PCEnabled'),
        pytest.param(TEST_AP_CONFIG_3, 'pc', False, id='PCDisabled'),
    ]
)
def test_validate_ap_config_asset_type_enabled(tmpdir, contents, check_asset_type, expected_result):
    test_dev_root = 'dev'
    tmpdir.ensure('{}/AssetProcessorPlatformConfig.ini'.format(test_dev_root))
    ap_config_file = tmpdir.join('{}/AssetProcessorPlatformConfig.ini'.format(test_dev_root))
    ap_config_file.write(contents)
    
    ap_config_file_path = str(tmpdir.join(test_dev_root).realpath())
    
    result = common.validate_ap_config_asset_type_enabled(ap_config_file_path, check_asset_type)
    
    assert expected_result == result


@pytest.mark.parametrize(
    "filename, file_mtime, file_size, contents, deep_check", [
        pytest.param('alpha.txt', 1000, 1000, "Alpha Alpha Alpha", False, id="TestShallow"),
        pytest.param('alpha.txt', 1000, 1000, "Alpha Alpha Alpha", True, id="TestDeepMatch"),
        pytest.param('beta.txt', 1000, 1000, "Beta Beta Beta", False, id="TestShallowMatch2"),
        pytest.param('beta.txt', 1001, 1000, "Beta Beta Beta", False, id="TestShallowMatch3"),
        pytest.param('ceti.txt', 2000, 2000, "Ceti Ceti Ceti", True, id="TestDeepMatch3"),
    ]
)
def test_file_fingerprint_success(tmpdir, filename, file_mtime, file_size, contents, deep_check):
    
    backup_stat = os.stat

    tmpdir.ensure(filename)
    ap_config_file = tmpdir.join(filename)
    ap_config_file.write(contents)
    full_path = str(tmpdir.join(filename).realpath())

    try:
        
        class MockStatResult(object):
            def __init__(self):
                self.st_mtime = file_mtime
                self.st_size = file_size
                
        def _mock_stat(path):
            assert path == full_path
            return MockStatResult()
    
        os.stat = _mock_stat
        
        expected_hasher = hashlib.md5()
        expected_hasher.update(str(file_mtime).encode('UTF-8'))
        expected_hasher.update(str(file_size).encode('UTF-8'))

        # If doing a deep check, also include the contents
        if deep_check:
            expected_hasher.update(contents.encode('UTF-8'))
        expected_result = expected_hasher.hexdigest()

        result = common.file_fingerprint(full_path, deep_check)
        
        assert result == expected_result

    finally:
        os.stat = backup_stat


def test_load_template_file_success(tmpdir):
    tmpdir.ensure('test_template.txt.in')
    test_template_content = """
### Copyright will be removed from template
[test]
subjectA = ${subject_A_value}
subjectB = ${subject_B_value}
"""
    expected_a = 'foo'
    expected_b = 'bar'
    test_template_file = tmpdir / 'test_template.txt.in'
    test_template_file.write_text(test_template_content, encoding='ascii')

    test_template_env = {
        'subject_A_value': expected_a,
        'subject_B_value': expected_b
    }

    result = common.load_template_file(pathlib.Path(str(test_template_file.realpath())), test_template_env)

    assert '###' not in result
    validator = configparser.ConfigParser()
    validator.read_string(result)
    validate_subjA = validator.get('test', 'subjectA')
    validate_subjB = validator.get('test', 'subjectB')
    assert validate_subjA == expected_a
    assert validate_subjB == expected_b



TEST_GAME_PROJECT_JSON_FORMAT = """
{{
    "project_name": "{project_name}",
    "product_name": "{project_name}",
    "executable_name": "{project_name}.GameLauncher",
    "modules" : [],
    "project_id": "{{4F3363D3-4A7C-47A6-B464-B21524771358}}",

    "android_settings" : {{
        "package_name" : "com.lumberyard.yourgame",
        "version_number" : 1,
        "version_name" : "1.0.0.0",
        "orientation" : "landscape"
    }}
}}
"""


def test_verify_game_project_and_dev_root_success(tmpdir):

    dev_root = 'dev'
    game_name = 'MyFoo'
    game_folder = 'myfoo'
    game_project_json = TEST_GAME_PROJECT_JSON_FORMAT.format(project_name=game_name)
    tmpdir.ensure(f'{dev_root}/{game_folder}/project.json')
    project_json_path = tmpdir / dev_root / game_folder / 'project.json'
    project_json_path.write_text(game_project_json, encoding='ascii')

    result_game_name, _ = common.verify_game_project_and_dev_root(game_folder,
                                                                  str(tmpdir.join(dev_root).realpath()))
    assert result_game_name == game_name



def test_platform_last_settings_success(tmpdir):

    tmpdir.ensure('platform.list')

    test_build_dir = "c:/test/path"
    test_projects_str = "ProjA;ProjB"
    test_projects = test_projects_str.split(';')
    test_asset_deploy_mode = "LOOSE"
    test_asset_deploy_type = "pc"
    test_platform = 'foo'

    last_settings_content = f"""
# Auto Generated from last cmake project generation (2020-07-24T12:10:47)
[settings]
platform={test_platform}
game_projects={test_projects_str}
asset_deploy_mode={test_asset_deploy_mode}
asset_deploy_type={test_asset_deploy_type}
"""
    test_last_file = tmpdir / 'platform.settings'
    test_last_file.write_text(last_settings_content, encoding='ascii')

    result = common.PlatformSettings(tmpdir.realpath())

    assert result.projects == test_projects
    assert result.asset_deploy_mode == test_asset_deploy_mode
    assert result.asset_deploy_type == test_asset_deploy_type


def test_cmake_dependency_success(tmpdir):

    test_module = 'FooBar'

    tmpdir.ensure(f'Registry/cmake_dependencies.{test_module.lower()}.setreg')
    test_setreg_path = tmpdir / 'Registry' / f'cmake_dependencies.{test_module.lower()}.setreg'

    test_module_1 = "Gem.Maestro.Editor.3b9a978ed6f742a1acb99f74379a342c.v0.1.0.dll"
    test_module_2 = "Gem.TextureAtlas.5a149b6b3c964064bd4970f0e92f72e2.v0.1.0.dll"

    test_retreg_content = f"""
{{
    "Amazon":
    {{
        "Gems":
        {{
            "Maestro.Editor":
            {{
                "Module":"{test_module_1}",
                "SourcePaths":["Gems/Maestro"]
            }},
            "TextureAtlas":
            {{
                "Module":"{test_module_2}",
                "SourcePaths":["Gems/TextureAtlas"]
            }}
        }}
    }}
}}
    """
    test_setreg_path.write_text(test_retreg_content, encoding='ascii')

    result = common.get_cmake_dependency_modules(tmpdir, test_module, 'Gems')

    assert result
    assert test_module_1 in result
    assert test_module_2 in result

