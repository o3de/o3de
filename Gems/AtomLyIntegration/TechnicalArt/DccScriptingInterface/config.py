# # coding:utf-8
# #!/usr/bin/python
# #
# # All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# # its licensors.
# #
# # For complete copyright and license terms please see the LICENSE at the root of this
# # distribution (the "License"). All use of this software is governed by the License,
# # or, if provided, by the license below or the license accompanying this file. Do not
# # remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# #
# # -- This line is 75 characters -------------------------------------------
#

"""! @brief This module is the main server for handling communications/commands between the DCCsi and Maya """

##
# @file config.py
#
# @brief This serves as the main entrypoint to the DCCsi. The main purpose of this file is to establish the environment
# settings specific to user needs. These settings are predominately governed using the Dynaconf configuration
# system (www.dynaconf.com) to switch between environments, which is critical for supporting the array of applications
# and Python distributions necessary to work with the DCCsi framework.
#
# @section Config Description
# The Dynaconf-served startup environment is set to "Default", unless alternatives are passed via cli access, or
# overridden in the "settings.json" file sitting alongside this module. If launched as default, all important locations
# and values are constructed/found/established on startup of the module. Any values in the settings.json file that are
# filled are overrides that remain persistent, unless starting the DCCsi from a saved preset file configuration.
# Intuitive, GUI-driven configuration management will soon be fully supported by way of the DCCsi Launcher Tool, which
# can be surfaced by setting the "STARTUP_MODE" to "launcher" (has yet to be fully implemented). You also have the
# option of passing a file configuration using the "load-settings" flag via CLI
#
# @section Launcher Notes
# - Comments are Doxygen compatible


from pathlib import Path
from dynaconf import settings
from azpy import config_utils
from azpy import constants as constants
import platform
import json
import sys
import os


# TODO - Consolidate settings variables with equal values (ie 'BLENDER_BIN_PATH' and 'DCCSI_BLENDER_PY_BIN', etc)
# TODO - Add Doxygen commenting where needed- currently not fully documented as things are likely to change
# TODO - Add Gem Creation and activation for material conversion


# Get O3DE Python location (CLI functionality requires it to be loaded this way)
try:
    from o3de import manifest, global_project
except ImportError:
    base_path = Path(__file__).parent.absolute()
    o3de_scripts_path = base_path.parents[3]
    print(o3de_scripts_path)
    config_utils.load_module_by_path('o3de', o3de_scripts_path / 'scripts/o3de/o3de/__init__.py')
    from o3de import manifest, global_project


# ++++++++++++++++++++++++++++++++---->>
# INSTANTIATE LOGGING SYSTEM +++++----->>
# ++++++++++++++++++++++++++++++++---->>

import baselogger
logger = baselogger.BaseLogger(Path(__file__).parent.absolute())
_MODULENAME = 'DCCsi.config'
_LOGGER = logger.get_logger(_MODULENAME)
_LOGGER.info('Getting Configuration...')


def initialize_settings():
    if not settings.get('DCCSI_INITIALIZED'):
        initialize_override_settings()
        initialize_core_settings()
        initialize_o3de_settings()
        initialize_maya_settings()
        initialize_blender_settings()
        initialize_substance_settings()
        settings.setenv()


def initialize_override_settings():
    dynaconf_settings = Path(__file__).parent / 'settings.json'
    with open(dynaconf_settings.as_posix()) as f:
        data = json.load(f)
        for key in [key for key in data.keys()]:
            for env_key, env_value in data[key].items():
                if env_value:
                    settings.set(env_key, env_value)


# ++++++++++++++++++++++++++++++++---->>
# GET CORE DCCsi PATHS +++++++++++----->>
# ++++++++++++++++++++++++++++++++---->>

def initialize_core_settings():
    user_os = platform.system()
    user_home = Path.home()
    py_major, py_minor, py_release = sys.version_info[:3]
    pybase = {'Windows': 'python.cmd', 'Linux': 'python.sh', 'Darwin': 'python.sh'}
    dccsi_root = Path(os.path.abspath(__file__)).parent
    repository_root = dccsi_root.parents[3]
    build_root = repository_root / 'build'
    build_profile = build_root / 'bin/profile'
    o3de_python_root = repository_root / 'python'
    dccsi_py_ide = Path(sys.executable)
    dccsi_tools_directory = dccsi_root / 'Tools'
    dccsi_dcc_tools_directory = dccsi_tools_directory / 'Tools'
    dccsi_dev_tools_directory = dccsi_tools_directory / 'Dev/Windows'
    dccsi_python_root = dccsi_root / '3rdParty/Python'
    dccsi_python_library = dccsi_python_root / f'Lib/{py_major}.x/{py_major}.{py_minor}.x/site-packages'
    dccsi_log_path = dccsi_root / '.temp/logs/dccsi.log'
    dccsi_python_base = o3de_python_root / pybase.get(user_os)
    o3de_scripts_directory = repository_root / 'scripts/o3de'
    o3de_stylesheet = repository_root / 'Gems/AWSCore/Code/Tools/ResourceMappingTool/style/base_style_sheet.qss'
    bootstrap_location = manifest.get_o3de_folder() / 'Registry/bootstrap.setreg'
    qt_for_python_path = repository_root / 'Gems/QtForPython/3rdParty/pyside2/windows/release'
    qt_plugin_path = build_profile / 'EditorPlugins'
    qt_platform_plugin_path = qt_plugin_path / 'platforms'
    os.environ["QT_PLUGIN_PATH"] = qt_plugin_path.as_posix()
    os.environ["DYNACONF_QTFORPYTHON_PATH"] = qt_for_python_path.as_posix()
    os.environ["QT_QPA_PLATFORM_PLUGIN_PATH"] = qt_platform_plugin_path.as_posix()

    # Update Core Dynaconf Settings
    settings.set('DCCSI_INITIALIZED', True)
    settings.set('DCCSI_PY_BASE', dccsi_python_base)
    settings.set('DCCSI_PY_DEFAULT', dccsi_python_base)
    settings.set('DCCSI_PY_IDE', dccsi_py_ide)
    settings.set('DCCSI_PY_VERSION_MAJOR', py_major)
    settings.set('DCCSI_PY_VERSION_MINOR', py_minor)
    settings.set('DCCSI_PY_VERSION_RELEASE', py_release)
    settings.set('DCCSI_LOG_PATH', dccsi_log_path)
    settings.set('GLOBAL_QT_STYLESHEET', o3de_stylesheet)
    settings.set('O3DE_DEV', repository_root)
    settings.set('O3DE_BUILD_FOLDER', build_root)
    settings.set('PATH_DCCSIG', dccsi_root)
    settings.set('PATH_DCCSI_PYTHON', dccsi_python_library)
    settings.set('PATH_DCCSI_PYTHON_LIB', dccsi_python_library)
    settings.set('PATH_DCCSI_TOOLS', dccsi_tools_directory)
    settings.set('PATH_DCCSI_TOOLS_DCC', dccsi_dcc_tools_directory)
    settings.set('PATH_DCCSI_TOOLS_DEV', dccsi_dev_tools_directory)
    settings.set('PATH_USER_HOME', user_home)
    settings.set('PATH_O3DE_BIN', build_profile)
    settings.set('PATH_O3DE_BUILD', build_root)
    settings.set('PATH_O3DE_PROJECT', dccsi_root)
    settings.set('PATH_O3DE_PYTHON_INSTALL', o3de_python_root)
    settings.set('SYSTEM_TYPE', platform.system())
    settings.set('PATH_O3DE_PROJECT', global_project.get_global_project(bootstrap_location))
    settings.set('QTFORPYTHON_PATH', qt_for_python_path)
    settings.set('QT_PLUGIN_PATH', qt_plugin_path)

    # Add Paths to "path" Environment Variable
    path_list = [p for p in sys.path]
    path_list.append(Path(sys.executable).as_posix())
    path_list.append(build_profile.as_posix())
    path_list.append(o3de_python_root.as_posix())
    path_list.append(o3de_scripts_directory.as_posix())
    path_list.append(dccsi_dev_tools_directory.as_posix())
    path_list.append(dccsi_python_library.as_posix())

    os.environ['path'] = ''
    for target_path in list(set(path_list)):
        if Path(target_path).exists():
            os.environ['path'] = target_path + os.pathsep + os.environ['path']

    # Get DCC Application locations
    legacy_applications_directory = Path(os.environ['ProgramFiles(x86)'])
    local_applications_directory = user_home / 'AppData/Local'
    applications_directory = Path(os.environ['ProgramFiles'])
    vscode_location = local_applications_directory / 'Programs/Microsoft VS Code'
    pycharm_location = list(local_applications_directory.glob('Jetbrains/Toolbox/apps/PyCharm-P/*/*/bin/pycharm64.exe'))
    pycharm_location += list(applications_directory.glob('JetBrains/*/bin/pycharm64.exe'))
    wing_location = list(legacy_applications_directory.glob('Wing Pro*/wingdb.exe'))
    maya_location = list(applications_directory.glob('Autodesk/Maya*/bin/maya.exe'))
    maya_python_location = list(applications_directory.glob('Autodesk/Maya*/bin/mayapy.exe'))
    blender_location = list(applications_directory.glob('Blender Foundation/Blender*/blender.exe'))
    blender_python_location = list(applications_directory.glob('Blender Foundation/Blender*/*/python/bin/python.exe'))
    houdini_standard_base = list(applications_directory.glob('Side Effects Software/Houdini*/bin/houdini.exe'))
    houdini_python_location = list(applications_directory.glob('Side Effects Software/Houdini*/python37/python.exe'))
    substance_location = list(applications_directory.glob('Adobe/Adobe Substance 3D Designer/*Designer.exe'))
    substance_location += list(applications_directory.glob('Allegorithmic/Substance Designer/Substance Designer.exe'))
    substance_python_location = list(applications_directory.glob(
        'Adobe/Adobe Substance 3D Designer/plugins/pythonsdk/python.exe'))
    substance_automation_toolkit_location = dccsi_python_library / 'pysbs'
    if substance_automation_toolkit_location.is_dir():
        settings.set("PATH_PYSBS", substance_automation_toolkit_location.as_posix())

    app_locations = {
        'PYCHARM_EXE': {'pycharm64.exe': pycharm_location},
        'WING_EXE': {'wingdb.exe': wing_location},
        'VSCODE_EXE': {'code.exe': vscode_location},
        'MAYA_EXE': {'maya.exe': maya_location},
        'MAYA_PY': {'python.exe': maya_python_location},
        'BLENDER_EXE': {'blender.exe': blender_location},
        'BLENDER_PY': {'python.exe': blender_python_location},
        'SUBSTANCE_EXE': {'Adobe Substance 3D Designer.exe': substance_location},
        'SUBSTANCE_PY': {'python.exe': substance_python_location},
        'HOUDINI_EXE': {'houdini.exe': houdini_standard_base},
        'HOUDINI_PY': {'python.exe': houdini_python_location}
    }

    # Add application paths to Dynaconf
    for key, values in app_locations.items():
        for app_name, app_search_paths in values.items():
            if isinstance(app_search_paths, list):
                installation_path = max(app_search_paths, key=lambda x: x.stat().st_ctime)
            else:
                installation_path = max(app_search_paths.glob(app_name), key=lambda x: x.stat().st_ctime)
            settings.set(key, installation_path.as_posix())


# ++++++++++++++++++++++++++++++++---->>
# INITIALIZE O3DE SETTINGS +++++++----->>
# ++++++++++++++++++++++++++++++++---->>

def initialize_o3de_settings():
    registered_projects = manifest.get_all_projects()
    registered_engines = manifest.get_manifest_engines()
    bootstrap_location = manifest.get_o3de_folder() / 'Registry/bootstrap.setreg'
    current_project = global_project.get_global_project(bootstrap_location).as_posix()
    current_engine_projects = manifest.get_engine_projects()
    current_engine_gems = manifest.get_engine_gems()
    engine_external_directories = manifest.get_engine_external_subdirectories()
    o3de_manifest = manifest.get_o3de_manifest()

    settings.set('REGISTERED_PROJECTS', registered_projects)
    settings.set('REGISTERED_ENGINE', registered_engines)
    settings.set('BOOTSTRAP_LOCATION', bootstrap_location)
    settings.set('CURRENT_PROJECT', current_project)
    settings.set('CURRENT_ENGINE_PROJECTS', current_engine_projects)
    settings.set('CURRENT_ENGINE_GEMS', current_engine_gems)
    settings.set('ENGINE_EXTERNAL_DIRECTORIES', engine_external_directories)
    settings.set('O3DE_MANIFEST', o3de_manifest)


# ++++++++++++++++++++++++++++++++---->>
# INITIALIZE DCC SETTINGS ++++++++----->>
# ++++++++++++++++++++++++++++++++---->>

def initialize_maya_settings():
    if settings.get('MAYA_EXE'):
        maya_bin_path = Path(settings.get('MAYA_EXE')).parent
        maya_python_path = maya_bin_path / 'mayapy.exe'
        maya_dccsi_tools_path = Path(settings.get('PATH_DCCSI_TOOLS')) / 'DCC/Maya'
        maya_dccsi_plugin_path = maya_dccsi_tools_path / 'plugins'
        maya_shelf_path = maya_dccsi_tools_path / 'Prefs/Shelves'
        maya_dccsi_icons_path = maya_dccsi_tools_path / 'Prefs/Icons'
        maya_dccsi_scripts_path = maya_dccsi_tools_path / 'Scripts'
        maya_dccsi_mel_path = maya_dccsi_scripts_path / 'Mel'
        maya_dccsi_python_path = maya_dccsi_scripts_path / 'Python'
        maya_python_version = get_python_version(maya_python_path)
        maya_version = Path(settings.get('MAYA_EXE')).parents[1].name.split('Maya')[-1]

        settings.set('MAYA_BIN_PATH', maya_bin_path)
        settings.set('MAYA_MODULE_PATH', maya_dccsi_tools_path)
        settings.set('MAYA_PLUG_IN_PATH', maya_dccsi_plugin_path)  # Needs to set the Maya built in envvar
        settings.set('MAYA_SCRIPT_PATH', f'{maya_dccsi_mel_path};{maya_dccsi_python_path}')
        settings.set('PATH_DCCSI_TOOLS_MAYA', maya_dccsi_tools_path)
        settings.set('MAYA_VERSION', maya_version)
        settings.set('DCCSI_PY_MAYA', maya_python_path)
        settings.set('DCCSI_MAYA_PLUG_IN_PATH', maya_dccsi_plugin_path)
        settings.set('DCCSI_MAYA_SHELF_PATH', maya_shelf_path)
        settings.set('DCCSI_MAYA_XBMLANGPATH', maya_dccsi_icons_path)
        settings.set('DCCSI_MAYA_SCRIPT_PATH', maya_dccsi_scripts_path)
        settings.set('DCCSI_MAYA_SCRIPT_MEL_PATH', maya_dccsi_mel_path)
        settings.set('DCCSI_MAYA_SCRIPT_PY_PATH', maya_dccsi_python_path)
        settings.set('DCCSI_PY_VERSION_MAJOR', maya_python_version['major'])
        settings.set('DCCSI_PY_VERSION_MINOR', maya_python_version['minor'])
        settings.set('DCCSI_PY_VERSION_RELEASE', maya_python_version['release'])
        settings.set('MAYA_INITIALIZED', True)


def initialize_blender_settings():
    if settings.get('BLENDER_EXE'):
        blender_location = Path(settings.get('BLENDER_EXE')).parent
        blender_launcher_location = blender_location / 'blender_launcher.exe'
        blender_python_path = list(Path(settings.get('BLENDER_EXE')).parent.glob('*/python/bin/python.exe'))[0]
        blender_bin_path = blender_location / 'bin'
        dccsi_py_ide = blender_bin_path / 'blender.exe'
        blender_scripts_path = blender_python_path.parents[2] / 'scripts'
        blender_add_ons_path = blender_scripts_path / 'addons'
        blender_version = blender_python_path.parents[2].name
        blender_python_version = get_python_version(blender_python_path)
        blender_dccsi_tools_path = Path(settings.get('PATH_DCCSI_TOOLS')) / 'DCC/Blender'

        settings.set('BLENDER_BIN_PATH', blender_bin_path)
        settings.set('BLENDER_PY_IDE', dccsi_py_ide)
        settings.set('DCCSI_BLENDER_EXE', settings.get('BLENDER_EXE'))
        settings.set('DCCSI_PY_DEFAULT', dccsi_py_ide)
        settings.set('DCCSI_PY_BLENDER', blender_python_path)
        settings.set('DCCSI_BLENDER_LOCATION', blender_location)
        settings.set('DCCSI_BLENDER_VERSION', blender_version)
        settings.set('DCCSI_BLENDER_LAUNCHER', blender_launcher_location)
        settings.set('DCCSI_TOOLS_BLENDER_PATH', blender_dccsi_tools_path)
        settings.set('DCCSI_BLENDER_PYTHON', blender_python_path)
        settings.set('DCCSI_BLENDER_PY_BIN', blender_bin_path)
        settings.set('DCCSI_BLENDER_PY_EXE', blender_python_path)
        settings.set('DCCSI_BLENDER_SCRIPTS', blender_scripts_path)
        settings.set('DCCSI_BLENDER_PROJECT', settings.get('PATH_O3DE_PROJECT'))
        settings.set('DCCSI_BLENDER_ADDONS', blender_add_ons_path)
        settings.set('DCCSI_PY_VERSION_MAJOR', blender_python_version['major'])
        settings.set('DCCSI_PY_VERSION_MINOR', blender_python_version['minor'])
        settings.set('DCCSI_PY_VERSION_RELEASE', blender_python_version['release'])
        settings.set('BLENDER_INITIALIZED', True)


def initialize_substance_settings():
    if settings.get('BLENDER_EXE'):
        pass


# ++++++++++++++++++++++++++++++++---->>
# DEBUG MESSAGING ++++++++++++++++----->>
# ++++++++++++++++++++++++++++++++---->>

_LOGGER.debug('Initializing: {}.'.format(_MODULENAME))
# _LOGGER.debug('DCCSI_GDEBUG: {}'.format(settings.get('DCCSI_GDEBUG')))
# _LOGGER.debug('DCCSI_DEV_MODE: {}'.format(settings.get('DCCSI_DEV_MODE')))
# _LOGGER.debug('DCCSI_LOGLEVEL: {}'.format(settings.get('DCCSI_LOG_LEVEL')))


# ++++++++++++++++++++++++++++++++---->>
# VALIDATION +++++++++++++++++++++----->>
# ++++++++++++++++++++++++++++++++---->>

# TODO - Validation is possible with Dynaconf, but adding validation is only necessary on the condition of using
# TODO - persistent settings files (as opposed to automatic bootstrapping on startup). If left at default settings,
# TODO - path validation is already handled. Other values that require proper settings need to be determined- I assume
# TODO - that realistically only a subset of values need to be validated


def launch_dccsi():
    custom_configuration = settings.get('DCCSI_SETTINGS_FILE_CFG')
    if not settings.get('DCCSI_INITIALIZED'):
        if custom_configuration:
            config_utils.load_preconfigured_settings(custom_configuration)
        else:
            initialize_settings()


def get_python_version(interpreter_location=None):
    """!
    This system needs to be worked out- I kind of got caught up trying to get this working, but had promised to
    get this module and many others viewable EOD. For now it just returns current Python version information and
    ignores targeted queries based on executable paths. It would be valuable to have a system that can retrieve
    version numbers on request, rather than hard coding and consistently needing to update

    @param interpreter_location The path to the Python executable to gather version info. If no path is added, it will
    default to the currently running Python executable
    """
    from azpy.test import get_python_version
    version_info = get_python_version.get_python_version(qprocess_query=False)
    return {'major': version_info['major'], 'minor': version_info['minor'], 'release': version_info['micro']}

    # TODO - Add switcher for setting process type (i.e. Detached) in kwargs for handling
    # if interpreter_location:
    #     from azpy.shared import qt_process
    #     version_script = Path(settings.get('PATH_DCCSIG')) / 'azpy/test/get_python_version.py'
    #     prc = qt_process.QtProcess(interpreter_location.as_posix(), version_script.as_posix())
    #     version_info = prc.process_info.connect(prc.get_process_output)
    #     prc.start_process(False)
    # else:
    #     from azpy.test import get_python_version
    #     version_info = get_python_version.get_python_version()


if __name__ == '__main__':
    # Run this file as a standalone cli script for testing/debugging
    _LOGGER.info(f'+++ {_MODULENAME}.py. Running script as __main__')
    initialize_settings()

    import time
    import argparse

    os.system('color 8e')
    timer_start = time.process_time()

    parser = argparse.ArgumentParser(
        description='O3DE DCCsi Dynamic Config (dynaconf)',
        epilog="Attempts to determine O3DE project if -pp not set")

    parser.add_argument('-gd', '--global-debug',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables global debug flag.')

    parser.add_argument('-dm', '--developer-mode',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables dev mode for early auto attaching debugger.')

    parser.add_argument('-sd', '--set-debugger',
                        type=str,
                        required=False,
                        default='WING',
                        help='(NOT IMPLEMENTED) Default debugger: WING, Also supported: PYCHARM and VSCODE.')

    parser.add_argument('-ep', '--engine-path',
                        type=Path,
                        required=False,
                        default=Path('{ to do: implement }'),
                        help='The path to the o3de engine.')

    parser.add_argument('-eb', '--engine-bin-path',
                        type=Path,
                        required=False,
                        default=Path('{ to do: implement }'),
                        help='The path to the o3de engine binaries (build/bin/profile).')

    parser.add_argument('-bf', '--build-folder',
                        type=str,
                        required=False,
                        default='build',
                        help='The name (tag) of the o3de build folder, example build or windows.')

    parser.add_argument('-pp', '--project-path',
                        type=Path,
                        required=False,
                        default=Path('{ to do: implement }'),
                        help='The path to the project.')

    parser.add_argument('-pn', '--project-name',
                        type=str,
                        required=False,
                        default='{ to do: implement }',
                        help='The name of the project.')

    parser.add_argument('-py', '--enable-python',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables O3DE python access.')

    parser.add_argument('-qt', '--enable-qt',
                        type=bool,
                        required=False,
                        default=False,
                        help='Enables O3DE Qt & PySide2 access.')

    parser.add_argument('-ls', '--load-settings',
                        type=Path,
                        required=False,
                        default=False,
                        help='Use settings from a specified path.')

    parser.add_argument('-tp', '--test-pyside2',
                        type=bool,
                        required=False,
                        default=False,
                        help='Runs Qt/PySide2 tests and reports.')

    parser.add_argument('-ex', '--exit',
                        type=bool,
                        required=False,
                        default=False,
                        help='Exits python. Do not exit if you want to be in interactive interpreter after config.')

    args = parser.parse_args()

    # Modify Dynaconf settings based on CLI arguments. Most need to be implemented still- I have some ideas/questions
    # about the modular approach envisioned that will help instruct the best way to wire in the remaining layered
    # elements

    if args.load_settings:
        config_utils.load_preconfigured_settings(args.load_settings)
    else:
        initialize_core_settings()

    if args.global_debug:
        _LOGGER.info('Global Debug fired')
        settings.set('DCCSI_GDEBUG', True)

    if args.developer_mode:
        _LOGGER.info('Developer mode fired')
        settings.set('DCCSI_DEV_MODE', True)
        config_utils.attach_debugger()

    # Configuration information below is printed to the console (as opposed to using logging) to keep the information
    # sent to the console succinct and clearly readable (logging formatting makes it less so for looped output)

    # Print Dynaconf environment values
    print(f'\n{constants.STR_CROSSBAR}\n~ Dynaconf [{settings.current_env}] environment values:\n'
          f'{constants.STR_CROSSBAR}')
    sorted_keys = sorted(settings.keys(), key=lambda x: x.lower())
    for setting in sorted_keys:
        if settings[setting] and 'FOR_DYNACONF' not in setting:
            print(f'{setting} = {settings[setting]}')

    # Print OS 'path' location
    print(f'\n{constants.STR_CROSSBAR}\n~ Environment PATH locations:\n{constants.STR_CROSSBAR}')
    environment_paths = os.environ['path'].split(';')
    for p in environment_paths:
        print(p)

    # Print OS 'pythonpath' locations
    print(f'{constants.STR_CROSSBAR}\n~ Environment PYTHONPATH locations:\n{constants.STR_CROSSBAR}')
    pp_locations = os.environ['pythonpath'].split(';')
    for pp in pp_locations:
        print(pp)

else:
    launch_dccsi()

