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
ASSET_PLATFORM_KEY = 'android'

MINIMUM_ANDROID_GRADLE_PLUGIN_VER = Version("8.0")

IS_PLATFORM_WINDOWS = platform.system() == 'Windows'

PAK_FILE_INSTRUCTIONS = "Make sure to create release bundles (Pak files) before building and deploying to an android device. Refer to " \
                        "https://www.docs.o3de.org/docs/user-guide/packaging/asset-bundler/bundle-assets-for-release/ for more" \
                        "information."


class AndroidPostBuildError(Exception):
    pass


def copy_or_create_link(src: Path, tgt: Path, copy_folders: set = {}):
    """
    Copy or create a link/junction depending on the source type. If this is a file, then perform file copy from the
    source to the target. If it is a directory, then create a junction(windows) or symlink(linux) from the source to
    the target

    :param src:             The source to copy/link from
    :param tgt:             The target tp copy/link to
    :param copy_folders:    Optional list of folder names that will perform a copy instead of a junction
    """

    assert src.exists()
    assert not tgt.exists()

    full_src_path = str(src.resolve().absolute())
    full_tgt_path = str(tgt.resolve().absolute())

    try:
        if src.is_file():
            shutil.copy2(src=full_src_path,
                         dst=full_tgt_path,
                         follow_symlinks=True)
            logger.info(f"Copied {full_src_path} to {full_tgt_path}")
        else:
            if src.name in copy_folders:
                shutil.copytree(full_src_path, full_tgt_path)
                logger.info(f"Copied Folder {full_src_path} to {full_tgt_path}")
            else:
                if IS_PLATFORM_WINDOWS:
                    import _winapi
                    _winapi.CreateJunction(full_src_path, full_tgt_path)
                    logger.info(f'Created Junction {full_src_path} => {full_tgt_path}')
                else:
                    tgt.symlink_to(src, target_is_directory=True)
                    logger.info(f'Created symbolic link {full_src_path} => {full_tgt_path}')

    except OSError as err:
        raise AndroidPostBuildError(f"Error trying to copy/link  {src} => {tgt} : {err}")


def safe_clear_folder(target_folder: Path, delete_folders: set = {}) -> None:
    """
    Safely clean the contents of a folder. If items are links/junctions, then attempt to unlink, but if the
    items are non-linked, then perform a deletion.

    :param target_folder:   Folder to safely clear
    :param delete_folders:  Optional set of folders to perform a delete on instead of an unlink
    """

    if not target_folder.exists():
        logger.warn(f"Nothing to clean. '{target_folder}' does not exist")
        return

    if not target_folder.is_dir():
        logger.warn(f"Unable to clean '{target_folder}', target is not a directory")
        return

    for target_item in target_folder.iterdir():
        try:
            if target_item.is_dir() and target_item.name in delete_folders:
                shutil.rmtree(str(target_item))
            else:
                target_item.unlink()
        except OSError as os_err:
            raise AndroidPostBuildError(f"Error trying to unlink/delete {target_item}: {os_err}")


def synchronize_folders(src: Path, tgt: Path, copy_folders: set = {}) -> None:
    """
    Create a copy of a source folder 'src' to a target folder 'tgt', but use the following rules:
    1. Make sure that a 'tgt' folder exists
    2. For each item in the 'src' folder, call 'copy_or_create_link' to apply a copy or link of the items to the
       target folder 'tgt'

    :param src:             The source folder to synchronize from
    :param tgt:             The target folder to synchronize to
    :param copy_folders:    Optional list of folder names that will perform a copy instead of a junction
    """

    assert not tgt.is_file()
    assert src.is_dir()

    # Make sure the target folder exists
    tgt.mkdir(parents=True, exist_ok=True)

    # Iterate through the items in the source folder and copy to the target folder
    processed = 0
    for src_item in src.iterdir():
        tgt_item = tgt / src_item.name
        copy_or_create_link(src_item, tgt_item, copy_folders=copy_folders)
        processed += 1

    logger.info(f"{processed} items from {src} linked/copied to {tgt}")


def determine_intermediate_folder_from_compile_commands(android_app_root_path:Path, native_build_folder:str, build_config:str,) -> str:
    """
    Gradle 7.x+ started to use a generated intermediate folder which makes it difficult the locate some of the binary files that do need
    to be included in the APK but are not automatically copied since there is no link dependencies on them.
    We will use a hack method to determine the name of the intermediate folder by inspecting the known 'compile_commands.json' that is generated
    by the Android Gradle Plugin (in {app_build_root}.parent/tools/{build_config}/{ANDROID_ARCH}/compile_commands.json and search for it from the directory pattern:
    {android_app_root_path.as_posix()}/{native_build_path}/{build_config}/(.*)/{ANDROID_ARCH}

    :param android_app_root_path:   The root path to the 'app' source in the gradle build
    :param native_build_folder:     The specified path for the intermediate build folders
    :param build_config:            The build configuration
    :return: The intermediate folder name
    """
    compile_commands_path = android_app_root_path / native_build_folder / 'tools' / build_config / ANDROID_ARCH / 'compile_commands.json'
    if not compile_commands_path.is_file():
        raise AndroidPostBuildError(f"Unable to locate  path: {compile_commands_path}.")
    compile_command_content = compile_commands_path.read_text()
    search_pattern = re.compile(f"app/{native_build_folder}/{build_config}/(.*)/{ANDROID_ARCH}", re.MULTILINE)
    matched = search_pattern.search(compile_command_content)
    if not matched:
        raise AndroidPostBuildError(f"Unable to determine intermediate folder from the contents of {compile_commands_path}.")
    intermediate_folder_name = matched.group(1)
    return intermediate_folder_name


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

    # Clear out the target folder first
    safe_clear_folder(target_layout_root)

    # Copy/Link the contents to the target folder
    synchronize_folders(src_pak_file_full_path, target_layout_root)


def apply_loose_layout(project_root: Path, target_layout_root: Path, android_app_root_path: Path, native_build_folder: str, build_config: str) -> None:
    """
    Apply the loose assets folder layout rules to the target assets folder

    :param project_root:            The project folder root to look for the loose assets
    :param target_layout_root:      The target layout destination of the loose assets
    :param android_app_root_path:   The root of the 'app' project in the gradle script
    :param native_build_folder:     The sub folder where the intermediate native project files are generated
    :param build_config:            The build configuration used for the native build
    """

    android_cache_folder = project_root / 'Cache' / ASSET_PLATFORM_KEY
    engine_json_marker = android_cache_folder / 'engine.json'
    if not engine_json_marker.is_file():
        raise AndroidPostBuildError(f"Assets have not been built for this project at ({project_root}) yet. "
                                    f"Please run the AssetProcessor for this project first.")

    # Clear out the target folder first
    safe_clear_folder(target_layout_root, delete_folders={'registry'})

    # Copy/Link the contents to the target folder
    synchronize_folders(android_cache_folder, target_layout_root, copy_folders={'registry'})

    # Copy over the Registry folder contents to the target layout
    try:
        intermediate_folder = determine_intermediate_folder_from_compile_commands(android_app_root_path=android_app_root_path,
                                                                                  native_build_folder=native_build_folder,
                                                                                  build_config=build_config)

        # Locate the Android Gradle Plugin version 7.x specific location to where the Registry folder is generated
        registry_source_folder = android_app_root_path / native_build_folder / build_config / intermediate_folder / ANDROID_ARCH / native_build_folder / 'Registry'
        if registry_source_folder.is_dir():
            target_asset_registry_path = target_layout_root / 'registry'
            if not target_asset_registry_path.is_dir():
                target_asset_registry_path.mkdir(parents=True, exist_ok=True)
            # Copy each item individually into the target folder
            for src_item in registry_source_folder.iterdir():
                if src_item.is_file():
                    shutil.copy2(str(src_item), str(target_asset_registry_path / src_item.name))
                elif src_item.is_dir():
                    shutil.copytree(str(src_item), str(target_asset_registry_path / src_item.name))
        else:
            logger.info(f"Unable to locate generated 'Registry' folder at {registry_source_folder}. Skipping copying of registry files.")
    except AndroidPostBuildError:
        logger.info(f"Unable to determine the intermediate build folder for {target_layout_root} . Skipping copying of registry files.")


def post_build_action(android_app_root: Path,
                      native_build_folder: str,
                      build_config: str,
                      project_root: Path,
                      gradle_version: Version,
                      asset_mode: str,
                      asset_bundle_folder: str):
    """
    Perform the post-build logic for android native builds that will prepare the output folders by laying out the asset files
    to their locations before the APK is generated.

    :param android_app_root:    The root path of the 'app' project within the android gradle build script
    :param native_build_folder: (For gradle versions 7.x+) Gradle uses an intermediate subfolder The under the 'app' project.
    :param project_root:        The root of the project that the APK is being built for
    :param gradle_version:      The version of gradle used to build the APK (for validation)
    :param build_config:        The native build configuration (Debug, Profile, Release)
    :param asset_mode:          The desired asset mode to determine the layout rules
    :param asset_bundle_folder: (For PAK asset modes) the location of where the PAK files are expected.
    """

    android_app_root_path = Path(android_app_root)
    if not android_app_root_path.is_dir():
        raise AndroidPostBuildError(f"Invalid android gradle build path: {android_app_root_path} is not a directory or does not exist.")

    if gradle_version < MINIMUM_ANDROID_GRADLE_PLUGIN_VER:
        raise AndroidPostBuildError(f"Android gradle plugin versions below version {MINIMUM_ANDROID_GRADLE_PLUGIN_VER} is not supported.")
    logger.info(f"Applying post-build for android gradle plugin version {gradle_version}")

    # Validate the build directory exists
    app_build_root = android_app_root_path / 'build'
    if not app_build_root.is_dir():
        raise AndroidPostBuildError(f"Android gradle build path: {app_build_root} is not a directory or does not exist.")

    target_layout_root = android_app_root_path / 'src' / 'main' / 'assets'

    if asset_mode == ASSET_MODE_LOOSE:

        apply_loose_layout(project_root=project_root,
                           target_layout_root=target_layout_root,
                           android_app_root_path=android_app_root_path,
                           native_build_folder=native_build_folder,
                           build_config=build_config)

    elif asset_mode == ASSET_MODE_PAK:

        apply_pak_layout(project_root=project_root,
                         target_layout_root=target_layout_root,
                         asset_bundle_folder=asset_bundle_folder)


if __name__ == '__main__':

    try:
        parser = argparse.ArgumentParser(description="Android post gradle build step handler")
        parser.add_argument('android_app_root', type=str, help="The base of the 'app' in the O3DE generated gradle script.")
        parser.add_argument('--native-build-folder',  type=str, help="The native builder intermediate folder (for AGP version 7.x and newer)",
                            default='o3de')
        parser.add_argument('--build-config', type=str, help="The build configuration for the native build.", required=True)
        parser.add_argument('--project-root', type=str, help="The project root.", required=True)
        parser.add_argument('--gradle-version', type=str, help="The version of gradle.", required=True)
        parser.add_argument('--asset-mode', type=str, help="The asset mode of deployment (LOOSE, PAK, VFS)", default='LOOSE', choices=['LOOSE', 'PAK', 'VFS'])
        parser.add_argument('--asset-bundle-folder', type=str, help="The sub folder from the project root where the pak files are located (For Pak Asset Mode)",
                            default="AssetBundling/Bundles")

        args = parser.parse_args(sys.argv[1:])

        result_code = post_build_action(android_app_root=Path(args.android_app_root),
                                        native_build_folder=args.native_build_folder,
                                        build_config=args.build_config,
                                        project_root=Path(args.project_root),
                                        gradle_version=Version(args.gradle_version),
                                        asset_mode=args.asset_mode,
                                        asset_bundle_folder=args.asset_bundle_folder)

        exit(result_code)

    except AndroidPostBuildError as err:
        logging.error(str(err))
        exit(1)
