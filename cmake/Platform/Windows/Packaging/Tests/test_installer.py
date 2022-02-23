"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Pytest test configuration file.

"""
import pytest
from pathlib import Path
from subprocess import run, TimeoutExpired
from time import sleep
import json
import os
import shutil

@pytest.fixture(scope="session")
def installer_path(request):
    return Path(request.config.getoption("--installer-path")).resolve()

@pytest.fixture(scope="session")
def install_root(request):
    return Path(request.config.getoption("--install-root")).resolve()

@pytest.fixture(scope="session")
def project_path(request):
    # MSBuild doesn't want us to output in the temp directory MSB8029
    # so we use a pre-defined folder outside of %TMP%
    #return tmpdir_factory.mktemp("TestProject")
    path = Path(request.config.getoption("--project-path")).resolve()
    yield path

    # wait a few seconds for processes to quit
    sleep(5)

    shutil.rmtree(path)

@pytest.fixture(scope="session")
def cmake_path(install_root):
    return install_root / 'cmake' / 'runtime' / 'cmake-3.20.2-windows-x86_64' / 'bin' / 'cmake.exe'

@pytest.fixture(scope="session")
def test_installer(installer_path, install_root):
    assert installer_path.is_file(), f"Invalid installer path {installer_path}"

    # when the installer is run with timeout of 30 minutes
    result = run([installer_path , f"InstallFolder={install_root}", "/quiet"], timeout=30*60)

    # the installer succeeds
    assert result.returncode == 0, f"Installer failed with exit code {result.returncode}"

    # the install root is created
    assert install_root.is_dir(), f"Invalid install root {install_root}"

@pytest.mark.parametrize(
    "filename", [
        pytest.param('Editor.exe'),
        pytest.param('AssetProcessor.exe'),
        pytest.param('MaterialEditor.exe'),
        pytest.param('o3de.exe'),
    ]
)
def test_binaries_exist(test_installer, install_root, filename):
    bin_folder = install_root / 'bin/Windows/profile/Default'
    assert (bin_folder / filename).is_file(), f"{filename} not found in {bin_folder}"


@pytest.fixture(scope="session")
def test_o3de_registers_engine_fixture(install_root, test_installer):
    try:
        bin_folder = install_root / 'bin' / 'Windows' / 'profile' / 'Default'
        result = run([str(bin_folder / 'o3de.exe')], timeout=7)

        # o3de.exe should not close with an error code 
        assert result.returncode == 0, f"o3de.exe failed with exit code {result.returncode}"
    except TimeoutExpired as e:
        # we expect to close the app ourselves
        pass

    # the engine is registered
    engine_json_path = Path(install_root).resolve() / 'engine.json'
    with engine_json_path.open('r') as f:
        engine_json_data = json.load(f)

    engine_name = engine_json_data['engine_name']

    manifest_path = Path(os.path.expanduser("~")).resolve() / '.o3de' / 'o3de_manifest.json'
    with manifest_path.open('r') as f:
        manifest_json_data = json.load(f)
    
    engine_path = Path(install_root).as_posix()
    assert engine_path in manifest_json_data['engines'] , f"Engine path {engine_path} not found in o3de_manifest.json"
    assert engine_name in manifest_json_data['engines_path'], f"{engine_path} not found in manifest engines_path"
    assert manifest_json_data['engines_path'][engine_name] == engine_path, f"Engines path has invalid entry for {engine_name} expected {engine_path}" 

def test_o3de_registers_engine(test_o3de_registers_engine_fixture):
    """Utility test to run the fixture test from Visual Code Test Explorer"""
    assert True

@pytest.fixture(scope="session")
def test_create_project_fixture(install_root, project_path, test_o3de_registers_engine_fixture):
    o3de_path = install_root / 'scripts' / 'o3de.bat'
    result = run([str(o3de_path),'create-project','--project-path', str(project_path)])
    assert result.returncode == 0, f"o3de.bat failed to create a project with exit code {result.returncode}"

    project_json_path = Path(project_path) / 'project.json'
    assert project_json_path.is_file(), f"No project.json found at {project_json_path}"

def test_create_project(test_create_project_fixture):
    """Utility test to run the fixture test from Visual Code Test Explorer"""
    assert True

@pytest.fixture(scope="session")
def test_compile_project_fixture(install_root, project_path, test_create_project_fixture, cmake_path):
    """Project can be configured and compiled"""
    build_folder = project_path / 'build' / 'windows_vs2019'
    project_name = Path(project_path).name
    launcher_target = f"{project_name}.GameLauncher"

    # configure
    result = run([str(cmake_path),'-B', str(build_folder), '-S', '.', '-G', 'Visual Studio 16 2019'], cwd=project_path, capture_output=True)
    assert result.returncode == 0
    assert (build_folder / f'{project_name}.sln').is_file()

    # build profile
    result = run([str(cmake_path),'--build', str(build_folder), '--target', launcher_target, 'Editor', '--config', 'profile','--','-m'], cwd=project_path)
    assert result.returncode == 0
    assert (build_folder / 'bin' / 'profile' / f'{launcher_target}.exe').is_file()

    # build release not yet supported in pre-built SDK
    #result = run([str(cmake_path),'--build', str(build_folder), '--target', launcher_target, '--config', 'release','--','-m'], cwd=project_path)
    #assert result.returncode == 0
    #assert (build_folder / 'bin' / 'release' / f'{launcher_target}.exe').is_file()

def test_compile_project(test_compile_project_fixture):
    """Utility test to run the fixture test from Visual Code Test Explorer"""
    assert True

@pytest.fixture(scope="session")
def test_run_asset_processor_batch_fixture(install_root, project_path, test_compile_project_fixture):
    """Test asset batch processing before running Editor and Launcher tests which need assets"""
    bin_folder = install_root /'bin/Windows/profile/Default'
    try:
        result = run([str(bin_folder / 'AssetProcessorBatch.exe'),f'--project-path="{project_path}"','/platform=pc'], timeout=30*60)
        assert result.returncode == 0, f"AssetProcessorBatch.exe failed with exit code {result.returncode}"
    except TimeoutExpired as e:
        pass

def test_run_asset_processor_batch(test_run_asset_processor_batch_fixture):
    """Utility test to run the fixture test from Visual Code Test Explorer"""
    assert True

@pytest.fixture(scope="session")
def test_run_editor_fixture(install_root, project_path, test_run_asset_processor_batch_fixture):
    """Editor can be run without crashing"""
    bin_folder = install_root /'bin/Windows/profile/Default'

    # write out the attribution shown to avoid the popup stalling editor load
    aws_attribution_regset = {
            "Amazon": {
                "AWS": {
                    "Preferences": {
                        "AWSAttributionConsentShown": True,
                        "AWSAttributionEnabled": False 
                    }
                }
            }
        }
    aws_attribution_path = Path(project_path).resolve() / 'user' / 'Registry' / 'editor_aws_preferences.setreg'
    with aws_attribution_path.open('w') as f:
        json.dump(aws_attribution_regset, f) 
    
    try:
        # run Editor.exe for 2 mins 
        result = run([str(bin_folder / 'Editor.exe'),f'--project-path="{project_path}"','--rhi=null','--skipWelcomeScreenDialog=True','+wait_seconds','10','+quit'], timeout=2*60)
        assert result.returncode == 0, f"Editor.exe failed with exit code {result.returncode}"
    except TimeoutExpired as e:
        pass

    editor_log = Path(project_path).resolve() / 'user'/ 'log'/ 'Editor.log'
    assert editor_log.is_file(), "Editor.log file not found"

    # expect to find "Engine initialized" in Editor log
    engine_initialized_message_found = False
    with editor_log.open('r') as f:
        engine_initialized_message_found = ('Engine initialized' in f.read())

    assert engine_initialized_message_found, "Engine initialized message not found in editor.log"

def test_run_editor(test_run_editor_fixture):
    """Utility test to run the fixture test from Visual Code Test Explorer"""
    assert True

@pytest.fixture(scope="session")
def test_run_launcher_fixture(install_root, project_path, test_run_asset_processor_batch_fixture):
    """Game launcher can be run without crashing"""
    bin_folder = project_path / 'build' / 'windows_vs2019' / 'bin' / 'profile'
    project_name = Path(project_path).name
    launcher_filename = f"{project_name}.GameLauncher.exe"
    try:
        # run launcher for 2 mins 
        result = run([str(launcher_filename),'--rhi=null'], cwd=bin_folder, timeout=2*60)
        assert result.returncode == 0, f"{launcher_filename} failed with exit code {result.returncode}"
    except TimeoutExpired as e:
        # we expect to close the app on timeout ourselves
        # deferred console commands are disabled in launcher 
        pass

    game_log = Path(project_path).resolve() / 'user'/ 'log'/ 'Game.log'
    assert game_log.is_file(), "Game.log file not found"

    exception_found = False
    with game_log.open('r') as f:
        exception_found = ('Exception with exit code' in f.read())
    
    assert not exception_found, "Exception found in Game.log"

def test_run_launcher(test_run_launcher_fixture):
    """Utility test to run the fixture test from Visual Code Test Explorer"""
    assert True

def test_uninstall(installer_path, install_root, test_run_launcher_fixture, test_run_editor_fixture):

    assert installer_path.is_file(), f"Invalid installer path {installer_path}"

    engine_json_path = Path(install_root).resolve() / 'engine.json'
    with engine_json_path.open('r') as f:
        engine_json_data = json.load(f)

    engine_name = engine_json_data['engine_name']

    # when the installer is run with timeout of 30 minutes
    result = run([installer_path , f"InstallFolder={install_root}", "/quiet","/uninstall"], timeout=30*60)

    # the installer succeeds
    assert result.returncode == 0, f"Installer failed with exit code {result.returncode}"

    # the install root is created
    assert install_root.is_dir(), f"Invalid install root {install_root}"

    manifest_path = Path(os.path.expanduser("~")).resolve() / '.o3de' / 'o3de_manifest.json'
    with manifest_path.open('r') as f:
        manifest_json_data = json.load(f)
    
    engine_path = Path(install_root).as_posix()
    assert engine_path not in manifest_json_data['engines'] , f"Engine path {engine_path} still exists in o3de_manifest.json"
    assert engine_name not in manifest_json_data['engines_path'], f"{engine_path} still exists in manifest engines_path"
