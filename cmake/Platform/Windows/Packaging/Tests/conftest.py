"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Pytest test configuration file.

"""
import pytest
from pathlib import Path, PurePath
import shutil
import os
import io
import threading
import urllib.parse
import urllib.request
from tempfile import NamedTemporaryFile, mkdtemp
from time import sleep
from subprocess import run
import boto3

def pytest_addoption(parser):
    parser.addoption("--installer-uri", action="store", default="")
    parser.addoption("--install-root", action="store", default=PurePath('C:/O3DE/0.0.0.0'))
    parser.addoption("--project-path", action="store", default=PurePath('C:/Workspace/TestProject'))

class SessionContext:
    """Holder for session constants and helper functions"""
    def __init__(self, request):
        # setup to capture subprocess output
        self.log_file = request.config.getoption("--log-file")
        self.temp_file = NamedTemporaryFile(mode='w+t', delete=False)

        # download the installer if necessary
        self.installer_uri = request.config.getoption("--installer-uri")
        parsed_uri = urllib.parse.urlparse(self.installer_uri)
        if parsed_uri.scheme in ['http', 'https', 'ftp', 'ftps','s3']:
            tmp_dir = mkdtemp()
            self.installer_path = Path(tmp_dir) / 'installer.exe'

            if parsed_uri.scheme == 's3':
                session = boto3.session.Session()
                client = session.client('s3')
                client.download_file(parsed_uri.netloc, parsed_uri.path.lstrip('/'), str(self.installer_path))
            else:
                urllib.request.urlretrieve(parsed_uri.geturl(), self.installer_path)

                try:
                    # verify with SignTool if available
                    signtool_path_result = run(['powershell','-Command',"(Resolve-Path \"C:\\Program Files (x86)\\Windows Kits\\10\\bin\\*\\x64\" | Select-Object -Last 1).Path"], text=True, capture_output=True)
                    if signtool_path_result.returncode == 0:
                        signtool_path = Path(signtool_path_result.stdout.strip()) / 'signtool.exe'
                        signtool_result = self.run([str(signtool_path),'verify', '/pa', str(self.installer_path)])
                        assert signtool_result.returncode == 0
                except FileNotFoundError as e:
                    pass
        else:
            # strip the leading slash from the file URI 
            self.installer_path = Path(parsed_uri.path.lstrip('/')).resolve()

        self.install_root = Path(request.config.getoption("--install-root")).resolve()
        self.project_path = Path(request.config.getoption("--project-path")).resolve()
        self.engine_bin_path = self.install_root / 'bin' / 'Windows' / 'profile' / 'Default'
        self.project_build_path = self.project_path / 'build' / 'Windows'
        self.project_bin_path = self.project_build_path / 'bin' / 'profile'

        self.cmake_runtime_path = self.install_root / 'cmake' / 'runtime'

        # start a log reader thread to print the output to screen
        self.log_reader = io.open(self.temp_file.name, 'r', buffering=1)
        self.log_reader_thread = threading.Thread(target=self._tail_log, daemon=True)
        self.log_reader_thread.start()
        self.log_reader_shutdown = False
    
    def _tail_log(self):
        while True:
            line = self.log_reader.readline()
            if line == '':
                if self.log_reader_shutdown:
                    return
            else:
                print(line, end='')

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
        self.log_reader_shutdown = True
        self.log_reader_thread.join()
        self.log_reader.close()
        if self.log_file:
            shutil.copy(self.temp_file.name, Path(self.log_file).resolve())
        os.remove(self.temp_file.name)

@pytest.fixture(scope="session")
def context(request):
    session_context = SessionContext(request)
    yield session_context
    session_context.cleanup()
