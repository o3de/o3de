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

from Params import Params
from utils.util import *


class PackageEnv(Params):
    def __init__(self, target_platform, json_file):
        super(PackageEnv, self).__init__()
        self.__cur_dir = os.path.dirname(os.path.abspath(__file__))
        with open(json_file, 'r') as source:
            self.__data = json.load(source)
            self.__platforms = self.__data.get('platforms')
            if target_platform not in self.__platforms:
                ly_build_error('Target platform {} is not supported'.format(target_platform))
            self.__target_platform = target_platform

            # visited_platform is used to track the platform reference chain, in order to avoid chain cycle.
            visited_platform = [target_platform]
            platform_env = self.__platforms.get(target_platform)
            # If platform_env starts with @, it means that platform_env references another platform
            while isinstance(platform_env, str) and platform_env.startswith('@'):
                referenced_platform = platform_env.lstrip('@')
                if referenced_platform in visited_platform:
                    ly_build_error('Found reference chain cycle started from {}.\nSee {}'.format(referenced_platform, json_file))
                visited_platform.append(referenced_platform)
                platform_env = self.__platforms.get(referenced_platform)

            self.__platform_env = platform_env
            self.__global_env = self.__data.get('global')

    def get_target_platform(self):
        return self.__target_platform

    def get_global_env(self):
        return self.__global_env

    def get_platform_env(self):
        return self.__platform_env

    def __get_global_value(self, key):
        key = key.upper()
        value = self.__global_env.get(key)
        if value is None:
            ly_build_error('{} is not defined in global env'.format(key))
        return value

    def __get_platform_value(self, key):
        key = key.upper()
        value = self.__platform_env.get(key)
        if value is None:
            ly_build_error('{} is not defined in platform env for {}'.format(key, self.__target_platform))
        return value

    def __evaluate_boolean(self, v):
        return str(v).lower() in ['1', 'true']

    def __get_engine_root(self):
        def validate_engine_root(engine_root):
            if not os.path.isdir(engine_root):
                return False
            return os.path.exists(os.path.join(engine_root, 'engineroot.txt'))

        # Jenkins only
        workspace = os.getenv('WORKSPACE')
        if workspace is not None:
            print('Environment variable WORKSPACE={} detected'.format(workspace))
            if validate_engine_root(workspace):
                print('Setting ENGINE_ROOT to {}'.format(workspace))
                return workspace
            engine_root = os.path.join(workspace, 'dev')
            if validate_engine_root(engine_root):
                print('Setting ENGINE_ROOT to {}'.format(engine_root))
                return engine_root
            print('Cannot locate ENGINE_ROOT with Environment variable WORKSPACE')
            # End Jenkins only
            
        engine_root = os.getenv('ENGINE_ROOT', '')
        if validate_engine_root(engine_root):
            return engine_root

        print('Environment variable ENGINE_ROOT is not set or invalid, checking ENGINE_ROOT in env json file')
        engine_root = self.__global_env.get('ENGINE_ROOT')
        if validate_engine_root(engine_root):
            return engine_root

        # Set engine_root based on script location
        engine_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(self.__cur_dir))))
        print('ENGINE_ROOT from env json file is invalid, defaulting to {}'.format(engine_root))
        if validate_engine_root(engine_root):
            return engine_root
        else:
            error('Cannot Locate ENGINE_ROOT')

    def __get_thirdparty_home(self):
        third_party_home = os.getenv('ENV_3RDPARTY_PATH', '')
        if os.path.exists(third_party_home):
            print('ENV_3RDPARTY_PATH found, using {} as 3rdParty path.'.format(third_party_home))
            return third_party_home
        third_party_home = self.__get_global_value('THIRDPARTY_HOME')
        if os.path.isdir(third_party_home):
            return third_party_home

        # Set engine_root based on script location
        print('THIRDPARTY_HOME is not valid, looking for THIRD_PARTY_HOME')

        # Finding THIRD_PARTY_HOME
        cur_dir = self.__get_engine_root()
        last_dir = None
        while last_dir != cur_dir:
            third_party_home = os.path.join(cur_dir, '3rdParty')
            print('Cheking THIRDPARTY_HOME {}'.format(third_party_home))
            if os.path.exists(os.path.join(third_party_home, '3rdParty.txt')):
                print('Setting THIRDPARTY_HOME to {}'.format(third_party_home))
                return third_party_home
            last_dir = cur_dir
            cur_dir = os.path.dirname(cur_dir)
        error('Cannot locate THIRDPARTY_HOME')

    def __get_package_name_pattern(self):
        package_name_pattern = self.__get_global_value('PACKAGE_NAME_PATTERN')
        if os.getenv('PACKAGE_NAME_PATTERN') is not None:
            package_name_pattern = os.getenv('PACKAGE_NAME_PATTERN')
        return package_name_pattern

    def __get_build_number(self):
        build_number = self.__get_global_value('BUILD_NUMBER')
        if os.getenv('BUILD_NUMBER') is not None:
            build_number = os.getenv('BUILD_NUMBER')
        return build_number

    def __get_p4_changelist(self):
        p4_changelist = self.__get_global_value('P4_CHANGELIST')
        if os.getenv('P4_CHANGELIST') is not None:
            p4_changelist = os.getenv('P4_CHANGELIST')
        return p4_changelist

    def __get_major_version(self):
        major_version = self.__get_global_value('MAJOR_VERSION')
        if os.getenv('MAJOR_VERSION') is not None:
            major_version = os.getenv('MAJOR_VERSION')
        return major_version

    def __get_minor_version(self):
        minor_version = self.__get_global_value('MINOR_VERSION')
        if os.getenv('MINOR_VERSION') is not None:
            minor_version = os.getenv('MINOR_VERSION')
        return minor_version

    def __get_scrub_params(self):
        return self.__get_platform_value('SCRUB_PARAMS')

    def __get_validator_platforms(self):
        return self.__get_platform_value('VALIDATOR_PLATFORMS')

    def __get_package_targets(self):
        return self.__get_platform_value('PACKAGE_TARGETS')

    def __get_build_targets(self):
        return self.__get_platform_value('BUILD_TARGETS')

    def __get_asset_processor_path(self):
        return self.__get_platform_value('ASSET_PROCESSOR_PATH')

    def __get_asset_game_folders(self):
        return self.__get_platform_value('ASSET_GAME_FOLDERS')

    def __get_asset_platform(self):
        return self.__get_platform_value('ASSET_PLATFORM')

    def __get_bootstrap_cfg_game_folder(self):
        return self.__get_platform_value('BOOTSTRAP_CFG_GAME_FOLDER')

    def __get_run_launcher_unit_test(self):
        run_launcher_unit_test = os.getenv('RUN_LAUNCHER_UNIT_TEST')
        if run_launcher_unit_test is None:
            run_launcher_unit_test = self.__platform_env.get('RUN_LAUNCHER_UNIT_TEST')
        return self.__evaluate_boolean(run_launcher_unit_test)

    def __get_skip_build(self):
        skip_build = os.getenv('SKIP_BUILD')
        if skip_build is None:
            skip_build = self.__platform_env.get('SKIP_BUILD')
        return self.__evaluate_boolean(skip_build)