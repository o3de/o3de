#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import argparse
import hashlib
import os
import pathlib
import re
import shutil
import stat
import sys
import tempfile
import platform
import logging
from packaging.version import Version

logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.INFO)

ANDROID_ARCH = 'arm64-v8a'

ASSET_MODE_PAK = 'PAK'
ASSET_MODE_LOOSE = 'LOOSE'
ASSET_MODE_VFS = 'VFS'


class AndroidPostBuildError(Exception):
    pass


def remove_link(link:pathlib.PurePath):
    """
    Helper function to either remove a symlink, or remove a folder
    """
    link = pathlib.PurePath(link)
    if os.path.isdir(link):
        try:
            os.unlink(link)
        except OSError:
            # If unlink fails use shutil.rmtree
            def remove_readonly(func, path, _):
                "Clear the readonly bit and reattempt the removal"
                os.chmod(path, stat.S_IWRITE)
                func(path)

            try:
                shutil.rmtree(link, onerror=remove_readonly)
            except shutil.Error as shutil_error:
                raise AndroidPostBuildError(f'Error trying remove directory {link}: {shutil_error}', shutil_error.errno)


def create_link(src:pathlib.Path, tgt:pathlib.Path, copy:bool):
    """
    Helper function to create a directory link or copy a directory. On windows, this will be a directory junction, and on mac/linux
    this will be a soft link

    :param src:     The name of the link to create
    :param tgt:     The target of the new link
    :param copy:    Perform a directory copy instead of a link
    """
    src = pathlib.Path(src)
    tgt = pathlib.Path(tgt)
    if copy:
        # Remove the exist target
        if tgt.exists():
            if tgt.is_symlink():
                tgt.unlink()
            else:
                def remove_readonly(func, path, _):
                    "Clear the readonly bit and reattempt the removal"
                    os.chmod(path, stat.S_IWRITE)
                    func(path)
                shutil.rmtree(tgt, onerror=remove_readonly)

        logging.debug(f'Copying from {src} to {tgt}')
        shutil.copytree(str(src), str(tgt), symlinks=False)
    else:
        link_type = "symlink"
        logging.debug(f'Creating symlink {src} =>{tgt}')
        try:
            if platform.system() == "Windows":
                link_type = "junction"
                import _winapi
                _winapi.CreateJunction(str(src), str(tgt))
            else:
                if tgt.exists():
                    tgt.unlink()
                tgt.symlink_to(src, target_is_directory=True)
        except OSError as err:
            raise AndroidPostBuildError(f"Error trying to create {link_type} {src} => {tgt} : {err}")


def determine_intermediate_folder_from_compile_commands(android_app_root_path:pathlib.Path,native_build_path:str,build_config:str,) -> str:
    """
    Gradle 7.x+ started to use a generated intermediate folder which makes it difficult the locate some of the binary files that do need
    to be included in the APK but are not automatically copied since there is no link dependencies on them.
    We will use a hack method to determine the name of the intermediate folder by inspecting the known 'compile_commands.json' that is generated
    by the Android Gradle Plugin (in {app_build_root}.parent/tools/{build_config}/{ANDROID_ARCH}/compile_commands.json and search for it from the directory pattern:
    {android_app_root_path.as_posix()}/{native_build_path}/{build_config}/(.*)/{ANDROID_ARCH}

    :param android_app_root_path:   The root path to the 'app' source in the gradle build
    :param native_build_path:       The specified path for the intermediate build folders
    :param build_config:            The build configuration
    :return: The intermediate folder name
    """
    compile_commands_path = android_app_root_path / native_build_path / 'tools' / build_config / ANDROID_ARCH / 'compile_commands.json'
    if not compile_commands_path.is_file():
        raise AndroidPostBuildError(f"Unable to locate  path: {compile_commands_path}.")
    compile_command_content = compile_commands_path.read_text()
    search_pattern = re.compile(f"{android_app_root_path.as_posix()}/{native_build_path}/{build_config}/(.*)/{ANDROID_ARCH}", re.MULTILINE)
    matched = search_pattern.search(compile_command_content)
    if not matched:
        raise AndroidPostBuildError(f"Unable to determine intermediate folder from the contents of {compile_commands_path}.")
    intermediate_folder_name = matched.group(1)
    return intermediate_folder_name


def apply_pak_layout(project_root: pathlib.Path, asset_bundle_folder:str, target_layout_root:pathlib.Path, copy:bool) -> None:

    src_pak_file_full_path = project_root / asset_bundle_folder
    if not src_pak_file_full_path.is_dir():
        raise AndroidPostBuildError(f"Pak files are expected at location {src_pak_file_full_path}, but the folder doesnt exist. Make sure "
                                    f"to create release bundles (Pak files) before building and deploying to an android device. Refer to "
                                    f"https://www.docs.o3de.org/docs/user-guide/packaging/asset-bundler/bundle-assets-for-release/ for more"
                                    f"information.")
    else:
        pak_count = 0
        for pak_dir_item in src_pak_file_full_path.iterdir():
            if pak_dir_item.is_file() and pak_dir_item.suffix == '.pak':
                pak_count += 1
        if pak_count == 0:
            raise AndroidPostBuildError(f"Pak files are expected at location {src_pak_file_full_path}, but none was detected. Make sure "
                                        f"to create release bundles (Pak files) before building and deploying to an android device. Refer to "
                                        f"https://www.docs.o3de.org/docs/user-guide/packaging/asset-bundler/bundle-assets-for-release/ for more"
                                        f"information.")

    if target_layout_root.is_dir():
        remove_link(target_layout_root)
    create_link(src_pak_file_full_path, target_layout_root, copy)

def apply_loose_layout(project_root:pathlib.Path, target_layout_root:pathlib.Path, agp_intermediate_folder_name:str, copy:bool) -> None:

    android_cache_folder = project_root / 'Cache' / 'android'
    engine_json_marker = android_cache_folder / 'engine.json'
    if not engine_json_marker.is_file():
        raise AndroidPostBuildError(f"Assets have not been built for this project at ({project_root}) yet. "
                                    f"Please run the AssetProcessor for this project first.")
    if target_layout_root.is_dir():
        remove_link(target_layout_root)
    create_link(android_cache_folder, target_layout_root, copy)

def apply_vfs_layout(project_root:pathlib.Path, target_layout_root:pathlib.Path, copy:bool) -> None:

    android_cache_folder = project_root / 'Cache' / 'android'
    engine_json_marker = android_cache_folder / 'engine.json'
    if not engine_json_marker.is_file():
        raise AndroidPostBuildError(f"Assets have not been built for this project at ({project_root}) yet. "
                                    f"Please run the AssetProcessor for this project first.")

    vfs_asset_source = android_cache_folder / 'config'
    if not vfs_asset_source.is_dir():
        raise AndroidPostBuildError(f"Asset cache folder at {android_cache_folder} is missing a 'config' folder.")

    # create a temporary folder that will serve as a working junction point into the layout
    hasher = hashlib.md5()
    hasher.update(str(project_root).encode('UTF-8'))
    result = hasher.hexdigest()

    temp_dir = tempfile.gettempdir()
    temp_vfs_layout_path = os.path.join(temp_dir, 'ly-layout-{}'.format(result), 'vfs')
    temp_vfs_layout_project_path = pathlib.Path(temp_vfs_layout_path)
    temp_vfs_layout_project_config_path = temp_vfs_layout_project_path / 'config'

    # If the temporary folder was created previously, always reset it
    if temp_vfs_layout_project_path.is_dir():
        if temp_vfs_layout_project_config_path.is_dir():
            temp_vfs_layout_project_config_path.rmdir()
        shutil.rmtree(temp_vfs_layout_project_path)
    temp_vfs_layout_project_path.mkdir(parents=True, exist_ok=True)

    # Create the 'project asset platform cache' junction before copying configuration files at the engine root to it
    layout_project_folder_target = target_layout_root
    # Remove previous layout folder if it is a directory
    if os.path.isdir(layout_project_folder_target):
        remove_link(layout_project_folder_target)
    if os.path.isdir(temp_vfs_layout_project_path):
        create_link(temp_vfs_layout_project_path, target_layout_root, copy)

    # Create the link
    create_link(vfs_asset_source, temp_vfs_layout_project_config_path, copy)

    # Copy minimum assets to the layout necessary for vfs
    root_assets = ['engine.json',
                   'bootstrap.client.debug.setreg', 'bootstrap.client.profile.setreg', 'bootstrap.client.release.setreg',
                   'bootstrap.server.debug.setreg', 'bootstrap.server.profile.setreg', 'bootstrap.server.release.setreg',
                   'bootstrap.unified.debug.setreg', 'bootstrap.unified.profile.setreg', 'bootstrap.unified.release.setreg']
    for root_asset in root_assets:
        logging.debug(f"Copying {android_cache_folder}/{root_asset} -> {target_layout_root}")
        shutil.copy2(android_cache_folder/root_asset, target_layout_root)

def apply_gradle_8x_rules(android_app_root_path:pathlib.Path,
                          project_root:pathlib.Path,
                          native_build_path:str,
                          build_config:str,
                          asset_mode:str,
                          asset_bundle_folder:str,
                          is_monolithic:bool,
                          apk_assets:bool,
                          copy:bool):

    base_intermediate_cxx_path = android_app_root_path / 'build' / 'intermediates' / 'cxx' / build_config
    if not base_intermediate_cxx_path.is_dir():
        base_intermediate_cxx_path = android_app_root_path / 'build' / 'intermediates' / '.cxx' / build_config
    if not base_intermediate_cxx_path.is_dir():
        raise AndroidPostBuildError(f"Invalid android gradle build path: {base_intermediate_cxx_path} is not a directory or does not exist.")

    # If assets must be included inside the APK, then the layout 'assets' folder will be stored under the app's 'main'
    # folder. This is so it will be packaged into the APK as well. Otherwise, do the layout under a different folder
    # so it's created, but not copied into the APK. The transfer of the assets will come from this path instead but
    # will be done from a non-gradle process
    if apk_assets:
        target_layout_root = android_app_root_path / 'src' / 'main' / 'assets'
    else:
        target_layout_root = android_app_root_path / 'src' / 'assets'

    intermediate_folder_name = determine_intermediate_folder_from_compile_commands(android_app_root_path=android_app_root_path,
                                                                                   native_build_path=native_build_path,
                                                                                   build_config=build_config)

    if asset_mode == ASSET_MODE_LOOSE:

        apply_loose_layout(project_root=project_root,
                           agp_intermediate_folder_name=intermediate_folder_name,
                           target_layout_root=target_layout_root,
                           copy=copy)

    elif asset_mode == ASSET_MODE_PAK:

        apply_pak_layout(project_root=project_root,
                         target_layout_root=target_layout_root,
                         copy=copy)

    elif asset_mode == ASSET_MODE_VFS:

        apply_vfs_layout(project_root=project_root,
                         target_layout_root=target_layout_root,
                         copy=copy)
    else:
        raise AndroidPostBuildError(f"Invalid asset mode: {asset_mode}.")

    if not is_monolithic:
        source_module_path = android_app_root_path / 'build' / 'intermediates' / 'stripped_native_libs' / build_config / 'out' / 'lib' / ANDROID_ARCH / build_config
        destination_module_path = android_app_root_path / 'build' / 'intermediates' / 'stripped_native_libs' / build_config / 'out' / 'lib' / ANDROID_ARCH
        for source_module in source_module_path.iterdir():
            logging.info(f"Copying file {source_module.name} to {destination_module_path}")
            shutil.copy2(source_module, destination_module_path)



GRADLE_VER_8_0 = Version("8.0")


def post_build_action(android_app_root: pathlib.Path, project_root:pathlib.Path, gradle_version:Version, build_config:str, native_build_path:str, asset_mode:str, asset_bundle_folder: str, is_monolithic:bool, apk_assets:bool, copy:bool):

    android_app_root_path = pathlib.Path(android_app_root)
    if not android_app_root_path.is_dir():
        raise AndroidPostBuildError(f"Invalid android gradle build path: {android_app_root_path} is not a directory or does not exist.")

    logging.info(f"Applying post-build for gradle version {gradle_version}")

    # Validate the build directory exists
    app_build_root = android_app_root_path / 'build'
    if not app_build_root.is_dir():
        raise AndroidPostBuildError(f"Android gradle build path: {app_build_root} is not a directory or does not exist.")

    if gradle_version >= GRADLE_VER_8_0:
        apply_gradle_8x_rules(android_app_root_path=android_app_root_path,
                              project_root=project_root,
                              native_build_path=native_build_path,
                              build_config=build_config,
                              asset_mode=asset_mode,
                              is_monolithic=is_monolithic,
                              apk_assets=apk_assets,
                              asset_bundle_folder=asset_bundle_folder,
                              copy=copy)

if __name__ == '__main__':

    try:
        parser = argparse.ArgumentParser(description="Android post gradle build step handler")
        parser.add_argument('android_app_root', type=str, help="The base of the 'app' in the O3DE generated gradle script.")
        parser.add_argument('--project-root', type=str, help="The project root.", required=True)
        parser.add_argument('--gradle-version', type=str, help="The version of gradle.", required=True)
        parser.add_argument('--build-config', type=str, help="The build configuration that was built.", required=True)
        parser.add_argument('--native-build-path', type=str, help="The native build path (from the app folder) that was specified in the android-generate command.",
                            default='.')
        parser.add_argument('--asset-mode', type=str, help="The asset mode of deployment (LOOSE, PAK, VFS)", default='LOOSE', choices=['LOOSE', 'PAK', 'VFS'])
        parser.add_argument('--monolithic', help="Flag to indicate this is a monolithic build",action='store_true')
        parser.add_argument('--apk-assets', help="Flag to indicate that assets will be contained within the apk", action='store_true')
        parser.add_argument('--asset-bundle-folder', type=str, help="The sub folder from the project root where the pak files are located (For Pak Asset Mode)",
                            default="AssetBundling/Bundles")

        parser.add_argument('--copy', help="Flag to copy assets to the android assets folder instead of creating links",action='store_true')

        args = parser.parse_args(sys.argv[1:])

        result_code = post_build_action(android_app_root=pathlib.Path(args.android_app_root),
                                        project_root=pathlib.Path(args.project_root),
                                        gradle_version=Version(args.gradle_version),
                                        native_build_path=args.native_build_path,
                                        build_config=args.build_config,
                                        asset_mode=args.asset_mode,
                                        asset_bundle_folder=args.asset_bundle_folder,
                                        is_monolithic=args.monolithic,
                                        apk_assets=args.apk_assets,
                                        copy=args.copy)

        exit(result_code)

    except AndroidPostBuildError as err:
        logging.error(str(err))
        exit(1)
