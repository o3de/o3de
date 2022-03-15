"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Pytest test configuration file.

Example method to run these tests:

pytest cmake/Platform/Windows/Packaging/Tests -s \
    --log-file=C:/workspace/test_installer.log \
    --installer-uri=file:///C:/path/to/o3de_installer.exe \
    --install-root=C:/O3DE/0.0.0.0 \
    --project-path=C:/workspace/TestProject

Alternately, the installer-uri can be an s3 or web URL. For example:
--installer-uri=s3://bucketname/o3de_installer.exe
--installer-uri=https://o3de.binaries.org/o3de_installer.exe

"""
import pytest
import json
import os
from pathlib import Path
from subprocess import TimeoutExpired

@pytest.fixture(scope="session")
def test_installer_fixture(context):
    """Installer executable succeeds"""
    assert context.installer_path.is_file(), f"Invalid installer path {context.installer_path}"

    # when the installer is run with timeout of 30 minutes
    result = context.run([str(context.installer_path) , f"InstallFolder={context.install_root}", "/quiet"], timeout=30*60)

    # the installer succeeds
    assert result.returncode == 0, f"Installer failed with exit code {result.returncode}"

    # the install root is created
    assert context.install_root.is_dir(), f"Invalid install root {context.install_root}"

@pytest.mark.parametrize(
    "filename", [
        pytest.param('Editor.exe'),
        pytest.param('AssetProcessor.exe'),
        pytest.param('MaterialEditor.exe'),
        pytest.param('o3de.exe'),
    ]
)
def test_binaries_exist(test_installer_fixture, context, filename):
    """Installer key binaries exist"""
    assert (context.engine_bin_path / filename).is_file(), f"{filename} not found in {context.engine_bin_path}"

@pytest.fixture(scope="session")
def test_o3de_registers_engine_fixture(test_installer_fixture, context):
    print(f"Begin running register engine test")
    """Engine is registered when o3de.exe is run"""
    try:
        result = context.run([str(context.engine_bin_path / 'o3de.exe')], timeout=15)

        # o3de.exe should not close with an error code 
        assert result.returncode == 0, f"o3de.exe failed with exit code {result.returncode}"
        print(f"o3de.exe quit with return code {result.returncode}")
    except TimeoutExpired as e:
        # we expect to close the app ourselves
        print("o3de.exe was closed after exceeding the timeout")
        pass

    # the engine is registered
    engine_json_path = context.install_root / 'engine.json'
    with engine_json_path.open('r') as f:
        engine_json_data = json.load(f)

    engine_name = engine_json_data['engine_name']

    manifest_path = Path(os.path.expanduser("~")).resolve() / '.o3de' / 'o3de_manifest.json'
    with manifest_path.open('r') as f:
        manifest_json_data = json.load(f)
    
    engine_path = Path(context.install_root).as_posix()
    assert engine_path in manifest_json_data['engines'] , f"Engine path {engine_path} not found in o3de_manifest.json"
    assert engine_name in manifest_json_data['engines_path'], f"{engine_path} not found in manifest engines_path"
    assert manifest_json_data['engines_path'][engine_name] == engine_path, f"Engines path has invalid entry for {engine_name} expected {engine_path}" 
    print(f"\nManifest {manifest_path}.")
    print(f"Finished running register engine test for engine {engine_name} at path {engine_path}.")

@pytest.fixture(scope="session")
def test_create_project_fixture(test_o3de_registers_engine_fixture, context):
    """o3de CLI creates a project"""
    o3de_path = context.install_root / 'scripts' / 'o3de.bat'
    print(f"Begin running create project test using {o3de_path}")
    result = context.run([str(o3de_path),'create-project','--project-path', str(context.project_path)])
    assert result.returncode == 0, f"o3de.bat failed to create a project with exit code {result.returncode}"

    project_json_path = Path(context.project_path) / 'project.json'
    assert project_json_path.is_file(), f"No project.json found at {project_json_path}"
    print(f"End running create project test for project with json at {project_json_path}")

    assert False, "Stopping for debug purposes"

@pytest.fixture(scope="session")
def test_compile_project_fixture(test_create_project_fixture, context):
    """Project can be configured and compiled"""
    project_name = Path(context.project_path).name
    cmake_path = next(context.cmake_runtime_path.glob('**/cmake.exe'))
    launcher_target = f"{project_name}.GameLauncher"

    # configure
    result = context.run([str(cmake_path),'-B', str(context.project_build_path), '-S', '.', '-G', 'Visual Studio 16 2019'], cwd=context.project_path)
    assert result.returncode == 0
    assert (context.project_build_path / f'{project_name}.sln').is_file()

    # build profile
    result = context.run([str(cmake_path),'--build', str(context.project_build_path), '--target', launcher_target, 'Editor', '--config', 'profile','--','-m'], cwd=context.project_path)
    assert result.returncode == 0
    assert (context.project_bin_path / f'{launcher_target}.exe').is_file()

@pytest.fixture(scope="session")
def test_run_asset_processor_batch_fixture(test_compile_project_fixture, context):
    """Test asset batch processing before running Editor and Launcher tests which need assets"""
    try:
        result = context.run([str(context.engine_bin_path / 'AssetProcessorBatch.exe'),f'--project-path="{context.project_path}"','/platform=pc'], timeout=30*60)
        assert result.returncode == 0, f"AssetProcessorBatch.exe failed with exit code {result.returncode}"
    except TimeoutExpired as e:
        pass

@pytest.fixture(scope="session")
def test_run_editor_fixture(test_run_asset_processor_batch_fixture, context):
    """Editor can be run without crashing"""
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
    aws_attribution_path = context.project_path / 'user' / 'Registry' / 'editor_aws_preferences.setreg'
    with aws_attribution_path.open('w') as f:
        json.dump(aws_attribution_regset, f) 
    
    try:
        # run Editor.exe for 2 mins 
        result = context.run([str(context.engine_bin_path / 'Editor.exe'),f'--project-path="{context.project_path}"','--rhi=null','--skipWelcomeScreenDialog=True','+wait_seconds','10','+quit'], timeout=2*60)
        assert result.returncode == 0, f"Editor.exe failed with exit code {result.returncode}"
    except TimeoutExpired as e:
        pass

    editor_log = context.project_path / 'user'/ 'log'/ 'Editor.log'
    assert editor_log.is_file(), "Editor.log file not found"

    # expect to find "Engine initialized" in Editor log
    engine_initialized_message_found = False
    with editor_log.open('r') as f:
        engine_initialized_message_found = ('Engine initialized' in f.read())

    assert engine_initialized_message_found, "Engine initialized message not found in editor.log"

@pytest.fixture(scope="session")
def test_run_launcher_fixture(test_run_asset_processor_batch_fixture, context):
    """Game launcher can be run without crashing"""
    project_name = Path(context.project_path).name
    launcher_filename = f"{project_name}.GameLauncher.exe"
    try:
        # run launcher for 2 mins 
        result = context.run([str(context.project_bin_path / launcher_filename),'--rhi=null'], cwd=context.project_bin_path, timeout=2*60)
        assert result.returncode == 0, f"{launcher_filename} failed with exit code {result.returncode}"
    except TimeoutExpired as e:
        # we expect to close the app on timeout ourselves
        # deferred console commands are disabled in launcher 
        pass

    game_log = context.project_path / 'user'/ 'log'/ 'Game.log'
    assert game_log.is_file(), "Game.log file not found"

    exception_found = False
    with game_log.open('r') as f:
        exception_found = ('Exception with exit code' in f.read())
    
    assert not exception_found, "Exception found in Game.log"

def test_uninstall(test_run_launcher_fixture, test_run_editor_fixture, context):
    """Uninstall succeeds and unregisters the engine"""
    assert context.installer_path.is_file(), f"Invalid installer path {context.installer_path}"

    engine_json_path = context.install_root / 'engine.json'
    with engine_json_path.open('r') as f:
        engine_json_data = json.load(f)

    engine_name = engine_json_data['engine_name']

    # when the installer is run with timeout of 30 minutes
    result = context.run([str(context.installer_path) , f"InstallFolder={context.install_root}", "/quiet","/uninstall"], timeout=30*60)

    # the installer succeeds
    assert result.returncode == 0, f"Installer failed with exit code {result.returncode}"

    manifest_path = Path(os.path.expanduser("~")).resolve() / '.o3de' / 'o3de_manifest.json'
    with manifest_path.open('r') as f:
        manifest_json_data = json.load(f)
    
    engine_path = Path(context.install_root).as_posix()
    assert engine_path not in manifest_json_data['engines'] , f"Engine path {engine_path} still exists in o3de_manifest.json"
    assert engine_name not in manifest_json_data['engines_path'], f"{engine_path} still exists in manifest engines_path"


# Convenience functions for running test fixtures from VS Code Test Explorer
@pytest.mark.skip(reason="For Test Explorer use only")
def test_installer(test_installer_fixture):
    assert True

@pytest.mark.skip(reason="For Test Explorer use only")
def test_o3de_registers_engine(test_o3de_registers_engine_fixture):
    assert True

@pytest.mark.skip(reason="For Test Explorer use only")
def test_create_project(test_create_project_fixture):
    assert True

@pytest.mark.skip(reason="For Test Explorer use only")
def test_compile_project(test_compile_project_fixture):
    assert True

@pytest.mark.skip(reason="For Test Explorer use only")
def test_run_asset_processor_batch(test_run_asset_processor_batch_fixture):
    assert True

@pytest.mark.skip(reason="For Test Explorer use only")
def test_run_editor(test_run_editor_fixture):
    assert True

@pytest.mark.skip(reason="For Test Explorer use only")
def test_run_launcher(test_run_launcher_fixture):
    assert True