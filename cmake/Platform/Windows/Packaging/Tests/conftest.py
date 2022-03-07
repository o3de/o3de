"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Pytest test configuration file.

"""
import pytest
import pathlib
import shutil
import os
from tempfile import NamedTemporaryFile
from time import sleep
from subprocess import run

def pytest_addoption(parser):
    parser.addoption("--installer-path", action="store", default=pathlib.Path().resolve())
    parser.addoption("--install-root", action="store", default=pathlib.PurePath('C:/O3DE/0.0.0.0'))
    parser.addoption("--project-path", action="store", default=pathlib.PurePath('C:/Workspace/TestProject'))

class SessionContext:
    """Holder for session constants and helper functions"""
    def __init__(self, request):
        self.installer_path = pathlib.Path(request.config.getoption("--installer-path")).resolve()
        self.install_root = pathlib.Path(request.config.getoption("--install-root")).resolve()
        self.project_path = pathlib.Path(request.config.getoption("--project-path")).resolve()
        self.engine_bin_path = self.install_root / 'bin' / 'Windows' / 'profile' / 'Default'
        self.project_build_path = self.project_path / 'build' / 'Windows'
        self.project_bin_path = self.project_build_path / 'bin' / 'profile'

        cmake_runtime_path = self.install_root / 'cmake' / 'runtime'
        self.cmake_path = next(cmake_runtime_path.glob('**/cmake.exe'))

        self.log_file = request.config.getoption("--log-file")
        self.temp_file = NamedTemporaryFile(mode='w+t', delete=False)

    def run(self, command, timeout=None, cwd=None):
        self.temp_file.write(' '.join(command) + '\n')
        self.temp_file.flush()
        return run(command, timeout=timeout, cwd=cwd, stdout=self.temp_file, stderr=self.temp_file, text=True)

    def cleanup(self):
        if self.project_path.is_dir():
            # wait a few seconds for processes to stop using the project folder 
            sleep(5)

            o3de_path = self.install_root / 'scripts' / 'o3de.bat'
            if o3de_path.is_file():
                self.run([str(o3de_path),'register','--project-path', str(self.project_path), '--remove'])

            shutil.rmtree(self.project_path)

        self.temp_file.close()
        if self.log_file:
            shutil.copy(self.temp_file.name, pathlib.Path(self.log_file).resolve())
        os.remove(self.temp_file.name)

@pytest.fixture(scope="session")
def context(request):
    session_context = SessionContext(request)
    yield session_context
    session_context.cleanup()
