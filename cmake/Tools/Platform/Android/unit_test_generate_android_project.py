#
#   All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
#   its licensors.
#
#   For complete copyright and license terms please see the LICENSE at the root of this
#   distribution (the "License"). All use of this software is governed by the License,
#   or, if provided, by the license below or the license accompanying this file. Do not
#   remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
import json
import os
import pytest
import platform
import subprocess
import sys

from distutils.version import LooseVersion

ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common
from cmake.Tools.Platform.Android import android_support, generate_android_project

@pytest.mark.parametrize(
    "from_override, version_str, expected_result", [
        pytest.param(False, b"Gradle 4.10.1", LooseVersion('4.10.1'), id='equalMinVersion'),
        pytest.param(False, b"Gradle 5.6.4", LooseVersion('5.6.4'), id='eualMaxVersion'),
        pytest.param(False, b"Gradle 1.0", common.LmbrCmdError('error', common.ERROR_CODE_ENVIRONMENT_ERROR), id='lessThanMinVersion'),
        pytest.param(False, b"Gradle 26.3", common.LmbrCmdError('error', common.ERROR_CODE_ENVIRONMENT_ERROR), id='greaterThanMaxVersion'),
        pytest.param(True, b"Gradle 4.10.1", LooseVersion('4.10.1')),
        pytest.param(True, b"Gradle 5.6.4", LooseVersion('5.6.4')),
        pytest.param(True, b"Gradle 1.0", common.LmbrCmdError('error', common.ERROR_CODE_ENVIRONMENT_ERROR)),
        pytest.param(True, b"Gradle 26.3", common.LmbrCmdError('error', common.ERROR_CODE_ENVIRONMENT_ERROR))
    ]
)
def test_verify_gradle(tmpdir, from_override, version_str, expected_result):

    orig_check_output = subprocess.check_output

    if from_override:
        gradle_script = 'gradle.bat' if platform.system() == 'Windows' else 'gradle'
        tmpdir.ensure(f'gradle/bin/{gradle_script}')
        override_gradle_install_path = str(tmpdir.join('gradle').realpath())
    else:
        override_gradle_install_path = None

    def _mock_check_output(args, shell):
        assert args
        assert shell is True
        if from_override:
            assert args[0] == os.path.normpath(f'{override_gradle_install_path}/bin/{gradle_script}')
        assert args[1] == '-v'

        return version_str

    subprocess.check_output = _mock_check_output

    try:
        result_version, result_override_path = generate_android_project.verify_gradle(override_gradle_install_path)
        assert isinstance(expected_result, LooseVersion)
        assert result_version == expected_result
        if from_override:
            assert os.path.normpath(result_override_path) == os.path.normpath(os.path.join(override_gradle_install_path, 'bin', gradle_script))
        else:
            assert result_override_path is None

    except common.LmbrCmdError:
        assert isinstance(expected_result, common.LmbrCmdError)

    except Exception as e:
        pass

    finally:
         subprocess.check_output = orig_check_output


@pytest.mark.parametrize(
    "from_override, version_str, expected_result", [
        pytest.param(False, b"cmake version 3.17.0\nKit Ware", LooseVersion('3.17.0'), id='equalMinVersion'),
        pytest.param(False, b"cmake version 4.0.0\nKit Ware", LooseVersion('4.0.0'), id='greaterThanMinVersion'),
        pytest.param(False, b"cmake version 1.0.0\nKit Ware", common.LmbrCmdError('error', common.ERROR_CODE_ENVIRONMENT_ERROR), id='lessThanMinVersion'),
        pytest.param(True, b"cmake version 3.17.0\nKit Ware", LooseVersion('3.17.0'), id='override_equalMinVersion'),
        pytest.param(True, b"cmake version 4.0.0\nKit Ware", LooseVersion('4.0.0'), id='override_greaterThanMinVersion'),
        pytest.param(True, b"cmake version 1.0.0\nKit Ware", common.LmbrCmdError('error', common.ERROR_CODE_ENVIRONMENT_ERROR), id='override_lessThanMinVersion'),
    ]
)
def test_verify_cmake(tmpdir, from_override, version_str, expected_result):

    orig_check_output = subprocess.check_output

    if from_override:
        cmake_exe = 'cmake.exe' if platform.system() == 'Windows' else 'cmake'
        tmpdir.ensure(f'cmake/bin/{cmake_exe}')
        override_cmake_install_path = str(tmpdir.join('cmake').realpath())
    else:
        override_cmake_install_path = None

    def _mock_check_output(args, shell):
        assert args
        assert shell is True
        if from_override:
            assert args[0] == os.path.normpath(f'{override_cmake_install_path}/bin/{cmake_exe}')
        assert args[1] == '--version'

        return version_str

    subprocess.check_output = _mock_check_output

    try:
        result_version, result_override_path = generate_android_project.verify_cmake(override_cmake_install_path)
        assert isinstance(expected_result, LooseVersion)
        assert result_version == expected_result
        if from_override:
            assert os.path.normpath(result_override_path) == os.path.normpath(os.path.join(override_cmake_install_path, 'bin', cmake_exe))
        else:
            assert result_override_path is None

    except common.LmbrCmdError:
        assert isinstance(expected_result, common.LmbrCmdError)

    finally:
         subprocess.check_output = orig_check_output


@pytest.mark.parametrize(
    "from_override, version_str, expected_result", [
        pytest.param(False, b"1.0.0", LooseVersion('1.0.0')),
        pytest.param(False, b"1.10.0", LooseVersion('1.10.0')),
        pytest.param(True, b"1.0.0", LooseVersion('1.0.0')),
        pytest.param(True, b"1.10.0", LooseVersion('1.10.0'))
    ]
)
def test_verify_ninja(tmpdir, from_override, version_str, expected_result):

    orig_check_output = subprocess.check_output

    if from_override:
        ninja_exe = 'ninja.exe' if platform.system() == 'Windows' else 'ninja'
        tmpdir.ensure(f'ninja/{ninja_exe}')
        override_cmake_install_path = str(tmpdir.join('ninja').realpath())
    else:
        override_cmake_install_path = None

    def _mock_check_output(args, shell):
        assert args
        assert shell is True
        if from_override:
            assert args[0] == os.path.normpath(f'{override_cmake_install_path}/{ninja_exe}')
        assert args[1] == '--version'

        return version_str

    subprocess.check_output = _mock_check_output

    try:
        result_version, result_override_path = generate_android_project.verify_ninja(override_cmake_install_path)
        assert isinstance(expected_result, LooseVersion)
        assert result_version == expected_result
        if from_override:
            assert os.path.normpath(result_override_path) == os.path.normpath(os.path.join(override_cmake_install_path, ninja_exe))
        else:
            assert result_override_path is None

    except common.LmbrCmdError:
        assert isinstance(expected_result, common.LmbrCmdError)

    finally:
         subprocess.check_output = orig_check_output


TEST_VALIDATE_VERSION_MIN = 19
TEST_VALIDATE_VERSION_MAX = 21


@pytest.mark.parametrize(
    "test_input, expected", [
        pytest.param('20', 20),
        pytest.param('android-20', 20),
        pytest.param('bad-21', "android-'XX'"),
        pytest.param('10', "minimum"),
        pytest.param('30', "maximum")
    ]
)
def test_validate_android_platform_input(test_input, expected):
    try:
        result = android_support.validate_android_platform_input(input_android_platform=test_input,
                                                                 platform_variable_type='test',
                                                                 min_version=TEST_VALIDATE_VERSION_MIN,
                                                                 max_version=TEST_VALIDATE_VERSION_MAX)
        assert isinstance(expected, int)
        assert result == expected
    except Exception as e:
        assert expected in str(e)


def test_verify_android_sdk_success(tmpdir):

    test_android_path = 'android_sdk'
    sdk_version_number = 28
    sdk_version = f'android-{sdk_version_number}'

    tmpdir.ensure(f'{test_android_path}/platforms/{sdk_version}/package.xml')

    tmpdir.ensure(f'{test_android_path}/build-tools/28.0.3/package.xml')
    tmpdir.ensure(f'{test_android_path}/build-tools/29.0.3/package.xml')

    input_sdk_path = tmpdir.join(test_android_path).realpath()
    argument_name = '--android-sdk'

    requested_build_tool_version = '29.0.3'

    result_sdk_version, result_sdk_path, result_build_tool_version = android_support.verify_android_sdk(android_sdk_platform=sdk_version,
                                                                                                        argument_name=argument_name,
                                                                                                        override_android_sdk_path=input_sdk_path,
                                                                                                        preferred_sdk_build_tools_ver=requested_build_tool_version)
    assert result_sdk_version == sdk_version_number
    assert result_sdk_path == input_sdk_path
    assert result_build_tool_version == requested_build_tool_version

    sdk_version_number_only = str(sdk_version_number)
    result_sdk_version, result_sdk_path, result_build_tool_version = android_support.verify_android_sdk(android_sdk_platform=sdk_version_number_only,
                                                                                                        argument_name=argument_name,
                                                                                                        override_android_sdk_path=input_sdk_path)
    assert result_sdk_version == sdk_version_number
    assert result_sdk_path == input_sdk_path
    assert result_build_tool_version == '28.0.3'

    requested_build_tool_version = '30.0.3'
    result_sdk_version, result_sdk_path, result_build_tool_version = android_support.verify_android_sdk(android_sdk_platform=sdk_version,
                                                                                                        argument_name=argument_name,
                                                                                                        override_android_sdk_path=input_sdk_path,
                                                                                                        preferred_sdk_build_tools_ver=requested_build_tool_version)
    assert result_sdk_version == sdk_version_number
    assert result_sdk_path == input_sdk_path
    assert result_build_tool_version == '28.0.3'


@pytest.mark.parametrize(
    "desired_ndk_version_number, available_ndk_revisions, pkg_revision, mappings, expect_error", [
        pytest.param(21, [21, 22, 24], '15.2.4203891', None, False, id='preNdk19ExactMatch'),
        pytest.param(23, [21, 22, 24], '15.2.4203891', None, False, id='preNdk19FallbackMatch'),
        pytest.param(22, [21, 22, 24], '19.2.4203891', {'23': 21}, False, id='postNdk19ExactMatch'),
        pytest.param(23, [21, 22, 24], '21.2.4203891', {'23': 21}, False, id='postNdk19MappingMatch'),
        pytest.param(android_support.ANDROID_NDK_MIN_PLATFORM-1, [21, 22, 24], '15.2.4203891', None, True, id='preNdk19BelowMinVer'),
        pytest.param(android_support.ANDROID_NDK_MAX_PLATFORM+1, [21, 22, 24], '15.2.4203891', None, True, id='preNdk19AboveMaxVer'),
        pytest.param(25, [21, 22, 24], '19.2.4203891', {'23': 21}, True, id='postNdk19NoMatch')
    ]
)
def test_verify_android_ndk_success(tmpdir, desired_ndk_version_number, available_ndk_revisions, pkg_revision, mappings, expect_error):

    test_android_path = 'android_ndk'
    for ndk_number in available_ndk_revisions:
        tmpdir.ensure(f'{test_android_path}/platforms/android-{ndk_number}/arch-arm64/usr/lib/libc.so')

    tmpdir.ensure(f'{test_android_path}/source.properties')
    test_ndk_source_properties_file = tmpdir / test_android_path / 'source.properties'
    test_ndk_source_properties_file.write_text(f'Pkg.Desc = Android NDK\nPkg.Revision = {pkg_revision}\n', encoding='ASCII')

    if mappings:
        platform_mapping = {
            # min and max are arbitrary for now since we dont use it during evaluation, but if we do, parameterize it here as well
            "min": 16,  #
            "max": 29,
            "aliases": {}
        }
        for key, value in mappings.items():
            platform_mapping['aliases'][key] = value
        tmpdir.ensure(f'{test_android_path}/meta/platforms.json')
        platform_mapping_file = tmpdir / test_android_path / 'meta/platforms.json'
        platform_mapping_file.write_text(json.dumps(platform_mapping), encoding='ASCII')

    input_ndk_path = tmpdir.join(test_android_path).realpath()

    try:
        android_ndk_platform_number, android_ndk_path = android_support.verify_android_ndk(android_ndk_platform=str(desired_ndk_version_number),
                                                                                           argument_name="--android-ndk",
                                                                                           override_android_ndk_path=input_ndk_path)
        assert not expect_error
        assert android_ndk_platform_number == desired_ndk_version_number
        assert android_ndk_path == input_ndk_path
    except Exception:
        assert expect_error

