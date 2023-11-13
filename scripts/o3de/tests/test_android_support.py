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
