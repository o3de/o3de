#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

from Params import Params
from util import *


class PackageEnv(Params):
    def __init__(self, platform, type, json_file):
        super(PackageEnv, self).__init__()
        self.__cur_dir = os.path.dirname(os.path.abspath(__file__))
        global_env_file = os.path.join(self.__cur_dir, json_file)
        with open(global_env_file, 'r') as source:
            data = json.load(source)
            self.__global_env = data.get('global_env')
        platform_env_file = os.path.join(self.__cur_dir, 'Platform', platform, json_file)
        if not os.path.exists(platform_env_file):
            print(f'{platform_env_file} is not found.')
            # Search restricted platform folders
            engine_root = self.get('ENGINE_ROOT')
            # Use real path in case engine root is a symlink path
            if os.name == 'posix' and os.path.islink(engine_root):
                engine_root = os.readlink(engine_root)
            rel_path = os.path.relpath(self.__cur_dir, engine_root)
            platform_env_file = os.path.join(engine_root, 'restricted', platform, rel_path, json_file)
            if not os.path.exists(platform_env_file):
                ly_build_error(f'{platform_env_file} is not found.')
        with open(platform_env_file, 'r') as source:
            data = json.load(source)
            types = data.get('types')
            if type not in types:
                ly_build_error(f'Package type {type} is not supported')
            self.__platform = platform
            self.__platform_env = data.get('local_env')
            self.__platform_env.update(self.__global_env)
            self.__type = type
            self.__type_env = types.get(type)

    def get_platform(self):
        return self.__platform

    def get_type(self):
        return self.__type

    def get_platform_env(self):
        return self.__platform_env

    def get_type_env(self):
        return self.__type_env

    def __get_platform_value(self, key):
        key = key.upper()
        value = self.__platform_env.get(key)
        if value is None:
            ly_build_error(f'{key} is not defined in global env nor in local env')
        return value

    def __get_type_value(self, key):
        key = key.upper()
        value = self.__type_env.get(key)
        if value is None:
            ly_build_error(f'{key} is not defined in package type {self.__type} for platform {self.__platform}')
        return value

    def __evaluate_boolean(self, v):
        return str(v).lower() in ['1', 'true']

    def __get_engine_root(self):
        def validate_engine_root(engine_root):
            if not os.path.isdir(engine_root):
                return False
            return os.path.exists(os.path.join(engine_root, 'engine.json'))

        workspace = os.getenv('WORKSPACE')
        if workspace is not None:
            print(f'Environment variable WORKSPACE={workspace} detected')
            if validate_engine_root(workspace):
                print(f'Setting ENGINE_ROOT to {workspace}')
                return workspace
            print('Cannot locate ENGINE_ROOT with Environment variable WORKSPACE')

        engine_root = os.getenv('ENGINE_ROOT', '')
        if validate_engine_root(engine_root):
            return engine_root

        print('Environment variable ENGINE_ROOT is not set or invalid, checking ENGINE_ROOT in env json file')
        engine_root = self.__global_env.get('ENGINE_ROOT')
        if validate_engine_root(engine_root):
            return engine_root

        # Set engine_root based on script location
        engine_root = os.path.dirname(os.path.dirname(os.path.dirname(self.__cur_dir)))
        print(f'ENGINE_ROOT from env json file is invalid, defaulting to {engine_root}')
        if validate_engine_root(engine_root):
            return engine_root
        else:
            error('Cannot Locate ENGINE_ROOT')

    def __get_thirdparty_home(self):
        third_party_home = os.getenv('LY_3RDPARTY_PATH', '')
        if os.path.exists(third_party_home):
            print(f'LY_3RDPARTY_PATH found, using {third_party_home} as 3rdParty path.')
            return third_party_home
        third_party_home = self.__get_platform_value('THIRDPARTY_HOME')
        if os.path.isdir(third_party_home):
            return third_party_home

        # Set engine_root based on script location
        print('THIRDPARTY_HOME is not valid, looking for THIRD_PARTY_HOME')

        # Finding THIRD_PARTY_HOME
        cur_dir = self.__get_engine_root()
        last_dir = None
        while last_dir != cur_dir:
            third_party_home = os.path.join(cur_dir, '3rdParty')
            print(f'Cheking THIRDPARTY_HOME {third_party_home}')
            if os.path.exists(os.path.join(third_party_home, '3rdParty.txt')):
                print(f'Setting THIRDPARTY_HOME to {third_party_home}')
                return third_party_home
            last_dir = cur_dir
            cur_dir = os.path.dirname(cur_dir)
        error('Cannot locate THIRDPARTY_HOME')

    def __get_package_name_pattern(self):
        package_name_pattern = self.__get_platform_value('PACKAGE_NAME_PATTERN')
        if os.getenv('PACKAGE_NAME_PATTERN') is not None:
            package_name_pattern = os.getenv('PACKAGE_NAME_PATTERN')
        return package_name_pattern

    def __get_branch_name(self):
        branch_name = self.__get_platform_value('BRANCH_NAME')
        if os.getenv('BRANCH_NAME') is not None:
            branch_name = os.getenv('BRANCH_NAME')
        branch_name = branch_name.replace('/', '_').replace('\\', '_')
        return branch_name

    def __get_build_number(self):
        build_number = self.__get_platform_value('BUILD_NUMBER')
        if os.getenv('BUILD_NUMBER') is not None:
            build_number = os.getenv('BUILD_NUMBER')
        return build_number

    def __get_scrub_params(self):
        return self.__get_type_value('SCRUB_PARAMS')

    def __get_validator_platforms(self):
        return self.__get_type_value('VALIDATOR_PLATFORMS')

    def __get_package_targets(self):
        return self.__get_type_value('PACKAGE_TARGETS')

    def __get_build_targets(self):
        return self.__get_type_value('BUILD_TARGETS')

    def __get_asset_processor_path(self):
        return self.__get_type_value('ASSET_PROCESSOR_PATH')

    def __get_asset_game_folders(self):
        return self.__get_type_value('ASSET_GAME_FOLDERS')

    def __get_asset_platform(self):
        return self.__get_type_value('ASSET_PLATFORM')

    def __get_bootstrap_cfg_game_folder(self):
        return self.__get_type_value('BOOTSTRAP_CFG_GAME_FOLDER')

    def __get_skip_build(self):
        skip_build = os.getenv('SKIP_BUILD')
        if skip_build is None:
            skip_build = self.__get_type_value('SKIP_BUILD')
        return self.__evaluate_boolean(skip_build)

    def __get_skip_scrubbing(self):
        skip_scrubbing = os.getenv('SKIP_SCRUBBING')
        if skip_scrubbing is None:
            skip_scrubbing = self.__type_env.get('SKIP_SCRUBBING', 'False')
        return self.__evaluate_boolean(skip_scrubbing)

    def __get_internal_s3_bucket(self):
        return self.__get_platform_value('INTERNAL_S3_BUCKET')

    def __get_qa_s3_bucket(self):
        return self.__get_platform_value('QA_S3_BUCKET')

    def __get_s3_prefix(self):
        return self.__get_platform_value('S3_PREFIX')
