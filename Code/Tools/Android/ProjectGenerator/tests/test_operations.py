#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

import os
from pathlib import Path
import subprocess
import sys
import threading
from unittest.mock import Mock

import pytest


@pytest.fixture(autouse=True)
def generator_module_path(monkeypatch):
    monkeypatch.syspath_prepend(str(Path(__file__).resolve().parents[1]))


@pytest.mark.parametrize("returncode", [0, 1])
@pytest.mark.parametrize("output, encoding, expected", [
    ("UTF-8: caf\u00e9".encode("utf-8"), "ascii", "UTF-8: caf\u00e9"),
    (b"Generating 2\xa0048 bit RSA key pair", "cp1251", "Generating 2\u00a0048 bit RSA key pair"),
    (b"Invalid byte: \xff", "utf-8", "Invalid byte: \ufffd"),
])
def test_subprocess_output(monkeypatch, returncode, output, encoding, expected):
    from subprocess_runner import SubprocessRunner

    process = Mock(returncode=returncode)
    process.communicate.return_value = (output, output)
    monkeypatch.setattr(subprocess, "Popen", Mock(return_value=process))
    monkeypatch.setattr("locale.getpreferredencoding", lambda _: encoding)

    runner = SubprocessRunner(["keytool"], 10)
    assert runner.run() == (returncode == 0)
    assert runner.get_error_code() == returncode
    assert runner.get_stdout() == expected
    assert runner.get_stderr() == expected


def test_subprocess_missing_executable(tmp_path):
    from subprocess_runner import SubprocessRunner

    runner = SubprocessRunner([str(tmp_path / "missing-keytool")], 1)
    assert not runner.run()
    assert runner.get_error_code() == -1
    assert runner.get_stderr()


def test_subprocess_timeout_preserves_non_utf8_output(monkeypatch):
    from subprocess_runner import SubprocessRunner

    process = Mock()
    process.communicate.side_effect = [
        subprocess.TimeoutExpired("keytool", 1), (b"output\xa0", b"error\xa0")]
    monkeypatch.setattr(subprocess, "Popen", Mock(return_value=process))
    monkeypatch.setattr("locale.getpreferredencoding", lambda _: "cp1251")

    runner = SubprocessRunner(["keytool"], 1)
    assert not runner.run()
    process.kill.assert_called_once_with()
    assert runner.get_error_code() == -1
    assert runner.get_stdout() == "output\u00a0"
    assert runner.get_stderr() == "error\u00a0"


def test_subprocess_real_timeout():
    from subprocess_runner import SubprocessRunner

    runner = SubprocessRunner([sys.executable, "-c", "import time; time.sleep(30)"], 0.1)
    assert not runner.run()
    assert runner.get_error_code() == -1
    assert runner._subprocess.poll() is not None


def test_worker_exception_finishes_operation():
    from threaded_lambda import ThreadedLambda

    def fail():
        raise RuntimeError("worker failed")

    operation = ThreadedLambda("test", fail)
    operation.start()
    operation._thread.join(timeout=5)
    assert not operation._thread.is_alive()
    assert operation.is_finished()
    assert not operation.is_success()
    assert "RuntimeError: worker failed" in operation.get_report_msg()


def test_finished_waits_for_final_result_and_report():
    from threaded_lambda import ThreadedLambda

    finishing = threading.Event()
    release = threading.Event()

    def job():
        operation._is_finished = True
        finishing.set()
        if not release.wait(timeout=5):
            raise RuntimeError("Test did not release worker")
        operation._is_success = True
        operation._report_msg = "Complete report"

    operation = ThreadedLambda("test", job)
    assert not operation.is_finished()
    operation.start()
    try:
        assert finishing.wait(timeout=5)
        assert not operation.is_finished()
    finally:
        release.set()
        operation._thread.join(timeout=5)
    assert operation.is_finished()
    assert operation.is_success()
    assert operation.get_report_msg() == "Complete report"


@pytest.mark.parametrize("generator_name, command_count", [("keystore_generator", 5), ("project_generator", 6)])
@pytest.mark.parametrize("failure_index", [0, 2, -1])
def test_generators_stop_on_command_failure(monkeypatch, generator_name, command_count, failure_index):
    from config_data import ConfigData
    from keystore_generator import KeystoreGenerator
    from project_generator import ProjectGenerator

    generator = KeystoreGenerator if generator_name == "keystore_generator" else ProjectGenerator
    operation = generator(ConfigData())
    success = Mock(returncode=0)
    success.communicate.return_value = (b"", b"")
    failure = Mock(returncode=7)
    failure.communicate.return_value = (b"", b"command failed")
    processes = [success] * command_count
    processes[failure_index] = failure
    popen = Mock(side_effect=processes)
    monkeypatch.setattr(subprocess, "Popen", popen)

    operation.start()
    operation._thread.join(timeout=5)
    assert operation.is_finished()
    assert not operation.is_success()
    assert popen.call_count == (failure_index % command_count) + 1
    assert "Completed with status code 7" in operation.get_report_msg()
    assert "command failed" in operation.get_report_msg()
    assert "Next Steps" not in operation.get_report_msg()


@pytest.mark.parametrize("java_home", [None, "jdk with spaces"])
def test_keystore_selects_java_home_before_path(monkeypatch, tmp_path, java_home):
    from config_data import ConfigData
    from keystore_generator import KeystoreGenerator

    if java_home is None:
        monkeypatch.delenv("JAVA_HOME", raising=False)
        expected = "keytool"
    else:
        java_home = str(tmp_path / java_home)
        monkeypatch.setenv("JAVA_HOME", java_home)
        expected = str(Path(java_home) / "bin" / ("keytool.exe" if os.name == "nt" else "keytool"))
    operation = KeystoreGenerator(ConfigData())
    process = Mock(returncode=0)
    process.communicate.return_value = (b"", b"")
    popen = Mock(return_value=process)
    monkeypatch.setattr(subprocess, "Popen", popen)

    operation.start()
    operation._thread.join(timeout=5)
    assert operation.is_finished()
    assert operation.is_success()
    assert popen.call_count == 5
    assert popen.call_args.args[0][0] == expected


@pytest.mark.parametrize("failure", ["missing_keytool", "localized_output"])
def test_keystore_operation_finishes_after_keytool(monkeypatch, failure):
    from config_data import ConfigData
    from keystore_generator import KeystoreGenerator

    operation = KeystoreGenerator(ConfigData())
    configure = Mock(returncode=0)
    configure.communicate.return_value = (b"", b"")
    keytool = Mock(returncode=0)
    keytool.communicate.return_value = (b"", b"Generating 2\xa0048 bit RSA key pair")
    result = FileNotFoundError("keytool is missing") if failure == "missing_keytool" else keytool
    popen = Mock(side_effect=[configure] * 4 + [result])
    monkeypatch.setattr(subprocess, "Popen", popen)
    monkeypatch.setattr("locale.getpreferredencoding", lambda _: "cp1251")

    operation.start()
    operation._thread.join(timeout=5)
    assert not operation._thread.is_alive()
    assert operation.is_finished()
    assert operation.is_success() == (failure == "localized_output")
    if failure == "missing_keytool":
        assert "keytool is missing" in operation.get_report_msg()
    else:
        assert "Generating 2\u00a0048 bit RSA key pair" in operation.get_report_msg()
