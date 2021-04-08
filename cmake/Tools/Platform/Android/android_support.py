#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import imghdr
import datetime
import logging
import os
import json
import platform
import re
import shutil
import stat
import string
import sys
import subprocess
import pathlib

from distutils.version import LooseVersion

# Resolve the common python module
ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common


APP_NAME = 'app'
ANDROID_MANIFEST_FILE = 'AndroidManifest.xml'
ANDROID_LIBRARIES_JSON_FILE = 'android_libraries.json'
BUILD_CONFIGURATIONS = ['Debug', 'Profile', 'Release']

# We currently only support arm64-v8a
ANDROID_ARCH = 'arm64-v8a'

ANDROID_RESOLUTION_SETTINGS = ('mdpi', 'hdpi', 'xhdpi', 'xxhdpi', 'xxxhdpi')

DEFAULT_CONFIG_CHANGES = [
    'keyboard',
    'keyboardHidden',
    'orientation',
    'screenSize',
    'smallestScreenSize',
    'screenLayout',
    'uiMode',
]

TEST_RUNNER_PROJECT = 'AzTestRunner'
TEST_RUNNER_PACKAGE_NAME = 'com.lumberyard.tests'

# Android Orientation Constants
ORIENTATION_LANDSCAPE = 1 << 0
ORIENTATION_PORTRAIT = 1 << 1
ORIENTATION_ALL = (ORIENTATION_LANDSCAPE | ORIENTATION_PORTRAIT)

ORIENTATION_FLAG_TO_KEY_MAP = {
    ORIENTATION_LANDSCAPE: 'land',
    ORIENTATION_PORTRAIT: 'port',
}

ORIENTATION_MAPPING = {
    'landscape': ORIENTATION_LANDSCAPE,
    'reverseLandscape': ORIENTATION_LANDSCAPE,
    'sensorLandscape': ORIENTATION_LANDSCAPE,
    'userLandscape': ORIENTATION_LANDSCAPE,
    'portrait': ORIENTATION_PORTRAIT,
    'reversePortrait': ORIENTATION_PORTRAIT,
    'sensorPortrait': ORIENTATION_PORTRAIT,
    'userPortrait': ORIENTATION_PORTRAIT
}

MIPMAP_PATH_PREFIX = 'mipmap'

APP_ICON_NAME = 'app_icon.png'
APP_SPLASH_NAME = 'app_splash.png'

PYTHON_SCRIPT = 'python.cmd' if platform.system() == 'Windows' else 'python.sh'

ANDROID_LAUNCHER_NAME_PATTERN = "{project_name}.GameLauncher"

class AndroidProjectManifestEnvironment(object):
    """
    This class manages the environment for the AndroidManifiest.xml template file, based on project settings and environments
    that were passed in or calculated from the command line arguments.
    """

    def __init__(self, engine_root, project_path, android_sdk_version_number, android_ndk_platform_number, is_test:bool):
        """
        Initialize the object with the project specific parameters and values for the game project

        :param engine_root:                 The path where the engine is located
        :param project_path:                The path were the project is located
        :param android_sdk_version_number:  The android SDK platform version
        :param android_ndk_platform_number: The android NDK platform version
        :param is_test:                     Indicates if theAzTestRunner application should be run
        """

        if is_test:
            # The AzTestRunner project.json is located under {engine_root}/Code/Tools/AzTestRunner/Platform/Android/android_project.json
            project_properties_path = engine_root / 'Code' / 'Tools' / 'AzTestRunner' / 'Platform' / 'Android' / 'android_project.json'
        else:
            # The project.json file is located under the game name folder
            project_properties_path = project_path / 'project.json'
        # Read and parse the project.json file into a dictionary to process the specific attributes needed for the manifest template
        project_properties_content = project_properties_path.resolve(strict=True)\
                                                                             .read_text(encoding=common.DEFAULT_TEXT_READ_ENCODING,
                                                                                        errors=common.ENCODING_ERROR_HANDLINGS)
        self.project_path = project_path

        # Extract the key attributes we need to process and build up our environment table
        project_json = json.loads(project_properties_content)

        project_name = project_json.get('project_name')
        if not project_name:
            raise common.LmbrCmdError(f"Missing required 'project_name' from project.json for project at '{str(project_path)}'")
        product_name = project_json.get('product_name', project_name)

        game_project_android_settings = project_json['android_settings']

        package_name = game_project_android_settings["package_name"]

        package_path = package_name.replace('.', '/')

        project_activity = f'{TEST_RUNNER_PROJECT}Activity' if is_test else f'{project_name}Activity'

        # Multiview options require special processing
        multi_window_options = AndroidProjectManifestEnvironment.process_android_multi_window_options(game_project_android_settings)

        self.internal_dict = {
            'ANDROID_PACKAGE':                  package_name,
            'ANDROID_PACKAGE_PATH':             package_path,
            'ANDROID_VERSION_NUMBER':           game_project_android_settings["version_number"],
            "ANDROID_VERSION_NAME":             game_project_android_settings["version_name"],
            "ANDROID_SCREEN_ORIENTATION":       game_project_android_settings["orientation"],
            'ANDROID_APP_NAME':                 TEST_RUNNER_PROJECT if is_test else product_name,       # external facing name
            'ANDROID_PROJECT_NAME':             TEST_RUNNER_PROJECT if is_test else project_name,     # internal facing name
            'ANDROID_PROJECT_ACTIVITY':         project_activity,
            'ANDROID_LAUNCHER_NAME':            TEST_RUNNER_PROJECT if is_test else ANDROID_LAUNCHER_NAME_PATTERN.format(project_name=project_name),
            'ANDROID_CONFIG_CHANGES':           multi_window_options['ANDROID_CONFIG_CHANGES'],
            'ANDROID_APP_PUBLIC_KEY':           game_project_android_settings.get('app_public_key', 'NoKey'),
            'ANDROID_APP_OBFUSCATOR_SALT':      game_project_android_settings.get('app_obfuscator_salt', ''),
            'ANDROID_USE_MAIN_OBB':             game_project_android_settings.get('use_main_obb', 'false'),
            'ANDROID_USE_PATCH_OBB':            game_project_android_settings.get('use_patch_obb', 'false'),
            'ANDROID_ENABLE_KEEP_SCREEN_ON':    game_project_android_settings.get('enable_keep_screen_on', 'false'),
            'ANDROID_DISABLE_IMMERSIVE_MODE':   game_project_android_settings.get('disable_immersive_mode', 'false'),
            'ANDROID_MIN_SDK_VERSION':          android_ndk_platform_number,
            'ANDROID_TARGET_SDK_VERSION':       android_sdk_version_number,
            'ICONS':                            game_project_android_settings.get('icons', None),
            'SPLASH_SCREEN':                    game_project_android_settings.get('splash_screen', None),

            'ANDROID_MULTI_WINDOW':             multi_window_options['ANDROID_MULTI_WINDOW'],
            'ANDROID_MULTI_WINDOW_PROPERTIES':  multi_window_options['ANDROID_MULTI_WINDOW_PROPERTIES'],

            'SAMSUNG_DEX_KEEP_ALIVE':           multi_window_options['SAMSUNG_DEX_KEEP_ALIVE'],
            'SAMSUNG_DEX_LAUNCH_WIDTH':         multi_window_options['SAMSUNG_DEX_LAUNCH_WIDTH'],
            'SAMSUNG_DEX_LAUNCH_HEIGHT':        multi_window_options['SAMSUNG_DEX_LAUNCH_HEIGHT']
        }

    def __getitem__(self, item):
        return self.internal_dict.get(item)

    @staticmethod
    def process_android_multi_window_options(game_project_android_settings):
        """
        Perform custom processing for game projects that have custom 'multi_window_options' in their project.json definition
        :param game_project_android_settings:   The parsed out android settings from the game's project.json
        :return: Dictionary of attributes for any optional multiview option detected from the android settings
        """

        def is_number_option_valid(value, name):
            if value:
                if isinstance(value, int):
                    return True
                else:
                    logging.warning('[WARN] Invalid value for property "%s", expected whole number', name)
            return False

        def get_int_attribute(settings, key_name):
            settings_value = settings.get(key_name, None)
            if not settings_value:
                return None
            if not isinstance(settings_value, int):
                logging.warning('[WARN] Invalid value for property "%s", expected whole number', key_name)
                return None
            return settings_value

        multi_window_options = {
            'SAMSUNG_DEX_LAUNCH_WIDTH':         '',
            'SAMSUNG_DEX_LAUNCH_HEIGHT':        '',
            'SAMSUNG_DEX_KEEP_ALIVE':           '',
            'ANDROID_CONFIG_CHANGES':           '|'.join(DEFAULT_CONFIG_CHANGES),
            'ANDROID_MULTI_WINDOW_PROPERTIES':  '',
            'ANDROID_MULTI_WINDOW':             '',
            'ORIENTATION':                      ORIENTATION_ALL
        }

        multi_window_settings = game_project_android_settings.get('multi_window_options', None)
        if not multi_window_settings:
            # If there are no multi-window options, then set the orientation to the orientation attribute if set, otherwise use the default 'ALL' orientation
            requested_orientation = game_project_android_settings['orientation']
            multi_window_options['ORIENTATION'] = ORIENTATION_MAPPING.get(requested_orientation, ORIENTATION_ALL)
            return multi_window_options

        launch_in_fullscreen = False

        # the Samsung DEX specific values can be added regardless of target API and multi-window support
        samsung_dex_options = multi_window_settings.get('samsung_dex_options', None)
        if samsung_dex_options:
            launch_in_fullscreen = samsung_dex_options.get('launch_in_fullscreen', False)

            # setting the launch window size in DEX mode since launching in fullscreen is strictly tied
            # to multi-window being enabled
            launch_width = get_int_attribute(samsung_dex_options, 'launch_width')
            launch_height = get_int_attribute(samsung_dex_options, 'launch_height')

            # both have to be specified otherwise they are ignored
            if launch_width and launch_height:
                multi_window_options['SAMSUNG_DEX_LAUNCH_WIDTH'] = (f'<meta-data '
                                                                    f'android:name="com.samsung.android.sdk.multiwindow.dex.launchwidth" '
                                                                    f'android:value="{launch_width}"'
                                                                    f'/>')

                multi_window_options['SAMSUNG_DEX_LAUNCH_HEIGHT'] = (f'<meta-data '
                                                                     f'android:name="com.samsung.android.sdk.multiwindow.dex.launchheight" '
                                                                     f'android:value="{launch_height}"'
                                                                     f'/>')

                keep_alive = samsung_dex_options.get('keep_alive', None)
                if keep_alive in (True, False):
                    multi_window_options['SAMSUNG_DEX_KEEP_ALIVE'] = f'<meta-data ' \
                                                                     f'android:name="com.samsung.android.keepalive.density" ' \
                                                                     f'android:value="{str(keep_alive).lower()}" ' \
                                                                     f'/>'

        multi_window_enabled = multi_window_settings.get('enabled', False)

        # the option to change the display resolution was added in API 24 as well, these changes are sent as density changes
        multi_window_options['ANDROID_CONFIG_CHANGES'] = '|'.join(DEFAULT_CONFIG_CHANGES + ['density'])

        # if targeting above the min API level the default value for this attribute is true so we need to explicitly disable it
        multi_window_options['ANDROID_MULTI_WINDOW'] = f'android:resizeableActivity="{str(multi_window_enabled).lower()}"'

        if not multi_window_enabled:
            return multi_window_options

        # remove the DEX launch window size if requested to launch in fullscreen mode
        if launch_in_fullscreen:
            multi_window_options['SAMSUNG_DEX_LAUNCH_WIDTH'] = ''
            multi_window_options['SAMSUNG_DEX_LAUNCH_HEIGHT'] = ''

        default_width = multi_window_settings.get('default_width', None)
        default_height = multi_window_settings.get('default_height', None)

        min_width = multi_window_settings.get('min_width', None)
        min_height = multi_window_settings.get('min_height', None)

        gravity = multi_window_settings.get('gravity', None)

        layout = ''
        if any([default_width, default_height, min_width, min_height, gravity]):
            layout = '<layout '

            # the default width/height values are respected as launch values in DEX mode so they should
            # be ignored if the intention is to launch in fullscreen when running in DEX mode
            if not launch_in_fullscreen:
                if is_number_option_valid(default_width, 'default_width'):
                    layout += f'android:defaultWidth="{default_width}dp" '

                if is_number_option_valid(default_height, 'default_height'):
                    layout += f'android:defaultHeight="{default_height}dp" '

            if is_number_option_valid(min_height, 'min_height'):
                layout += f'android:minHeight="{min_height}dp" '

            if is_number_option_valid(min_width, 'min_width'):
                layout += f'android:minWidth="{min_width}dp" '

            if gravity:
                layout += f'android:gravity="{gravity}" '

            layout += '/>'

        multi_window_options['ANDROID_MULTI_WINDOW_PROPERTIES'] = layout

        return multi_window_options


PLATFORM_SETTINGS_FORMAT = """
# Auto Generated from last cmake project generation ({generation_timestamp})

[settings]
platform={platform}
game_projects={project_path}
asset_deploy_mode={asset_mode}
asset_deploy_type={asset_type}

[android]
android_sdk_path={android_sdk_path}
embed_assets_in_apk={embed_assets_in_apk}
is_unit_test={is_unit_test}
"""

NATIVE_CMAKE_SECTION_ANDROID_FORMAT = """
    externalNativeBuild {{
        cmake {{
            buildStagingDirectory "."
            version "{cmake_version}"
            path "{absolute_cmakelist_path}"
        }}
    }}
"""

NATIVE_CMAKE_SECTION_DEFAULT_CONFIG_NDK_FORMAT_STR = """
        ndk {{
            abiFilters '{abi}'
        }}
"""

NATIVE_CMAKE_SECTION_BUILD_TYPE_CONFIG_FORMAT_STR = """
            externalNativeBuild {{
                cmake {{
                    {targets_section}
                    arguments {arguments}
                }}
            }}
"""

CUSTOM_GRADLE_COPY_NATIVE_CONFIG_FORMAT_STR = """
    task copyNativeLibs{config}(type: Copy) {{
        delete 'outputs/native-lib/{abi}'

        from fileTree(dir: 'build/intermediates/cmake/{config_lower}/obj/arm64-v8a/{config_lower}', include: '**/*.so', exclude: 'lib{project_name}.GameLauncher.so' )
        into  'outputs/native-lib/{abi}'
    }}

    compile{config}Sources.dependsOn copyNativeLibs{config}

    copyNativeLibs{config}.mustRunAfter {{
        tasks.findAll {{ task->task.name.contains('externalNativeBuild{config}') }}
    }}
"""

CUSTOM_GRADLE_COPY_NATIVE_CONFIG_BUILD_ARTIFACTS_FORMAT_STR = """
    task copyNativeArtifacts{config}(type: Copy) {{
        from fileTree(dir: 'build/intermediates/cmake/{config_lower}/obj/arm64-v8a/{config_lower}', include: '{file_includes}' )
        into  '{asset_layout_folder}'
    }}

    compile{config}Sources.dependsOn copyNativeArtifacts{config}

    copyNativeArtifacts{config}.mustRunAfter {{
        tasks.findAll {{ task->task.name.contains('externalNativeBuild{config}') }}
    }}
"""

CUSTOM_APPLY_ASSET_LAYOUT_TASK_FORMAT_STR = """
    task syncLYLayoutMode{config}(type:Exec) {{
        workingDir '{working_dir}'
        commandLine '{python_full_path}', 'layout_tool.py', '--project-path', '{project_path}', '-p', 'Android', '-a', '{asset_type}', '-m', '{asset_mode}', '--create-layout-root', '-l', '{asset_layout_folder}'
    }}
    compile{config}Sources.dependsOn syncLYLayoutMode{config}
"""


PROJECT_DEPENDENCIES_VALUE_FORMAT = """
dependencies {{
{dependencies}
    api 'androidx.core:core:1.1.0'
}}
"""

OVERRIDE_JAVA_SOURCESET_STR = """
            java {{
                srcDirs = ['{absolute_azandroid_path}', 'src/main/java']
            }}
"""

class AndroidSigningConfig(object):
    """
    Class to manage android signing configs
    """
    def __init__(self, store_file, store_password, key_alias, key_password):
        if not store_file:
            raise common.LmbrCmdError(f"Keystore file not supplied for signing configuration",
                                      common.ERROR_CODE_INVALID_PARAMETER)
        if not os.path.isfile(store_file):
            raise common.LmbrCmdError(f"Missing/Invalid keystore file {store_file} for signing config",
                                      common.ERROR_CODE_INVALID_PARAMETER)
        self.store_file = store_file.replace('\\', '/')

        if not store_password:
            raise common.LmbrCmdError(f"Keystore password not supplied for signing configuration",
                                      common.ERROR_CODE_INVALID_PARAMETER)
        self.store_password = store_password

        if not key_alias:
            raise common.LmbrCmdError(f"Signing key alias not supplied for signing configuration",
                                      common.ERROR_CODE_INVALID_PARAMETER)
        self.key_alias = key_alias

        if not key_password:
            raise common.LmbrCmdError(f"Signing key password not supplied for signing configuration",
                                      common.ERROR_CODE_INVALID_PARAMETER)
        self.key_password = key_password

    def to_template_string(self, tabs):
        tab_prefix = ' '*4*tabs
        return f"""
{tab_prefix}storeFile file('{self.store_file}')
{tab_prefix}storePassword '{self.store_password}'
{tab_prefix}keyPassword '{self.key_password}'
{tab_prefix}keyAlias '{self.key_alias}'"""


class AndroidProjectGenerator(object):
    """
    Class the manages the process to generate an android project folder in order to build with gradle/android studio
    """

    def __init__(self, engine_root, build_dir, android_ndk_path, android_sdk_path, android_sdk_version, android_ndk_platform,
                 project_path, third_party_path, cmake_version, override_cmake_path, override_gradle_path, override_ninja_path,
                 android_sdk_build_tool_version, include_assets_in_apk, asset_mode, asset_type, signing_config, is_test_project=False, 
                 overwrite_existing=True):
        """
        Initialize the object with all the required parameters needed to create an Android Project. The parameters should be verified before initializing this object
        
        :param engine_root:             The engine root that contains the engine
        :param build_dir:               The target folder under the where the android project folder will be created
        :param android_ndk_path:        The path to the ANDROID_NDK used for building the native android code 
        :param android_sdk_path:        The path to the ANDROID_SDK used for building the android java code
        :param android_sdk_version:     The android platform version number to use for the Android SDK related builds
        :param android_ndk_platform:    The android platform version number to use for the Android NDK related builds
        :param project_path:            The path to the project
        :param third_party_path:        The required path to the lumberyard 3rd party path
        :param cmake_version:           The version number of cmake that will be used by gradle
        :param override_cmake_path:     The override path to cmake if it does not exists in the system path
        :param override_gradle_path:    The override path to gradle if it does not exists in the system path
        :param override_ninja_path:     The override path to ninja if it does not exists in the system path
        :param android_sdk_build_tool_version:  The preferred android SDK build-tool version. Will default to the first one detected in the android sdk path
        :param include_assets_in_apk:
        :param asset_mode:
        :param asset_type:
        :param signing_config:          Optional signing configuration arguments
        :param is_test_project:         Flag to indicate if this is a unit test runner project. (If true, project_path, asset_mode, asset_type, and include_assets_in_apk are ignored)
        :param overwrite_existing:      Flag to overwrite existing project files when being generated, or skip if they already exist.
        """
        self.env = {}

        self.engine_root = engine_root

        self.build_dir = build_dir

        self.android_ndk_path = android_ndk_path

        self.android_sdk_path = android_sdk_path

        self.android_project_builder_path = self.engine_root / 'Code/Tools/Android/ProjectBuilder'

        self.android_sdk_version = android_sdk_version

        self.android_sdk_build_tool_version = android_sdk_build_tool_version

        self.android_ndk_platform = android_ndk_platform

        self.project_path = project_path

        self.third_party_path = third_party_path

        self.cmake_version = cmake_version

        self.override_cmake_path = override_cmake_path

        self.override_gradle_path = override_gradle_path

        self.override_ninja_path = override_ninja_path

        self.include_assets_in_apk = include_assets_in_apk

        self.asset_mode = asset_mode

        self.asset_type = asset_type

        self.signing_config = signing_config

        self.is_test_project = is_test_project

        self.overwrite_existing = overwrite_existing

    def execute(self):
        """
        Execute the android project creation workflow
        """

        # Prepare the working build directory
        self.build_dir.mkdir(parents=True, exist_ok=True)

        self.create_platform_settings()

        self.create_default_local_properties()

        project_names = self.patch_and_transfer_android_libs()

        project_names.extend(self.create_lumberyard_app(project_names))

        root_gradle_env = {
            'SDK_VER': self.android_sdk_version,
            'NDK_PLATFORM_VER': self.android_ndk_platform,
            'SDK_BUILD_TOOL_VER': self.android_sdk_build_tool_version,
            'LY_ENGINE_ROOT': common.normalize_path_for_settings(self.engine_root)
        }
        # Generate the gradle build script
        self.create_file_from_project_template(src_template_file='root.build.gradle.in',
                                               template_env=root_gradle_env,
                                               dst_file=self.build_dir / 'build.gradle')

        self.write_settings_gradle(project_names)

        self.prepare_gradle_wrapper()

    def create_file_from_project_template(self, src_template_file, template_env, dst_file):
        """
        Create a file from an android template file

        :param src_template_file:       The name of the template file that is located under Code/Tools/Android/ProjectBuilder
        :param template_env:            The dictionary that contains the template substitution values
        :param dst_file:                The target concrete file to write to
        """

        src_template_file_path = self.android_project_builder_path / src_template_file
        if not dst_file.exists() or self.overwrite_existing:

            default_local_properties_content = common.load_template_file(template_file_path=src_template_file_path,
                                                                         template_env=template_env)

            dst_file.write_text(default_local_properties_content,
                                encoding=common.DEFAULT_TEXT_WRITE_ENCODING,
                                errors=common.ENCODING_ERROR_HANDLINGS)

            logging.info('Generated default {}'.format(dst_file.name))
        else:
            logging.info('Skipped {} (file exists)'.format(dst_file.name))

    def prepare_gradle_wrapper(self):
        """
        Generate the gradle wrapper by calling the validated version of gradle.
        """
        logging.info('Preparing Gradle Wrapper')

        if self.override_gradle_path:
            gradle_wrapper_cmd = [self.override_gradle_path]
        else:
            gradle_wrapper_cmd = ['gradle.bat' if platform.system() == 'Windows' else 'gradle']

        gradle_wrapper_cmd.extend(['wrapper', '-p', str(self.build_dir.resolve())])

        proc_result = subprocess.run(gradle_wrapper_cmd,
                                     shell=True)
        if proc_result.returncode != 0:
            raise common.LmbrCmdError("Gradle was unable to generate a gradle wrapper for this project (code {}): {}"
                                      .format(proc_result.returncode, proc_result.stderr or ""),
                                      common.ERROR_CODE_ERROR_NOT_SUPPORTED)

    def create_platform_settings(self):
        """
        Create the 'platform.settings' file for the deployment script to use
        """
        if self.is_test_project:
            platform_settings_content = PLATFORM_SETTINGS_FORMAT.format(generation_timestamp=str(datetime.datetime.now().strftime("%c")),
                                                                        platform='android',
                                                                        project_path=self.project_path,
                                                                        asset_mode='',
                                                                        asset_type='',
                                                                        android_sdk_path=str(self.android_sdk_path),
                                                                        embed_assets_in_apk=True,
                                                                        is_unit_test=True)
        else:
            platform_settings_content = PLATFORM_SETTINGS_FORMAT.format(generation_timestamp=str(datetime.datetime.now().strftime("%c")),
                                                                        platform='android',
                                                                        project_path=self.project_path,
                                                                        asset_mode=self.asset_mode,
                                                                        asset_type=self.asset_type,
                                                                        android_sdk_path=str(self.android_sdk_path),
                                                                        embed_assets_in_apk=str(self.include_assets_in_apk),
                                                                        is_unit_test=False)

        platform_settings_file = self.build_dir / 'platform.settings'
        platform_settings_file.open('w').write(platform_settings_content)

    def create_default_local_properties(self):
        """
        Create the default 'local.properties' file in the build folder
        """
        template_android_ndk_path = common.normalize_path_for_settings(self.android_ndk_path, True)
        template_android_sdk_path = common.normalize_path_for_settings(self.android_sdk_path, True)
        if self.override_cmake_path:
            # The cmake dir references the base cmake folder, not the executable path itself, so resolve to the base folder
            template_cmake_path = common.normalize_path_for_settings(pathlib.Path(self.override_cmake_path).parent.parent, True)
        else:
            template_cmake_path = None

        local_properties_env = {
            "GENERATION_TIMESTAMP": str(datetime.datetime.now().strftime("%c")),
            "ANDROID_NDK_PATH": template_android_ndk_path,
            "ANDROID_SDK_PATH": template_android_sdk_path,
            "CMAKE_DIR_LINE": f'cmake.dir={template_cmake_path}' if template_cmake_path else ''
        }

        self.create_file_from_project_template(src_template_file='local.properties.in',
                                               template_env=local_properties_env,
                                               dst_file=self.build_dir / 'local.properties')

    def patch_and_transfer_android_libs(self):
        """
        Patch and transfer android libraries from the android SDK path based on the rules outlined in Code/Tools/Android/ProjectBuilder/android_libraries.json
        """

        # The android_libraries.json is templatized and needs to be provided the following environment for processing
        # before we can process it.
        android_libraries_substitution_table = {
            "ANDROID_SDK_HOME": common.normalize_path_for_settings(self.android_sdk_path, False),
            "ANDROID_NDK_HOME": common.normalize_path_for_settings(self.android_ndk_path, False),
            "ANDROID_SDK_VERSION": "android-".format(self.android_sdk_version)
        }
        android_libraries_template_json_path = self.android_project_builder_path / ANDROID_LIBRARIES_JSON_FILE

        android_libraries_template_json_content = android_libraries_template_json_path.resolve(strict=True) \
                                                                                      .read_text(encoding=common.DEFAULT_TEXT_READ_ENCODING,
                                                                                                 errors=common.ENCODING_ERROR_HANDLINGS)

        android_libraries_json_content = string.Template(android_libraries_template_json_content) \
                                               .substitute(android_libraries_substitution_table)

        android_libraries_json = json.loads(android_libraries_json_content)

        # Process the android library rules
        libs_to_patch = []

        for libName, value in android_libraries_json.items():
            # The library is in different places depending on the revision, so we must check multiple paths.
            src_dir = None
            for path in value['srcDir']:
                path = string.Template(path).substitute(self.env)
                if os.path.exists(path):
                    src_dir = path
                    break

            if not src_dir:
                raise common.LmbrCmdError('[ERROR] Failed to find library - {} - in path(s) [{}]. Please download the '
                                          'library from the Android SDK Manager and run this command again.'
                                          .format(libName, ", ".join(string.Template(path).substitute(self.env) for path in value['srcDir'])))

            if 'patches' in value:
                lib_to_patch = self._Library(libName, src_dir, self.overwrite_existing, self.signing_config)
                for patch in value['patches']:
                    file_to_patch = self._File(patch['path'])
                    for change in patch['changes']:
                        line_num = change['line']
                        old_lines = change['old']
                        new_lines = change['new']
                        for oldLine in old_lines[:-1]:
                            change = self._Change(line_num, oldLine, (new_lines.pop() if new_lines else None))
                            file_to_patch.add_change(change)
                            line_num += 1
                        else:
                            change = self._Change(line_num, old_lines[-1], ('\n'.join(new_lines) if new_lines else None))
                            file_to_patch.add_change(change)

                    lib_to_patch.add_file_to_patch(file_to_patch)
                    lib_to_patch.dependencies = value.get('dependencies', [])
                    lib_to_patch.build_dependencies = value.get('buildDependencies', [])

                libs_to_patch.append(lib_to_patch)

        patched_lib_names = []

        # Patch the libraries
        for lib in libs_to_patch:
            lib.process_patch_lib(android_project_builder_path=self.android_project_builder_path,
                                  dest_root=self.build_dir)
            patched_lib_names.append(lib.name)

        return patched_lib_names

    def create_lumberyard_app(self, project_dependencies):
        """
        This will create the main lumberyard 'app' which will be packaged as an APK.

        :param project_dependencies:    Local project dependencies that may have been detected previously during construction of the android project folder
        :returns    List (of one) project name for the gradle build properties (see write_settings_gradle)
        """

        az_android_dst_path = self.build_dir / APP_NAME

        # We must always delete 'src' any existing copied AzAndroid projects since building may pick up stale java sources
        lumberyard_app_src = az_android_dst_path / 'src'
        if lumberyard_app_src.exists():
            common.remove_dir_path(lumberyard_app_src)

        logging.debug("Copying AzAndroid to '%s'", az_android_dst_path.resolve())

        # The folder structure from the base lib needs to be mapped to a structure that gradle can process as a
        # build project, and we need to generate some additional files

        # Prepare the target folder
        az_android_dst_path.mkdir(parents=True, exist_ok=True)

        # Prepare the 'PROJECT_DEPENDENCIES' environment variable
        gradle_project_dependencies = [f"    api project(path: ':{project_dependency}')" for project_dependency in project_dependencies]

        template_engine_root = common.normalize_path_for_settings(self.engine_root)
        template_third_party_path = common.normalize_path_for_settings(self.third_party_path)
        template_ndk_path = common.normalize_path_for_settings(self.android_ndk_path)

        gradle_build_env = dict()

        engine_root_as_path= pathlib.Path(self.engine_root)

        absolute_cmakelist_path = (engine_root_as_path / 'CMakeLists.txt').resolve().as_posix()
        absolute_azandroid_path = (engine_root_as_path / 'Code/Framework/AzAndroid/java').resolve().as_posix()

        gradle_build_env['TARGET_TYPE'] = 'application'
        gradle_build_env['PROJECT_DEPENDENCIES'] = PROJECT_DEPENDENCIES_VALUE_FORMAT.format(dependencies='\n'.join(gradle_project_dependencies))
        gradle_build_env['NATIVE_CMAKE_SECTION_ANDROID'] = NATIVE_CMAKE_SECTION_ANDROID_FORMAT.format(cmake_version=str(self.cmake_version), absolute_cmakelist_path=absolute_cmakelist_path)
        gradle_build_env['NATIVE_CMAKE_SECTION_DEFAULT_CONFIG'] = NATIVE_CMAKE_SECTION_DEFAULT_CONFIG_NDK_FORMAT_STR.format(abi=ANDROID_ARCH)

        gradle_build_env['OVERRIDE_JAVA_SOURCESET'] = OVERRIDE_JAVA_SOURCESET_STR.format(absolute_azandroid_path=absolute_azandroid_path)


        gradle_build_env['OPTIONAL_JNI_SRC_LIB_SET'] = ', "outputs/native-lib"'

        for native_config in BUILD_CONFIGURATIONS:

            native_config_upper = native_config.upper()
            native_config_lower = native_config.lower()

            # Prepare the cmake argument list based on the collected android settings and each build config
            cmake_argument_list = [
                '"-GNinja"',
                f'"-S{template_engine_root}"',
                f'"-DCMAKE_BUILD_TYPE={native_config_lower}"',
                f'"-DCMAKE_TOOLCHAIN_FILE={template_engine_root}/cmake/Platform/Android/Toolchain_Android.cmake"',
                f'"-DLY_3RDPARTY_PATH={template_third_party_path}"']

            if not self.is_test_project:
                cmake_argument_list.append(f'"-DLY_PROJECTS={pathlib.PurePath(self.project_path).as_posix()}"')
            else:
                cmake_argument_list.append('"-DLY_TEST_PROJECT=1"')

            cmake_argument_list.extend([
                f'"-DANDROID_NATIVE_API_LEVEL={self.android_ndk_platform}"',
                f'"-DLY_NDK_DIR={template_ndk_path}"',
                '"-DANDROID_STL=c++_shared"',
                '"-Wno-deprecated"',
            ])
            if native_config == 'Release':
                cmake_argument_list.append('"-DLY_MONOLITHIC_GAME=1"')

            if self.override_ninja_path:
                cmake_argument_list.append(f'"-DCMAKE_MAKE_PROGRAM={common.normalize_path_for_settings(self.override_ninja_path)}"')

            # Query the project_path from the project.json file
            project_name = common.read_project_name_from_project_json(self.project_path)
            # Prepare the config-specific section to place the cmake argument list in the build.gradle for the app
            gradle_build_env[f'NATIVE_CMAKE_SECTION_{native_config_upper}_CONFIG'] = \
                NATIVE_CMAKE_SECTION_BUILD_TYPE_CONFIG_FORMAT_STR.format(targets_section=f'targets "{project_name}.GameLauncher"'
                if project_name and not self.is_test_project else "", arguments=','.join(cmake_argument_list))

            if project_name:
                # Prepare the config-specific section to copy the related .so files that are marked as dependencies for the target
                # (launcher) since gradle will not include them automatically for APK import
                gradle_build_env[f'CUSTOM_GRADLE_COPY_NATIVE_{native_config_upper}_LIB_TASK'] = \
                    CUSTOM_GRADLE_COPY_NATIVE_CONFIG_FORMAT_STR.format(config=native_config,
                                                                       config_lower=native_config_lower,
                                                                       project_name=project_name,
                                                                       abi=ANDROID_ARCH,
                                                                       optional_test_excludes=",'*.Tests.so'" if not self.is_test_project else "")

            if self.is_test_project:
                gradle_build_env[f'CUSTOM_APPLY_ASSET_LAYOUT_{native_config_upper}_TASK'] = \
                    CUSTOM_GRADLE_COPY_NATIVE_CONFIG_BUILD_ARTIFACTS_FORMAT_STR.format(config=native_config,
                                                                                       config_lower=native_config_lower,
                                                                                       asset_layout_folder=(self.build_dir / 'app/src/main/assets').as_posix(),
                                                                                       file_includes='Test.Assets/**/*.*')
            else:
                # Copy over settings registry files from the Registry folder with build output directory
                gradle_build_env[f'CUSTOM_APPLY_ASSET_LAYOUT_{native_config_upper}_TASK'] = \
                    CUSTOM_GRADLE_COPY_NATIVE_CONFIG_BUILD_ARTIFACTS_FORMAT_STR.format(config=native_config,
                                                                                       config_lower=native_config_lower,
                                                                                       asset_layout_folder=(self.build_dir / 'app/src/main/assets').as_posix(),
                                                                                       file_includes='**/Registry/*.setreg')

            if self.include_assets_in_apk:
                if not self.is_test_project:
                    gradle_build_env[f'CUSTOM_APPLY_ASSET_LAYOUT_{native_config_upper}_TASK'] += \
                        CUSTOM_APPLY_ASSET_LAYOUT_TASK_FORMAT_STR.format(working_dir=common.normalize_path_for_settings(self.engine_root / 'cmake/Tools'),
                                                                         python_full_path=common.normalize_path_for_settings(self.engine_root / 'python' / PYTHON_SCRIPT),
                                                                         asset_type=self.asset_type,
                                                                         project_path=self.project_path.as_posix(),
                                                                         asset_mode=self.asset_mode if native_config != 'Release' else 'PAK',
                                                                         asset_layout_folder=common.normalize_path_for_settings(self.build_dir / 'app/src/main/assets'),
                                                                         config=native_config)
            else:
                gradle_build_env[f'CUSTOM_APPLY_ASSET_LAYOUT_{native_config_upper}_TASK'] = ''
            if self.signing_config:
                gradle_build_env[f'SIGNING_{native_config_upper}_CONFIG'] = f'signingConfig signingConfigs.{native_config_lower}' if self.signing_config else ''
            else:
                gradle_build_env[f'SIGNING_{native_config_upper}_CONFIG'] = ''

        if self.signing_config:
            gradle_build_env['SIGNING_CONFIGS'] = f"""
    signingConfigs {{
        debug {{{self.signing_config.to_template_string(3)}}}
        profile {{{self.signing_config.to_template_string(3)}}}
        release {{{self.signing_config.to_template_string(3)}}}
    }}
"""
        else:
            gradle_build_env['SIGNING_CONFIGS'] = ""

        az_android_gradle_file = az_android_dst_path / 'build.gradle'
        self.create_file_from_project_template(src_template_file='build.gradle.in',
                                               template_env=gradle_build_env,
                                               dst_file=az_android_dst_path / 'build.gradle')

        # Generate a AndroidManifest.xml and write to ${az_android_dst_path}/src/main/AndroidManifest.xml
        dest_src_main_path = az_android_dst_path / 'src/main'
        dest_src_main_path.mkdir(parents=True)
        az_android_package_env = AndroidProjectManifestEnvironment(engine_root=self.engine_root,
                                                                   project_path=self.project_path,
                                                                   android_sdk_version_number=self.android_sdk_version,
                                                                   android_ndk_platform_number=self.android_ndk_platform,
                                                                   is_test=self.is_test_project)
        self.create_file_from_project_template(src_template_file=ANDROID_MANIFEST_FILE,
                                               template_env=az_android_package_env,
                                               dst_file=dest_src_main_path / ANDROID_MANIFEST_FILE)

        # Apply the 'android_builder.json' rules to copy over additional files to the target
        self.apply_android_builder_rules(az_android_dst_path=az_android_dst_path,
                                         az_android_package_env=az_android_package_env)

        self.resolve_icon_overrides(az_android_dst_path=az_android_dst_path,
                                    az_android_package_env=az_android_package_env)

        self.resolve_splash_overrides(az_android_dst_path=az_android_dst_path,
                                      az_android_package_env=az_android_package_env)

        self.clear_unused_assets(az_android_dst_path=az_android_dst_path,
                                 az_android_package_env=az_android_package_env)

        return [APP_NAME]

    def write_settings_gradle(self, project_list):
        """
        Generate and write the 'settings.gradle' and 'gradle.properties file at the root of the android project folder

        :param project_list:    List of dependent projects to include in the gradle build
        """

        settings_gradle_lines = [f"include ':{project_name}'" for project_name in project_list]
        settings_gradle_content = '\n'.join(settings_gradle_lines)
        settings_gradle_file = self.build_dir / 'settings.gradle'
        settings_gradle_file.write_text(settings_gradle_content,
                                        encoding=common.DEFAULT_TEXT_READ_ENCODING,
                                        errors=common.ENCODING_ERROR_HANDLINGS)
        logging.info("Generated settings.gradle -> %s", str(settings_gradle_file.resolve()))

        # Write the default gradle.properties

        # TODO: Add substitution entries here if variables are added to gradle.properties.in
        # Refer to the Code/Tools/Android/ProjectBuilder/gradle.properties.in for reference.
        grade_properties_env = {}
        gradle_properties_file = self.build_dir / 'gradle.properties'
        self.create_file_from_project_template(src_template_file='gradle.properties.in',
                                               template_env=grade_properties_env,
                                               dst_file=gradle_properties_file)
        logging.info("Generated gradle.properties -> %s", str(gradle_properties_file.resolve()))

    def apply_android_builder_rules(self, az_android_dst_path, az_android_package_env):
        """
        Apply the 'android_builder.json' rule file that was used by WAF to prepare the gradle application apk file.

        :param az_android_dst_path:     The target application folder underneath the android target folder
        :param az_android_package_env:  The template environment to use to process all the source template files
        """

        android_builder_json_path = self.android_project_builder_path / 'android_builder.json'
        android_builder_json_content = common.load_template_file(template_file_path=android_builder_json_path,
                                                                 template_env=az_android_package_env)
        android_builder_json = json.loads(android_builder_json_content)

        # Legacy files that don't need to be copied to the path (not needed for APK processing)
        skip_files = ['wscript']

        def _copy(src_file, dst_path,  dst_is_directory):
            """
            Perform a specialized copy
            :param src_file:    Source file to copy (relative to ${android_project_builder_path})
            :param dst_path:    The destination to copy to
            :param dst_is_directory: Flag to indicate if the destination is a path or a file
            """
            if src_file in skip_files:
                # Filter out files that shouldn't be copied
                return

            src_path = self.android_project_builder_path / src_file
            resolved_src = src_path.resolve(strict=True)

            if imghdr.what(resolved_src) in ('rgb', 'gif', 'pbm', 'ppm', 'tiff', 'rast', 'xbm', 'jpeg', 'bmp', 'png'):
                # If the source file is a binary asset, then perform a copy to the target path
                logging.debug("Copy Binary file %s -> %s", str(src_path.resolve(strict=True)), str(dst_path.resolve()))
                dst_path.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(resolved_src, dst_path.resolve())
            else:
                if dst_is_directory:
                    # If the dst_path is a directory, then we are copying the file to that directory
                    dst_path.mkdir(parents=True, exist_ok=True)
                    dst_file = dst_path / src_file
                else:
                    # Otherwise, we are copying the file to the dst_path directly. A renaming may occur
                    dst_path.parent.mkdir(parents=True, exist_ok=True)
                    dst_file = dst_path

                project_activity_for_game_content = common.load_template_file(template_file_path=src_path,
                                                                              template_env=az_android_package_env)
                dst_file.write_text(project_activity_for_game_content)
                logging.debug("Copy/Update file %s -> %s", str(src_path.resolve(strict=True)), str(dst_path.resolve()))

        def _process_dict(node, dst_path):
            """
            Process a node from the android_builder.json file
            :param node:        The node to process
            :param dst_path:    The destination path derived from the node
            """

            assert isinstance(node, dict), f"Node for {android_builder_json_path} expected to be a dictionary"

            for key, value in node.items():

                if isinstance(value, str):
                    _copy(key, dst_path / value, False)

                elif isinstance(value, list):
                    for item in value:
                        assert isinstance(node, dict), f"Unexpected type found in '{android_builder_json_path}'.  Only lists of strings are supported"
                        _copy(item, dst_path / key, True)

                elif isinstance(value, dict):
                    _process_dict(value, dst_path / key)
                else:
                    assert False, f"Unexpected type '{type(value)}' found in '{android_builder_json_path}'. Only str, list, and dict is supported"

        _process_dict(android_builder_json, az_android_dst_path)

    def construct_source_resource_path(self, source_path):
        """
        Helper to construct the source path to an asset override such as
        application icons or splash screen images

        :param source_path: Source path or file to attempt to locate
        :return: The path to the resource file
        """
        if os.path.isabs(source_path):
            # Always return itself if the path is already and absolute path
            return pathlib.Path(source_path)

        game_gem_resources = self.project_path / 'Gem' / 'Resources'
        if game_gem_resources.is_dir(game_gem_resources):
            # If the source is relative and the game gem's resource is present, construct the path based on that
            return game_gem_resources / source_path

        raise common.LmbrCmdError(f'Unable to locate resources folder for project at path "{self.project_path}"')

    def resolve_icon_overrides(self, az_android_dst_path, az_android_package_env):
        """
        Resolve any icon overrides

        :param az_android_dst_path:     The destination android path (app project folder)
        :param az_android_package_env:  Dictionary of env values to retrieve override information
        """

        dst_resource_path = az_android_dst_path / 'src/main/res'

        icon_overrides = az_android_package_env['ICONS']
        if not icon_overrides:
            return

        # if a default icon is specified, then copy it into the generic mipmap folder
        default_icon = icon_overrides.get('default', None)

        if default_icon is not None:

            src_default_icon_file = self.construct_source_resource_path(default_icon)

            default_icon_target_dir = dst_resource_path / MIPMAP_PATH_PREFIX
            default_icon_target_dir.mkdir(parents=True, exist_ok=True)
            dst_default_icon_file = default_icon_target_dir / APP_ICON_NAME

            shutil.copyfile(src_default_icon_file.resolve(), dst_default_icon_file.resolve())
            os.chmod(dst_default_icon_file.resolve(), stat.S_IWRITE | stat.S_IREAD)
        else:
            logging.debug(f'No default icon override specified for project_at path {self.project_path}')

        # process each of the resolution overrides
        warnings = []
        for resolution in ANDROID_RESOLUTION_SETTINGS:

            target_directory = dst_resource_path / f'{MIPMAP_PATH_PREFIX}-{resolution}'
            target_directory.mkdir(parent=True, exist_ok=True)

            # get the current resolution icon override
            icon_source = icon_overrides.get(resolution, default_icon)
            if icon_source is default_icon:

                # if both the resolution and the default are unspecified, warn the user but do nothing
                if icon_source is None:
                    warnings.append(f'No icon override found for "{resolution}".  Either supply one for "{resolution}" or a '
                                    f'"default" in the android_settings "icon" section of the project.json file for {self.project_path}')

                # if only the resolution is unspecified, remove the resolution specific version from the project
                else:
                    logging.debug(f'Default icon being used for "{resolution}" in {self.project_path}', resolution)
                    common.remove_dir_path(target_directory)
                continue

            src_icon_file = self.construct_source_resource_path(icon_source)
            dst_icon_file = target_directory / APP_ICON_NAME

            shutil.copyfile(src_icon_file.resolve(), dst_icon_file.resolve())
            os.chmod(dst_icon_file.resolve(), stat.S_IWRITE | stat.S_IREAD)

        # guard against spamming warnings in the case the icon override block is full of garbage and no actual overrides
        if len(warnings) != len(ANDROID_RESOLUTION_SETTINGS):
            for warning_msg in warnings:
                logging.warning(warning_msg)

    def resolve_splash_overrides(self, az_android_dst_path, az_android_package_env):
        """
        Resolve any splash screen overrides

        :param az_android_dst_path:     The destination android path (app project folder)
        :param az_android_package_env:  Dictionary of env values to retrieve override information
        """

        dst_resource_path = az_android_dst_path / 'src/main/res'

        splash_overrides = az_android_package_env['SPLASH_SCREEN']
        if not splash_overrides:
            return

        orientation = az_android_package_env['ORIENTATION']
        drawable_path_prefix = 'drawable-'

        for orientation_flag, orientation_key in ORIENTATION_FLAG_TO_KEY_MAP.items():
            orientation_path_prefix = drawable_path_prefix + orientation_key

            oriented_splashes = splash_overrides.get(orientation_key, {})

            unused_override_warning = None
            if (orientation & orientation_flag) == 0:
                unused_override_warning = f'Splash screen overrides specified for "{orientation_key}" when desired orientation ' \
                                          f'is set to "{ORIENTATION_FLAG_TO_KEY_MAP[orientation]}" in project {self.project_path}. ' \
                                          f'These overrides will be ignored.'

            # if a default splash image is specified for this orientation, then copy it into the generic drawable-<orientation> folder
            default_splash_img = oriented_splashes.get('default', None)

            if default_splash_img is not None:
                if unused_override_warning:
                    logging.warning(unused_override_warning)
                    continue

                src_default_splash_img_file = self.construct_source_resource_path(default_splash_img)

                dst_default_splash_img_dir = dst_resource_path / orientation_path_prefix
                dst_default_splash_img_dir.mkdir(parents=True, exist_ok=True)
                dst_default_splash_img_file = dst_default_splash_img_dir / APP_SPLASH_NAME

                shutil.copyfile(src_default_splash_img_file.resolve(), dst_default_splash_img_file.resolve())
                os.chmod(dst_default_splash_img_file.resolve(), stat.S_IWRITE | stat.S_IREAD)
            else:
                logging.debug(f'No default splash screen override specified for "%s" orientation in %s', orientation_key,
                              self.project_path)

            # process each of the resolution overrides
            warnings = []

            # The xxxhdpi resolution is only for application icons, its overkill to include them for drawables... for now
            valid_resolutions = set(ANDROID_RESOLUTION_SETTINGS)
            valid_resolutions.discard('xxxhdpi')

            for resolution in valid_resolutions:
                target_directory = dst_resource_path / f'{orientation_path_prefix}-{resolution}'

                # get the current resolution splash image override
                splash_img_source = oriented_splashes.get(resolution, default_splash_img)
                if splash_img_source is default_splash_img:

                    # if both the resolution and the default are unspecified, warn the user but do nothing
                    if splash_img_source is None:
                        section = f"{orientation_key}-{resolution}"
                        warnings.append(f'No splash screen override found for "{section}".  Either supply one for "{resolution}" '
                                        f'or a "default" in the android_settings "splash_screen-{orientation_key}" section of the '
                                        f'project.json file for {self.project_path}.')
                    else:
                        # if only the resolution is unspecified, remove the resolution specific version from the project
                        logging.debug(f'Default splash screen being used for "{orientation_key}-{resolution}" in {self.project_path}')
                        common.remove_dir_path(target_directory)
                    continue
                src_splash_img_file = self.construct_source_resource_path(splash_img_source)
                dst_splash_img_file = target_directory / APP_SPLASH_NAME

                shutil.copyfile(src_splash_img_file.resolve(), dst_splash_img_file.resolve())
                os.chmod(dst_splash_img_file.resolve(), stat.S_IWRITE | stat.S_IREAD)

            # guard against spamming warnings in the case the splash override block is full of garbage and no actual overrides
            if len(warnings) != len(valid_resolutions):
                if unused_override_warning:
                    logging.warning(unused_override_warning)
                else:
                    for warning_msg in warnings:
                        logging.warning(warning_msg)

    @staticmethod
    def clear_unused_assets(az_android_dst_path, az_android_package_env):
        """
        micro-optimization to clear assets from the final bundle that won't be used

        :param az_android_dst_path:     The destination android path (app project folder)
        :param az_android_package_env:  Dictionary of env values to retrieve override information
        """

        orientation = az_android_package_env['ORIENTATION']
        if orientation == ORIENTATION_LANDSCAPE:
            path_prefix = 'drawable-land'
        elif orientation == ORIENTATION_PORTRAIT:
            path_prefix = 'drawable-port'
        else:
            return

        # Prepare all the sub-folders to clear
        clear_folders = [path_prefix]
        clear_folders.extend([f'{path_prefix}-{resolution}' for resolution in ANDROID_RESOLUTION_SETTINGS if resolution != 'xxxhdpi'])

        # Clear out the base folder
        dst_resource_path = az_android_dst_path / 'src/main/res'

        for clear_folder in clear_folders:
            target_directory = dst_resource_path / clear_folder
            if target_directory.is_dir():
                logging.debug("Clearing folder %s", target_directory)
                common.remove_dir_path(target_directory)

    class _Library:
        """
        Library class to manage the library node in android_libraries.json
        """
        def __init__(self, name, path, overwrite_existing, signing_config=None):
            self.name = name
            self.path = path
            self.signing_config = signing_config
            self.overwrite_existing = overwrite_existing
            self.patch_files = []
            self.dependencies = []
            self.build_dependencies = []

        def add_file_to_patch(self, file):
            self.patch_files.append(file)

        def process_patch_lib(self, android_project_builder_path, dest_root):
            """
            Perform the patch logic on the library node of 'android_libraries.json' (root level)
            :param android_project_builder_path:    Path to the Android/ProjectBuilder path for the templates
            :param dest_root:                       The target android project folder
            """

            # Clear out any existing target path's src and recreate
            dst_path = dest_root / self.name

            dst_path_src = dst_path / 'src'
            if dst_path_src.exists():
                common.remove_dir_path(dst_path_src)
            dst_path.mkdir(parents=True, exist_ok=True)

            logging.debug("Copying library '{}' to '{}'".format(self.name, dst_path))

            # The folder structure from the base lib needs to be mapped to a structure that gradle can process as a
            # build project, and we need to generate some additional files

            # Generate the gradle build script for the library based on the build.gradle.in template file
            gradle_dependencies = []
            if self.build_dependencies:
                gradle_dependencies.extend([f"    api '{build_dependency}'" for build_dependency in self.build_dependencies])
            if self.dependencies:
                gradle_dependencies.extend([f"    api project(path: ':{dependency}')" for dependency in self.dependencies])
            if gradle_dependencies:
                project_dependencies = "dependencies {{\n{}\n}}".format('\n'.join(gradle_dependencies))
            else:
                project_dependencies = ""

            # Prepare an environment for a basic, no-native (cmake) gradle project (java only)
            build_gradle_env = {
                'PROJECT_DEPENDENCIES': project_dependencies,
                'TARGET_TYPE': 'library',
                'NATIVE_CMAKE_SECTION_DEFAULT_CONFIG': '',
                'NATIVE_CMAKE_SECTION_ANDROID': '',
                'NATIVE_CMAKE_SECTION_DEBUG_CONFIG': '',
                'NATIVE_CMAKE_SECTION_PROFILE_CONFIG': '',
                'NATIVE_CMAKE_SECTION_RELEASE_CONFIG': '',
                'OVERRIDE_JAVA_SOURCESET': '',
                'OPTIONAL_JNI_SRC_LIB_SET': '',

                'CUSTOM_APPLY_ASSET_LAYOUT_DEBUG_TASK': '',
                'CUSTOM_APPLY_ASSET_LAYOUT_PROFILE_TASK': '',
                'CUSTOM_APPLY_ASSET_LAYOUT_RELEASE_TASK': '',

                'CUSTOM_GRADLE_COPY_NATIVE_DEBUG_LIB_TASK': '',
                'CUSTOM_GRADLE_COPY_NATIVE_PROFILE_LIB_TASK': '',
                'CUSTOM_GRADLE_COPY_NATIVE_RELEASE_LIB_TASK': '',
                'SIGNING_CONFIGS': '',
                'SIGNING_DEBUG_CONFIG': '',
                'SIGNING_PROFILE_CONFIG': '',
                'SIGNING_RELEASE_CONFIG': ''
            }
            build_gradle_content = common.load_template_file(template_file_path=android_project_builder_path / 'build.gradle.in',
                                                             template_env=build_gradle_env)
            dest_gradle_script_file = dst_path / 'build.gradle'
            if not dest_gradle_script_file.exists() or self.overwrite_existing:
                dest_gradle_script_file.write_text(build_gradle_content,
                                                   encoding=common.DEFAULT_TEXT_WRITE_ENCODING,
                                                   errors=common.ENCODING_ERROR_HANDLINGS)

            src_path = pathlib.Path(self.path)

            # Prepare a 'src/main' folder
            dst_src_main_path = dst_path / 'src/main'
            dst_src_main_path.mkdir(parents=True, exist_ok=True)

            # Prepare a copy mapping list of tuples to process the copying of files and perform the straight file
            # copying
            library_copy_subfolder_pairs = [('res', 'res'),
                                            ('src', 'java')]

            for copy_subfolder_pair in library_copy_subfolder_pairs:

                src_subfolder = copy_subfolder_pair[0]
                dst_subfolder = copy_subfolder_pair[1]

                # {SRC}/{src_subfolder}/ -> {DST}/src/main/{dst_subfolder}/
                src_library_res_path = src_path / src_subfolder
                if not src_library_res_path.exists():
                    continue
                dst_library_res_path = dst_src_main_path / dst_subfolder
                shutil.copytree(src_library_res_path.resolve(),
                                dst_library_res_path.resolve(),
                                copy_function=shutil.copyfile)

            # Process the files identified for patching
            for file in self.patch_files:

                input_file_path = src_path / file.path
                if file.path == ANDROID_MANIFEST_FILE:
                    # Special case: AndroidManifest.xml does not go under the java/ parent path
                    output_file_path = dst_src_main_path / ANDROID_MANIFEST_FILE
                else:
                    output_subpath = f"java{file.path[3:]}"   # Strip out the 'src' from the library json and replace it with the target 'java' sub-path folder heading
                    output_file_path = dst_src_main_path / output_subpath

                logging.debug("  Patching file '%s'", os.path.basename(file.path))
                with open(input_file_path.resolve()) as input_file:
                    lines = input_file.readlines()

                with open(output_file_path.resolve(), 'w') as outFile:
                    for replace in file.changes:
                        lines[replace.line] = str.replace(lines[replace.line], replace.old,
                                                          (replace.new if replace.new else ""), 1)
                    outFile.write(''.join([line for line in lines if line]))

    class _File:
        """
        Helper class to manage individual files for each library (_Library) and their changes
        """
        def __init__(self, path):
            self.path = path
            self.changes = []

        def add_change(self, change):
            self.changes.append(change)

    class _Change:
        """
        Helper class to manage a change/patch as defined in android_libraries.json
        """
        def __init__(self, line, old, new):
            self.line = line
            self.old = old
            self.new = new


ANDROID_PLATFORM_PATTERN = re.compile(r'([\w\d]*-)?(\d+\d*)')   # Regex to handle android platform naming for both SDKs and NDKs


def validate_android_platform_input(input_android_platform, platform_variable_type, min_version, max_version):
    """
    Helper tool to support android platform number inputs and perform min/max version validation

    :param input_android_platform:  The inpuit argument to evaluate
    :param platform_variable_type: The type of platform version to validate (android sdk / android ndk)
    :param min_version:     The minimum version to validate against
    :param max_version:     The maximum version to validate against
    :return: The int version of the extracted platform number from the input
    """
    # Validate the platform number's format and against the supported versions
    platform_number_match = ANDROID_PLATFORM_PATTERN.search(input_android_platform)
    if not platform_number_match or not platform_number_match.group(2) or (platform_number_match.group(1) and platform_number_match.group(1) != 'android-'):
        raise common.LmbrCmdError(f"Invalid {platform_variable_type} version value ({input_android_platform}). It must be "
                                  f"either 'XX' or android-'XX' where 'XX' is a platform number.",
                                  common.ERROR_CODE_INVALID_PARAMETER)

    android_platform_number = int(platform_number_match.group(2))
    if android_platform_number < min_version:
        raise common.LmbrCmdError(f"Invalid {platform_variable_type} version value ({input_android_platform}) is less than the minimum "
                                  f"supported version ({min_version}).",
                                  common.ERROR_CODE_INVALID_PARAMETER)
    if android_platform_number > max_version:
        raise common.LmbrCmdError(f"Invalid {platform_variable_type} version value ({input_android_platform}) is greater than the maximum "
                                  f"supported version ({max_version}).",
                                  common.ERROR_CODE_INVALID_PARAMETER)
    return android_platform_number


ANDROID_SDK_ENV_NAME = 'ANDROID_SDK'
ANDROID_SDK_MIN_PLATFORM = 28
ANDROID_SDK_MAX_PLATFORM = 29


def verify_android_sdk(android_sdk_platform, argument_name, override_android_sdk_path=None, preferred_sdk_build_tools_ver=None):
    """
    Verify the android sdk and the requested platform platform against the android sdk path

    :param android_sdk_platform:            The android sdk platform to use (e.g. '28' or 'android-28')
    :param argument_name:                   The name of the argument for descriptive errors to present
    :param override_android_sdk_path:       The location of the android SDK path if not set through the environment variable
    :param preferred_sdk_build_tools_ver:   Option prefered built tool version under the android SDK if available. Will fallback to the first one discovered
    :returns    tuple of the verified android sdk platform number, path to the Android SDK path and the build tool version
    """
    android_sdk_platform_number = validate_android_platform_input(input_android_platform=android_sdk_platform,
                                                                  platform_variable_type='android sdk',
                                                                  min_version=ANDROID_SDK_MIN_PLATFORM,
                                                                  max_version=ANDROID_SDK_MAX_PLATFORM)

    # Get the candidate android sdk path from either the override argument or the system environment variable
    if override_android_sdk_path:
        check_android_sdk_path = override_android_sdk_path
    else:
        check_android_sdk_path = os.environ.get(ANDROID_SDK_ENV_NAME)
        if not check_android_sdk_path:
            raise common.LmbrCmdError(f"Android SDK path not set. Make sure that either the '{ANDROID_SDK_ENV_NAME}' environment is "
                                      f"set or it is passed in through the {argument_name} argument")

    # The android sdk folder structure is expected to have a 'platforms' sub folder based on the android sdk-platform number
    check_android_sdk_path = pathlib.Path(check_android_sdk_path)
    android_sdk_platforms_path = check_android_sdk_path / 'platforms'
    if not android_sdk_platforms_path.is_dir():
        raise common.LmbrCmdError(f"Invalid Android SDK path '{str(check_android_sdk_path)}': Missing 'platforms' directory.")

    # Collect the available platform numbers from the platforms subdirectory
    validated_android_platforms = []
    for dir_item in android_sdk_platforms_path.iterdir():
        if not dir_item.is_dir():
            continue
        check_file = dir_item / 'package.xml'
        if check_file.is_file():
            validated_android_platforms.append(dir_item.name)

    if not validated_android_platforms:
        raise common.LmbrCmdError(f"Invalid Android SDK path '{str(check_android_sdk_path)}': Unable to find any android platforms.")

    # Normalize the android_sdk argument to fit the same folder name pattern
    android_sdk_platform_name = f'android-{android_sdk_platform_number}'
    if android_sdk_platform_name not in validated_android_platforms:
        raise common.LmbrCmdError(f"Android SDK platform {android_sdk_platform_name} is not a valid for the android SDK located under '{str(check_android_sdk_path)}'")

    # Enumerate through the build tools under android sdk
    android_sdk_build_tools_dir = check_android_sdk_path / 'build-tools'
    if not android_sdk_build_tools_dir.is_dir():
        raise common.LmbrCmdError(f"Invalid Android SDK path '{str(check_android_sdk_path)}': Unable to find any built-tools folder.")
    supported_build_tools = [str(build_tool.name) for build_tool in android_sdk_build_tools_dir.iterdir() if build_tool.is_dir()]
    if not supported_build_tools:
        raise common.LmbrCmdError(f"Invalid Android SDK path '{str(check_android_sdk_path)}': Unable to find any built-tools.")
    if preferred_sdk_build_tools_ver:
        if preferred_sdk_build_tools_ver in supported_build_tools:
            validated_build_tool = preferred_sdk_build_tools_ver
        else:
            validated_build_tool = supported_build_tools[0]
            logging.warning("Unable to locate android sdk build tool version {preferred_sdk_build_tools_ver}. Defaulting to version {validated_build_tool}")

    else:
        validated_build_tool = supported_build_tools[0]

    return android_sdk_platform_number, check_android_sdk_path, validated_build_tool


ANDROID_NDK_ENV_NAME = 'ANDROID_NDK'
ANDROID_NDK_MIN_PLATFORM = 21
ANDROID_NDK_MAX_PLATFORM = 29
ANDROID_NDK_SOURCE_PROPERTIES_REVISION_PATTERN = re.compile(r'Pkg.Revision\s*=\s*(\d+.\d+.\d+)')


def verify_android_ndk(android_ndk_platform, argument_name, override_android_ndk_path=None):
    """
    Verify the android ndk and requested platform against the android ndk path

    :param android_ndk_platform:        The android ndk platform to use (e.g. '21' or 'android-21')
    :param argument_name:               The name of the argument for descriptive errors to present
    :param override_android_ndk_path:   The location of the android NDK path if not set through the environment variable
    :returns    tuple of the verified android ndk platform number and the Path to the Android SDK path and the
    """

    android_ndk_platform_number = validate_android_platform_input(input_android_platform=android_ndk_platform,
                                                                  platform_variable_type='android ndk',
                                                                  min_version=ANDROID_NDK_MIN_PLATFORM,
                                                                  max_version=ANDROID_NDK_MAX_PLATFORM)

    # Get the candidate android ndk path from either the override argument or the system environment variable
    if override_android_ndk_path:
        check_android_ndk_path = str(override_android_ndk_path)
    else:
        check_android_ndk_path = os.environ.get(ANDROID_NDK_ENV_NAME)
        if not check_android_ndk_path:
            raise common.LmbrCmdError(f"Android NDK path not set. Make sure that either the {ANDROID_NDK_ENV_NAME} environment "
                                      f"is set or it is passed in through the {argument_name} argument")
    check_android_ndk_path = pathlib.Path(check_android_ndk_path)

    # Validate the android ndk path

    # Determine the NDK revision by reading the source.properties file
    ndk_source_properties_file = check_android_ndk_path / 'source.properties'
    if not ndk_source_properties_file.is_file():
        raise common.LmbrCmdError(f"Invalid Android NDK path '{str(check_android_ndk_path)}'. Missing 'source.properties' file.",
                                  common.ERROR_CODE_INVALID_PARAMETER)
    ndk_source_properties_file_content = ndk_source_properties_file.read_text(encoding=common.DEFAULT_TEXT_READ_ENCODING,
                                                                              errors=common.ENCODING_ERROR_HANDLINGS)

    ndk_revision_match = ANDROID_NDK_SOURCE_PROPERTIES_REVISION_PATTERN.search(ndk_source_properties_file_content)
    if not ndk_revision_match:
        raise common.LmbrCmdError(f"Invalid Android NDK path '{str(check_android_ndk_path)}'. Unable to extract version from 'source.properties' file.",
                                  common.ERROR_CODE_INVALID_PARAMETER)
    ndk_revision_number = LooseVersion(ndk_revision_match.group(1))
    logging.info(f"Detected Android NDK Revision {str(ndk_revision_number)}")

    # Collect the supported android platforms from the required 'platforms' folder under the ndk path
    android_ndk_platforms_path = check_android_ndk_path / 'platforms'
    if not android_ndk_platforms_path.is_dir():
        raise common.LmbrCmdError(f"Invalid Android NDK path '{str(check_android_ndk_path)}'. Missing 'platforms' folder.",
                                  common.ERROR_CODE_INVALID_PARAMETER)

    validated_android_platforms = []
    for dir_item in android_ndk_platforms_path.iterdir():
        if not dir_item.is_dir():
            continue
        api_version_match = ANDROID_PLATFORM_PATTERN.search(dir_item.name)
        if not api_version_match or api_version_match.group(1) != 'android-':
            continue

        check_lib_path = dir_item / 'arch-arm64/usr/lib'
        if check_lib_path.is_dir():
            validated_android_platforms.append(dir_item.name)

    # For NDK revisions 19 and up, there is a mapping file for version numbers that map to other version.
    platforms_map_aliases = {}
    if ndk_revision_number >= LooseVersion('19.0.0'):
        platforms_map_file = check_android_ndk_path / 'meta/platforms.json'
        if platforms_map_file.exists():
            with open(platforms_map_file, 'r') as platforms_map_file_handle:
                platforms_map_file_json = json.load(platforms_map_file_handle)
            platforms_map_aliases = platforms_map_file_json['aliases']
    elif validated_android_platforms:
        # Revisions before 19 does not have a mapping file for API versions, they fall back to the previous one
        # So we need to make a mapping file that does the same
        platforms_map_aliases = {}
        validated_android_platforms.sort()
        max_supported_api_number = int(ANDROID_PLATFORM_PATTERN.search(validated_android_platforms[-1]).group(2))
        for validated_android_platform in validated_android_platforms:
            current_api_version = int(ANDROID_PLATFORM_PATTERN.search(validated_android_platform).group(2))
            next_api_version = current_api_version + 1
            while f'android-{next_api_version}' not in validated_android_platforms and next_api_version <= max_supported_api_number:
                platforms_map_aliases[str(next_api_version)] = current_api_version
                next_api_version += 1

    # Go through the aliases and add to the validated platforms if it is mapped to an existing platform
    for alias_key, alias_value in platforms_map_aliases.items():
        if not ANDROID_PLATFORM_PATTERN.search(f'android-{alias_key}'):
            # Skip any non android-XX (XX = number) aliases
            continue
        aliased_platform_key = f'android-{alias_value}'
        if aliased_platform_key in validated_android_platforms:
            validated_android_platforms.append(f'android-{alias_key}')

    if not validated_android_platforms:
        raise common.LmbrCmdError(f"Invalid Android NDK path {str(check_android_ndk_path)}")

    # Verify the ndk platform against the ndk path
    android_ndk_platform_name = f'android-{android_ndk_platform_number}'
    if android_ndk_platform_name not in validated_android_platforms:
        raise common.LmbrCmdError(f"Android NDK platform {android_ndk_platform_name} is not a valid for the Android NDK located under '{str(check_android_ndk_path)}'")

    return android_ndk_platform_number, check_android_ndk_path


ADB_TARGET = 'adb.exe' if platform.system() == 'Windows' else 'adb'


def resolve_adb_tool(android_sdk_path):
    """
    Resolve the location of the adb tool based on the input Android SDK Path
    :param android_sdk_path: The android SDK path to search for the adb tool
    :return: The absolute path to the adb tool
    """

    if isinstance(android_sdk_path, str):
        android_sdk_path = pathlib.Path(android_sdk_path)

    check_adb_target = android_sdk_path / 'platform-tools' / ADB_TARGET
    if not check_adb_target.exists():
        raise common.LmbrCmdError(f"Invalid Android SDK path '{str(android_sdk_path)}': Unable to locate '{ADB_TARGET}'.")
    return check_adb_target


class AdbTool(common.CommandLineExec):
    """
    Custom ADB command line processor
    """

    def __init__(self, android_sdk_path):

        check_adb_target = resolve_adb_tool(android_sdk_path)

        super().__init__(str(check_adb_target))

        self.is_connected = False
        self.device_filter = None

    def get_connected_device_serial_ids(self):
        """
        Get the connected android device serial numbers through adb
        :return: List of device serial numbers of android devices currently connected
        """
        ret, devices_result, _ = super().exec(['devices'], capture_stdout=True)
        if ret != 0:
            raise common.LmbrCmdError("Unable to get device list from adb")

        connected_device_serials = []
        device_result_lines = [device_result_line.strip() for device_result_line in devices_result.split('\n') if
                               device_result_line]
        for device_result_line in device_result_lines:
            match_result = re.match(r'([\w-]+)\s+(device)', device_result_line)
            if match_result:
                device_id = match_result.group(1)
                connected_device_serials.append(device_id)
        return connected_device_serials

    def connect(self, device_filter=None):
        """
        Start the adb server
        :param device_filter:   Any device to filter subsequent commands to in situations where there may be multiple devices connected
        """

        if self.is_connected:
            raise common.LmbrCmdError("Adb connection already started")

        super().exec(['start-server'])

        if device_filter:
            # If a device serial id was passed in, then verify if its valid
            device_matched = False
            connected_serial_ids = self.get_connected_device_serial_ids()
            for connected_serial_id in connected_serial_ids:
                if device_filter == connected_serial_id:
                    device_matched = True
                    break

            if not device_matched:
                raise common.LmbrCmdError(f"Invalid device serial {device_filter}. The current connected device serial ids are : {','.join(connected_serial_ids)}")

            self.device_filter = device_filter

        self.is_connected = True

    def disconnect(self):
        """
        Stop the adb server
        """

        super().exec(['kill-server'])
        self.is_connected = False
        self.device_filter = None

    def exec(self, arguments, capture_stdout=False, cwd=None):
        """
        Wrapper to the base 'exec' call which may append an optional device filter to the adb calls

        :param arguments:       'arguments' to pass to the base exec
        :param capture_stdout:  'capture_stdout' to pass to the base exec
        :param cwd:             'cwd'  to pass to the base exec
        :return: Result of the call (see common.CommandLineExec.exec)
        """
        if self.device_filter:
            adb_params = ['-s', self.device_filter]
            adb_params.extend(arguments)
        else:
            adb_params = arguments
        return super().exec(adb_params, capture_stdout, cwd)

    def popen(self, arguments, cwd=None):
        """
        Wrapper to the base 'popen' call which may append an optional device filter to the adb calls

        :param arguments:       'arguments' to pass to the base popen
        :param cwd:             'cwd'  to pass to the base exec
        :return: Result of the call (see common.CommandLineExec.popen)
        """
        if self.device_filter:
            adb_params = ['-s', self.device_filter]
            adb_params.extend(arguments)
        else:
            adb_params = arguments
        return super().popen(adb_params, cwd)
