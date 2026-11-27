#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import os
import pathlib
import shutil
import subprocess
import sys

import pytest


@pytest.mark.skipif(sys.platform != "win32", reason="get_python.bat requires Windows cmd.exe")
@pytest.mark.parametrize(
    "requirements_exit_code, editable_exit_code, expected_exit_code, expected_message",
    [
        (0, 0, 0, "calling PIP to O3DE"),
        (1, 0, 1, "Failed to install the packages listed in"),
        (0, 1, 1, "into python"),
    ],
)
def test_get_python_handles_parentheses_in_install_path(
    tmp_path, requirements_exit_code, editable_exit_code, expected_exit_code, expected_message
):
    # Simulate the installed engine with a directory like "Program Files (x86)".
    python_dir = tmp_path / "Program Files (x86)" / "O3DE" / "python"
    python_dir.mkdir(parents=True)
    script_source = pathlib.Path(__file__).resolve().parents[3] / "python" / "get_python.bat"
    script_path = python_dir / "get_python.bat"
    shutil.copyfile(script_source, script_path)

    # Bypass downloads and dependency installation while exercising the real
    # batch parser and the success/requirements-failure/editable-failure paths.
    (python_dir / "python.cmd").write_bytes(
        b'@echo off\r\n'
        b'if /I "%~1"=="--version" exit /b 0\r\n'
        b'if /I "%~4"=="-r" exit /b %O3DE_TEST_REQUIREMENTS_EXIT%\r\n'
        b'if /I "%~4"=="-e" exit /b %O3DE_TEST_EDITABLE_EXIT%\r\n'
        b'exit /b 99\r\n'
    )

    environment = os.environ.copy()
    environment["O3DE_TEST_REQUIREMENTS_EXIT"] = str(requirements_exit_code)
    environment["O3DE_TEST_EDITABLE_EXIT"] = str(editable_exit_code)
    result = subprocess.run(
        ["cmd.exe", "/d", "/c", "call", str(script_path)],
        cwd=tmp_path,
        env=environment,
        capture_output=True,
        text=True,
        timeout=30,
    )
    output = result.stdout + result.stderr
    assert result.returncode == expected_exit_code, output
    assert expected_message in output
    assert "was unexpected at this time" not in output
