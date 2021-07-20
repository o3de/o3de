#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import logging
import os
import pathlib
import sys

# Resolve the common python module
ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common
from cmake.Tools.Platform.Android import android_deployment

DEPLOY_TYPES = (android_deployment.AndroidDeployment.DEPLOY_APK_ONLY,
                android_deployment.AndroidDeployment.DEPLOY_ASSETS_ONLY,
                android_deployment.AndroidDeployment.DEPLOY_BOTH)


def validate_android_deployment_arguments(build_dir_name):
    """
    Validate the minimal platform deployment arguments

    @param build_dir_name:  The name of the build directory relative to the current working directory
    @param game_name:       The name of the game project to deploy
    @return: Tuple of (resolved pathlib, game name, asset mode, asset_type, platform_settings object, Android SDK path, embedded_assets (bool) )
    """

    build_dir = pathlib.Path(os.getcwd()) / build_dir_name
    if not build_dir.is_dir():
        raise common.LmbrCmdError(f"Invalid build directory {build_dir_name}")

    platform_settings = common.PlatformSettings(build_dir)

    if not platform_settings.projects:
        raise common.LmbrCmdError("Missing required platform settings object from build directory.")
    game_name = platform_settings.projects[0]

    android_sdk_path = getattr(platform_settings, 'android_sdk_path', None)
    if not android_sdk_path:
        raise common.LmbrCmdError(f"Android SDK Path {android_sdk_path} is missing in the platform settings for {build_dir_name}.")
    if not os.path.isdir(android_sdk_path):
        raise common.LmbrCmdError(f"Android SDK Path {android_sdk_path} in the platform settings for {build_dir_name} is not valid.")

    embedded_assets_str = getattr(platform_settings, 'embed_assets_in_apk', None)
    if not embedded_assets_str:
        raise common.LmbrCmdError(f"The emdedded assets flag 'embed_assets_in_apk'  is missing in the platform settings for {build_dir_name}.")

    embedded_assets = embedded_assets_str.lower() in ('t', 'true', '1')

    is_unit_test_str = getattr(platform_settings, 'is_unit_test', 'False')
    is_unit_test = is_unit_test_str.lower() in ('t', 'true', '1')

    return build_dir, game_name, platform_settings.asset_deploy_mode, platform_settings.asset_deploy_type, android_sdk_path, embedded_assets, is_unit_test


def main(args):
    parser = argparse.ArgumentParser()

    parser.add_argument('-b', '--build-dir',
                        help='The relative build directory to deploy from.',
                        required=True)

    parser.add_argument('-c', '--configuration',
                        help='The build configuration from the build directory for the source deployment files',
                        default='profile')

    parser.add_argument('--device-id-filter',
                        help='Comma separated list of connected android device IDs to filter the deployment to. If not supplied, no filter will be applied and deployment will occur on all devices.',
                        default='')

    parser.add_argument('-t', '--deployment-type',
                        help=f'The deployment type ({"|".join(DEPLOY_TYPES)}) to execute.',
                        choices=DEPLOY_TYPES,
                        default=android_deployment.AndroidDeployment.DEPLOY_BOTH)

    parser.add_argument('--clean',
                        help='Option to clean the target dev kit before deploying, ensuring a clean installation.',
                        action='store_true')

    parser.add_argument('--debug',
                        help='Option to enable debug messages',
                        action='store_true')

    parsed_args = parser.parse_args(args)

    # Prepare the logging
    logging.basicConfig(format='%(levelname)s: %(message)s',
                        level=logging.DEBUG if parsed_args.debug else logging.INFO)

    build_dir, game_project, asset_mode, asset_type,  android_sdk_path, embedded_assets, is_unit_test = validate_android_deployment_arguments(build_dir_name=parsed_args.build_dir)

    deployment = android_deployment.AndroidDeployment(dev_root=ROOT_DEV_PATH,
                                                      build_dir=build_dir,
                                                      configuration=parsed_args.configuration,
                                                      game_name=game_project,
                                                      asset_mode=asset_mode,
                                                      asset_type=asset_type,
                                                      embedded_assets=embedded_assets,
                                                      android_device_filter=parsed_args.device_id_filter,
                                                      clean_deploy=parsed_args.clean,
                                                      android_sdk_path=android_sdk_path,
                                                      deployment_type=parsed_args.deployment_type,
                                                      is_unit_test=is_unit_test)

    deployment.execute()


if __name__ == '__main__':

    try:
        main(sys.argv[1:])
        exit(0)

    except common.LmbrCmdError as err:
        print(str(err), file=sys.stderr)
        exit(err.code)

