#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import json
import os
import pytest
import platform
import subprocess
import sys

from packaging.version import Version

ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common
from cmake.Tools.Platform.Android import android_support, generate_android_project

@pytest.mark.parametrize(
    "from_override, version_str, expected_result", [
        pytest.param(False, b"Gradle 4.10.1", Version('4.10.1'), id='equalMinVersion'),
        pytest.param(False, b"Gradle 5.6.4", Version('5.6.4'), id='equalMaxVersion'),
        pytest.param(False, b"Gradle 1.0", common.LmbrCmdError('error', common.ERROR_CODE_ENVIRONMENT_ERROR), id='lessThanMinVersion'),
        pytest.param(False, b"Gradle 26.3", common.LmbrCmdError('error', common.ERROR_CODE_ENVIRONMENT_ERROR), id='greaterThanMaxVersion'),
        pytest.param(True, b"Gradle 4.10.1", Version('4.10.1')),
        pytest.param(True, b"Gradle 5.6.4", Version('5.6.4')),
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
        assert shell == (platform.system() == 'Windows')
        if from_override:
            assert args[0] == os.path.normpath(f'{override_gradle_install_path}/bin/{gradle_script}')
        assert args[1] == '-v'

        return version_str

    subprocess.check_output = _mock_check_output

    try:
        result_version, result_override_path = generate_android_project.verify_gradle(override_gradle_install_path)
        assert isinstance(expected_result, Version)
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
        pytest.param(False, f"cmake version {generate_android_project.CMAKE_MIN_VERSION}\nKit Ware", generate_android_project.CMAKE_MIN_VERSION, id='equalMinVersion'),
        pytest.param(False, "cmake version 4.0.0\nKit Ware", Version('4.0.0'), id='greaterThanMinVersion'),
        pytest.param(False, "cmake version 1.0.0\nKit Ware", common.LmbrCmdError('error', common.ERROR_CODE_ENVIRONMENT_ERROR), id='lessThanMinVersion'),
        pytest.param(True, f"cmake version {generate_android_project.CMAKE_MIN_VERSION}\nKit Ware", generate_android_project.CMAKE_MIN_VERSION, id='override_equalMinVersion'),
        pytest.param(True, "cmake version 4.0.0\nKit Ware", Version('4.0.0'), id='override_greaterThanMinVersion'),
        pytest.param(True, "cmake version 1.0.0\nKit Ware", common.LmbrCmdError('error', common.ERROR_CODE_ENVIRONMENT_ERROR), id='override_lessThanMinVersion'),
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

    def _mock_check_output(args, shell, stderr):
        assert args
        assert shell == (platform.system() == 'Windows')
        if from_override:
            assert args[0] == os.path.normpath(f'{override_cmake_install_path}/bin/{cmake_exe}')
        assert args[1] == '--version'

        return version_str.encode('utf-8', 'ignore')

    subprocess.check_output = _mock_check_output

    try:
        result_version, result_override_path = generate_android_project.verify_cmake(override_cmake_install_path)
        assert isinstance(expected_result, Version)
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
        pytest.param(False, b"1.0.0", Version('1.0.0')),
        pytest.param(False, b"1.10.0", Version('1.10.0')),
        pytest.param(True, b"1.0.0", Version('1.0.0')),
        pytest.param(True, b"1.10.0", Version('1.10.0'))
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

    def _mock_check_output(args, shell, stderr):
        assert args
        assert shell == (platform.system() == 'Windows')
        if from_override:
            assert args[0] == os.path.normpath(f'{override_cmake_install_path}/{ninja_exe}')
        assert args[1] == '--version'

        return version_str

    subprocess.check_output = _mock_check_output

    try:
        result_version, result_override_path = generate_android_project.verify_ninja(override_cmake_install_path)
        assert isinstance(expected_result, Version)
        assert result_version == expected_result
        if from_override:
            assert os.path.normpath(result_override_path) == os.path.normpath(os.path.join(override_cmake_install_path, ninja_exe))
        else:
            assert result_override_path is None

    except common.LmbrCmdError:
        assert isinstance(expected_result, common.LmbrCmdError)

    finally:
         subprocess.check_output = orig_check_output

