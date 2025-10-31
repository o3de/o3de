"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT

Pytest test configuration file.

Example command to run these tests:

pytest cmake/Platform/Windows/Packaging/Tests \
    --capture=no \
    --log-file=C:/workspace/test_installer.log \
    --installer-uri=file:///C:/path/to/o3de_installer.exe \
    --install-root=C:/O3DE/0.0.0.0 \
    --project-path=C:/workspace/TestProject

Alternately, the installer-uri can be an s3 or web URL. For example:
--installer-uri=s3://bucketname/o3de_installer.exe
--installer-uri=https://o3de.binaries.org/o3de_installer.exe

"""
import pytest
import shutil
from pathlib import Path
from subprocess import TimeoutExpired
from o3de import manifest

@pytest.fixture(scope="session")
def test_installer_fixture(context):
    """ Installer executable succeeds. """
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
    """ Installer key binaries exist. """
    assert (context.engine_bin_path / filename).is_file(), f"{filename} not found in {context.engine_bin_path}"


@pytest.fixture(scope="session")
def test_o3de_registers_engine_fixture(test_installer_fixture, context):
    """ Engine is registered when o3de.exe is run. """
    try:
        result = context.run([str(context.engine_bin_path / 'o3de.exe')], timeout=15)

        # o3de.exe should not close with an error code 
        assert result.returncode == 0, f"o3de.exe failed with exit code {result.returncode}"
    except TimeoutExpired as e:
        # we expect to close the app ourselves
        pass

    # a valid engine.json exists
    engine_json_data = manifest.get_engine_json_data(engine_name=None, engine_path=context.install_root)
    assert engine_json_data, f"Failed to get engine.json data for engine in {context.install_root}"
    assert 'engine_name' in engine_json_data, "Engine.json does not contain engine_name key"

    # the engine is registered
    engine_name = engine_json_data['engine_name']
    engine_registered_path = manifest.get_registered(engine_name = engine_name)

    assert engine_registered_path, f"Failed to get registered engine path for {engine_name}"
    assert engine_registered_path.resolve() == context.install_root, f"{engine_name} is registered to {engine_registered_path} instead of {context.install_root}"

@pytest.fixture(scope="session")
def test_create_project_fixture(test_o3de_registers_engine_fixture, context):
    """ o3de.bat CLI creates a project. """
    o3de_path = context.install_root / 'scripts/o3de.bat'

    result = context.run([str(o3de_path),'create-project','--project-path', str(context.project_path)])
    assert result.returncode == 0, f"o3de.bat failed to create a project with exit code {result.returncode}"

    project_json_path = Path(context.project_path) / 'project.json'
    assert project_json_path.is_file(), f"No project.json found at {project_json_path}"


@pytest.fixture(scope="session")
def test_compile_project_fixture(test_create_project_fixture, context):
    """ Project can be configured and compiled. """
    project_name = Path(context.project_path).name
    cmake_path = next(context.cmake_runtime_path.glob('**/cmake.exe'))
    launcher_target = f"{project_name}.GameLauncher"

    # configure non-monolithic 
    result = context.run([str(cmake_path),'-B', str(context.project_build_path_profile), '-S', '.'], cwd=context.project_path)
    assert result.returncode == 0, 'Failed to configure the test project non-monolithic build'
    assert (context.project_build_path_profile / f'{project_name}.sln').is_file(), 'No project solution file was created'

    # build profile (non-monolithic)
    result = context.run([str(cmake_path),'--build', str(context.project_build_path_profile), '--target', launcher_target, 'Editor', '--config', 'profile','--','-m'], cwd=context.project_path)
    assert result.returncode == 0, 'Failed to build the test project profile non-monolithic  Launcher and Editor targets'
    assert (context.project_bin_path_profile / f'{launcher_target}.exe').is_file(), 'No test project binary was created'

    # configure monolithic 
    result = context.run([str(cmake_path),'-B', str(context.project_build_path_release), '-S', '.','-DLY_MONOLITHIC_GAME=1'], cwd=context.project_path)
    assert result.returncode == 0, 'Failed to configure the test project monolithic build'
    assert (context.project_build_path_release / f'{project_name}.sln').is_file(), 'No project solution file was created'

    # build release (monolithic)
    result = context.run([str(cmake_path),'--build', str(context.project_build_path_release), '--target', launcher_target, '--config', 'release','--','-m'], cwd=context.project_path)
    assert result.returncode == 0, 'Failed to build the test project monolithic release Launcher target'
    assert (context.project_bin_path_release / f'{launcher_target}.exe').is_file(), 'No test project binary was created'


@pytest.fixture(scope="session")
def test_run_asset_processor_batch_fixture(test_compile_project_fixture, context):
    """ AssetProcessorBatch can process assets before running Editor and Launcher tests which use them. """
    try:
        result = context.run([str(context.engine_bin_path / 'AssetProcessorBatch.exe'),f'--project-path="{context.project_path}"','/platform=pc'], timeout=30*60)
        assert result.returncode == 0, f"AssetProcessorBatch.exe failed with exit code {result.returncode}"
    except TimeoutExpired as e:
        pass


@pytest.fixture(scope="session")
def test_run_editor_fixture(test_run_asset_processor_batch_fixture, context):
    """ Editor can be run without crashing. """
    try:
        # run Editor.exe for 2 mins 
        cmd = [
            str(context.engine_bin_path / 'Editor.exe'),
            f'--project-path="{context.project_path}"',
            '--rhi=null',
            '--regset="/Amazon/Settings/EnableSourceControl=false"',
            '--regset="/Amazon/AWS/Preferences/AWSAttributionConsentShown=true"',
            '--regset="/Amazon/AWS/Preferences/AWSAttributionEnabled=false"',
            '--skipWelcomeScreenDialog=true',
            '+wait_seconds','10',
            '+quit'
        ]
        result = context.run(cmd, timeout=2*60)
        assert result.returncode == 0, f"Editor.exe failed with exit code {result.returncode}"
    except TimeoutExpired as e:
        pass

    editor_log = context.project_path / 'user/log/Editor.log'
    assert editor_log.is_file(), "Editor.log file not found"

    # expect to find "Engine initialized" in Editor log
    engine_initialized_message_found = False
    with editor_log.open('r') as f:
        engine_initialized_message_found = ('Engine initialized' in f.read())

    assert engine_initialized_message_found, "Engine initialized message not found in editor.log"


@pytest.fixture(scope="session")
def test_run_launcher_fixture(test_run_asset_processor_batch_fixture, context):
    """ Game launcher can be run without crashing. """
    project_name = Path(context.project_path).name
    launcher_filename = f"{project_name}.GameLauncher.exe"
    try:
        # run profile launcher for 2 mins 
        result = context.run([str(context.project_bin_path_profile / launcher_filename),'--rhi=null'], cwd=context.project_bin_path_profile, timeout=2*60)
        assert result.returncode == 0, f"{launcher_filename} failed with exit code {result.returncode}"
    except TimeoutExpired as e:
        # we expect to close the app on timeout ourselves
        # deferred console commands are disabled in launcher 
        pass

    game_log = context.project_path / 'user/log/Game.log'
    assert game_log.is_file(), "Game.log file not found"

    exception_found = False
    with game_log.open('r') as f:
        exception_found = ('Exception with exit code' in f.read())
    
    assert not exception_found, "Exception found in Game.log"


@pytest.fixture(scope="session")
def test_uninstall_fixture(test_run_launcher_fixture, test_run_editor_fixture, context):
    """ Uninstall succeeds and unregisters the engine. """
    assert context.installer_path.is_file(), f"Invalid installer path {context.installer_path}"

    engine_json_data = manifest.get_engine_json_data(engine_name=None, engine_path=context.install_root)
    assert engine_json_data, f"Failed to get engine.json data for engine in {context.install_root}"
    assert 'engine_name' in engine_json_data, "Engine.json does not contain the engine_name key"

    # when the installer is run with timeout of 30 minutes
    result = context.run([str(context.installer_path) , f"InstallFolder={context.install_root}", "/quiet","/uninstall"], timeout=30*60)

    # the installer succeeds
    assert result.returncode == 0, f"Installer failed with exit code {result.returncode}"

    # the engine is no longer registered
    engine_name = engine_json_data['engine_name']
    engine_registered_path = manifest.get_registered(engine_name = engine_name)
    assert not engine_registered_path, f"{engine_name} is still registered with the path {engine_registered_path}"

def test_installer(test_uninstall_fixture):
    """ PyTest needs one non-fixture test. Change the 'test_uninstall_fixture' param to the 
    specific fixture you want to test during development. 
    """
    assert True
