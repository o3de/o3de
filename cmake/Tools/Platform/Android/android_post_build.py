#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import re
import shutil
import sys

import platform
import logging

from packaging.version import Version
from pathlib import Path

logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.INFO)
logger = logging.getLogger('o3de.android')

ANDROID_ARCH = 'arm64-v8a'

ASSET_MODE_PAK     = 'PAK'
ASSET_MODE_LOOSE   = 'LOOSE'
ASSET_MODE_NONE    = 'NONE'
SUPPORTED_ASSET_MODES = [ASSET_MODE_PAK, ASSET_MODE_LOOSE, ASSET_MODE_NONE]
ASSET_PLATFORM_KEY = 'android'

SUPPORTED_BUILD_CONFIGS = ['debug', 'profile', 'release']

MINIMUM_ANDROID_GRADLE_PLUGIN_VER = Version("4.2")

IS_PLATFORM_WINDOWS = platform.system() == 'Windows'

PAK_FILE_INSTRUCTIONS = "Make sure to create release bundles (Pak files) before building and deploying to an android device. Refer to " \
                        "https://www.docs.o3de.org/docs/user-guide/packaging/asset-bundler/bundle-assets-for-release/ for more" \
                        "information."


class AndroidPostBuildError(Exception):
    pass


def create_link(src: Path, tgt: Path):
    """
    Create a link/junction depending on the source type. If this is a file, then perform file copy from the
    source to the target. If it is a directory, then create a junction(windows) or symlink(linux/mac) from the source to
    the target

    :param src: The source to link from
    :param tgt: The target tp link to
    """

    assert src.exists()
    assert not tgt.exists()

    try:
        if src.is_file():
            tgt.symlink_to(src)
            logger.info(f"Created symbolic link {str(tgt)} => {str(src)}")
        else:
            if IS_PLATFORM_WINDOWS:
                import _winapi
                _winapi.CreateJunction(str(src.resolve().absolute()), str(tgt.resolve().absolute()))
                logger.info(f'Created Junction {str(tgt)} => {str(src)}')
            else:
                tgt.symlink_to(src, target_is_directory=True)
                logger.info(f'Created symbolic link {str(tgt)} => {str(src)}')

    except OSError as os_err:
        raise AndroidPostBuildError(f"Error trying to link  {tgt} => {src} : {os_err}")

# deprecated: The functionality has been replaced with a single call to remove_link_to_directory()
# Go to commit e302882 to get this functionality back.
# def safe_clear_folder(target_folder: Path) -> None:

def remove_link_to_directory(link_to_directory: Path) -> None:
    """
    Removes a Symbolic Link or Junction(Windows) that points to a directory
    Throws an exception if the link exists, and it points to a directory and could not be deleted.

    :param link_to_directory: The symbolic link or junction which should point to a directory.
    """
    if link_to_directory.is_dir():
        try:
            link_to_directory.unlink()
        except OSError as os_err:
            raise AndroidPostBuildError(f"Error trying to unlink/delete {link_to_directory}: {os_err}")


# deprecated: The functionality has been replaced with a single call to create_link()
# Go to commit e302882 to get this functionality back.
# def synchronize_folders(src: Path, tgt: Path) -> None:


def apply_pak_layout(project_root: Path, asset_bundle_folder: str, target_layout_root: Path) -> None:
    """
    Apply the pak folder layout to the target assets folder

    :param project_root:            The project root folder to base the search for the location of the pak files (Bundle)
    :param asset_bundle_folder:     The sub path within the project root folder of the location of the pak files
    :param target_layout_root:      The target layout destination of the pak files
    """

    src_pak_file_full_path = project_root / asset_bundle_folder

    # Make sure that the source bundle folder where we look up the paks exist
    if not src_pak_file_full_path.is_dir():
        raise AndroidPostBuildError(f"Pak files are expected at location {src_pak_file_full_path}, but the folder doesnt exist. {PAK_FILE_INSTRUCTIONS}")

    # Make sure that we have at least the engine_android.pak file
    has_engine_android_pak = False
    for pak_dir_item in src_pak_file_full_path.iterdir():
        if pak_dir_item.is_file and str(pak_dir_item.name).lower() == f'engine_{ASSET_PLATFORM_KEY}.pak':
            has_engine_android_pak = True
            break
    if not has_engine_android_pak:
        raise AndroidPostBuildError(f"Unable to located the required 'engine_android.pak' file at location specified at {src_pak_file_full_path}. "
                                    f"{PAK_FILE_INSTRUCTIONS}")

    # Remove the link to the source assets folder, if it exists.
    remove_link_to_directory(target_layout_root)

    # Create the link to the asset bundle folder.
    create_link(src_pak_file_full_path, target_layout_root)


def apply_loose_layout(project_root: Path, target_layout_root: Path) -> None:
    """
    Apply the loose assets folder layout rules to the target assets folder

    :param project_root:            The project folder root to look for the loose assets
    :param target_layout_root:      The target layout destination of the loose assets
    """

    android_cache_folder = project_root / 'Cache' / ASSET_PLATFORM_KEY
    engine_json_marker = android_cache_folder / 'engine.json'
    if not engine_json_marker.is_file():
        raise AndroidPostBuildError(f"Assets have not been built for this project at ({project_root}) yet. "
                                    f"Please run the AssetProcessor for this project first.")

    # Remove the link to the source assets folder, if it exists.
    remove_link_to_directory(target_layout_root)


    # Create a symlink or junction {target_layout_root} to this
    # project Cache folder for the android platform {android_cache_folder}.
    create_link(android_cache_folder, target_layout_root)


def post_build_action(android_app_root: Path, project_root: Path, gradle_version: Version,
                      asset_mode: str, asset_bundle_folder: str):
    """
    Perform the post-build logic for android native builds that will prepare the output folders by laying out the asset files
    to their locations before the APK is generated.

    :param android_app_root:    The root path of the 'app' project within the Android Gradle build script
    :param project_root:        The root of the project that the APK is being built for
    :param gradle_version:      The version of Gradle used to build the APK (for validation)
    :param asset_mode:          The desired asset mode to determine the layout rules
    :param asset_bundle_folder: (For PAK asset modes) the location of where the PAK files are expected.
    """

    if not android_app_root.is_dir():
        raise AndroidPostBuildError(f"Invalid Android Gradle build path: {android_app_root} is not a directory or does not exist.")

    if gradle_version < MINIMUM_ANDROID_GRADLE_PLUGIN_VER:
        raise AndroidPostBuildError(f"Android gradle plugin versions below version {MINIMUM_ANDROID_GRADLE_PLUGIN_VER} is not supported.")
    logger.info(f"Applying post-build for android gradle plugin version {gradle_version}")

    # Validate the build directory exists
    app_build_root = android_app_root / 'build'
    if not app_build_root.is_dir():
        raise AndroidPostBuildError(f"Android gradle build path: {app_build_root} is not a directory or does not exist.")

    target_layout_root = android_app_root / 'src' / 'main' / 'assets'

    if asset_mode == ASSET_MODE_LOOSE:

        apply_loose_layout(project_root=project_root,
                           target_layout_root=target_layout_root)

    elif asset_mode == ASSET_MODE_PAK:

        apply_pak_layout(project_root=project_root,
                         target_layout_root=target_layout_root,
                         asset_bundle_folder=asset_bundle_folder)
    elif asset_mode == ASSET_MODE_NONE:
        # Skip any asset layout
        pass
    else:
        raise AndroidPostBuildError(f"Invalid Asset Mode '{asset_mode}'.")


if __name__ == '__main__':

    try:
        parser = argparse.ArgumentParser(description="Android post Gradle build step handler")
        parser.add_argument('android_app_root', type=str, help="The base of the 'app' in the O3DE generated gradle script.")
        parser.add_argument('--project-root', type=str, help="The project root.", required=True)
        parser.add_argument('--gradle-version', type=str, help="The version of Gradle.", required=True)
        parser.add_argument('--asset-mode', type=str, help="The asset mode of deployment", default=ASSET_MODE_LOOSE, choices=SUPPORTED_ASSET_MODES)
        parser.add_argument('--asset-bundle-folder', type=str, help="The sub folder from the project root where the pak files are located (For Pak Asset Mode)",
                            default="AssetBundling/Bundles")

        args = parser.parse_args(sys.argv[1:])

        post_build_action(android_app_root=Path(args.android_app_root),
                          project_root=Path(args.project_root),
                          gradle_version=Version(args.gradle_version),
                          asset_mode=args.asset_mode,
                          asset_bundle_folder=args.asset_bundle_folder)

        exit(0)

    except AndroidPostBuildError as err:
        logging.error(str(err))
        exit(1)
