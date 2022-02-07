#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


import argparse
import hashlib
import logging
import os
import pathlib
import platform
import shutil
import stat
import subprocess
import sys
import tempfile
import timeit


# Resolve the common python module
ROOT_ENGINE_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))
if ROOT_ENGINE_PATH not in sys.path:
    sys.path.append(ROOT_ENGINE_PATH)

from cmake.Tools import common

LOCAL_HOST = '127.0.0.1'
CACHE_FOLDER_NAME = 'Cache'
ASSET_MODE_PAK = 'PAK'
ASSET_MODE_LOOSE = 'LOOSE'
ASSET_MODE_VFS = 'VFS'
ALL_ASSET_MODES = [ASSET_MODE_PAK, ASSET_MODE_LOOSE, ASSET_MODE_VFS]

PAK_FOLDER_NAME = 'Pak'

# Maintain a list of build configs that only will support PAK mode
PAK_ONLY_BUILD_CONFIGS = ['RELEASE']

# Save the platform system name. In our case, this will be one of:
#  Windows
#  Linux
#  Darwin (Currently 'Darwin' with python 3.7). Should use 'Windows' and 'Linux' first and fallback to Darwin
PLATFORM_NAME = platform.system()


# List of files to deny  from copying to the layout folder
COPY_ASSET_FILE_GENERAL_DENYLIST_FILES = [
    'aztest_bootstrap.json',
    'editor.cfg',
    'assetprocessorplatformconfig.setreg',
]

def verify_layout(layout_dir, platform_name, project_path, asset_mode, asset_type):
    """
    Verify a layout folder (WRT to assets and configs) against the bootstrap and system config files

    @param layout_dir:      The layout path to validate the asset mode against the bootstrap and system configs
    @param platform_name:   The name of the platform the deployment is for
    @param project_path:    The path to the project being deployed
    @param asset_mode:      The desired asset mode (PAK, LOOSE, VFS)
    @param asset_type:      The asset type
    @return: The number of possible errors in the configuration files based on the asset mode and type
    """

    def _warn(msg):
        logging.warning(msg)
        return 1

    def _validate_remote_ap(input_remote_ip, input_remote_connect, remote_on_check):

        if remote_on_check is None:
            # Validate that if '<platform>_connect_to_remote is enabled, that the 'input_remote_ip' is not set to local host
            if input_remote_connect == '1' and input_remote_ip == LOCAL_HOST:
                return _warn("'bootstrap.setreg' is configured to connect to Asset Processor remotely, but the 'remote_ip' "
                             " is configured for LOCAL HOST")
        else:
            if remote_on_check:
                # Verify we are set for remote AP connection
                if input_remote_ip == LOCAL_HOST:
                    return _warn(f"'bootstrap.setreg' is not configured for a remote Asset Processor connection (remote_ip={input_remote_ip})")
                if input_remote_connect != '1':
                    return _warn(f"'bootstrap.setreg' is not configured for a remote Asset Processor connection ({platform_name}_connect_to_remote={input_remote_connect}")
            else:
                # Verify we are disabled for remote AP connection
                if input_remote_connect != '0':
                    return _warn(f"'bootstrap.setreg' is not configured for a remote Asset Processor connection ({platform_name}_connect_to_remote={input_remote_connect}")

        return 0

    warning_count = 0

    # Look up the project_path from the project.json file
    project_name = common.read_project_name_from_project_json(project_path)
    # If the project-name could not be read from the project.json, then the supplied project path does not
    # point to a valid project
    if not project_name:
        return 1

    platform_name_lower = platform_name.lower()
    project_name_lower = project_path.lower()
    layout_path = pathlib.Path(layout_dir)

    bootstrap_path = pathlib.Path(ROOT_ENGINE_PATH) / 'Registry'
    bootstrap_values = common.get_bootstrap_values(str(bootstrap_path), [f'{platform_name_lower}_remote_filesystem',
                                                                         f'{platform_name_lower}_connect_to_remote',
                                                                         f'{platform_name_lower}_wait_for_connect',
                                                                         f'{platform_name_lower}_assets',
                                                                         f'assets',
                                                                         f'{platform_name_lower}_remote_ip',
                                                                         f'remote_ip'
                                                                         ])

    # Validate the system_{platform}_{asset type}.cfg exists
    platform_system_cfg_file = layout_path / f'system_{platform_name_lower}_{asset_type}.cfg'
    if not platform_system_cfg_file.is_file():
        warning_count += _warn(f"'system_{platform_name_lower}_{asset_type}.cfg' is missing from {str(layout_path)}")
        system_config_values = None
    else:
        system_config_values = common.get_config_file_values(str(platform_system_cfg_file), [])

    if bootstrap_values:

        remote_ip = bootstrap_values.get(f'{platform_name_lower}_remote_ip') or bootstrap_values.get('remote_ip') or LOCAL_HOST
        remote_connect = bootstrap_values.get(f'{platform_name_lower}_connect_to_remote') or '0'

        # Validate that the asset type for the platform matches the one set for the build
        bootstrap_asset_type = bootstrap_values.get(f'{platform_name_lower}_assets') or bootstrap_values.get('assets')
        if not bootstrap_asset_type:
            warning_count += _warn("'bootstrap.setreg' is missing specifications for asset type.")
        elif bootstrap_asset_type != asset_type:
            warning_count += _warn(f"The asset type specified in bootstrap.setreg ({bootstrap_asset_type}) does not match the asset type specified for this deployment({asset_type}).")

        # Validate that if '<platform>_connect_to_remote is enabled, that the 'remote_ip' is not set to local host
        warning_count += _validate_remote_ap(remote_ip, remote_connect, None)

        project_asset_path = layout_path / project_name_lower
        if not project_asset_path.is_dir():
            warning_count += _warn(f"Asset folder for project {project_name} is missing from the deployment layout.")

        elif system_config_values is not None:

            shaders_remote_compiler = '0'
            asset_processor_shader_compiler = system_config_values.get('r_AssetProcessorShaderCompiler') or '0'
            shader_compiler_server = LOCAL_HOST
            shaders_allow_compilation = system_config_values.get('r_ShadersAllowCompilation')

            def _validate_remote_shader_settings():

                if asset_processor_shader_compiler != '1':
                    return _warn(f"Connection to the remote shader compiler (r_ShaderCompilerServer) is not properly "
                                 f"set in system_{platform_name_lower}_{asset_type}.cfg. If it is set to {LOCAL_HOST}, then "
                                 f"r_AssetProcessorShaderCompiler must be set to 1.")


                else:
                    if _validate_remote_ap(remote_ip, remote_connect, False) > 0:
                        return _warn(f"The system_{platform_name_lower}_{asset_type}.cfg file is configured to connect to the"
                                     f" shader compiler server through the remote connection to the Asset Processor.")
                return 0

            # Validation steps based on the asset mode
            if asset_mode == ASSET_MODE_PAK:
                # Validate that we have pak files
                pak_count = 0
                has_shader_pak = False
                project_paks = project_asset_path.glob("*.pak")
                for project_pak in project_paks:
                    if project_pak.name == 'shadercachestartup.pak':
                        has_shader_pak = True
                    pak_count += 1

                if pak_count == 0:
                    warning_count += _warn("No pak files found for PAK mode deployment")
                    # Check if the shader paks are set
                if has_shader_pak:
                    # Since we are not connecting to the shader compiler, also make sure bootstrap is not configured to
                    # connect to Asset Processor remotely
                    warning_count += _validate_remote_ap(remote_ip, remote_connect, False)

                    if shaders_allow_compilation is not None and shaders_allow_compilation == '1':
                        warning_count += _warn(f"Shader paks are set for project {project_name} but shader compiling "
                                               f"(r_ShadersAllowCompilation) is still enabled "
                                               f"for it in system_{platform_name_lower}_{asset_type}.cfg.")

                else:
                    warning_count += _validate_remote_shader_settings()

            elif asset_mode == ASSET_MODE_VFS:
                remote_file_system = bootstrap_values.get(f'{platform_name_lower}_remote_filesystem') or '0'
                if not remote_file_system != '1':
                    warning_count += _warn("Remote file system is not configured in bootstrap.setreg for VFS mode.")
                else:
                    warning_count += _validate_remote_ap(remote_ip, remote_connect, True)

            else:
                # If there are no shader paks, make sure that a connection to the shader compiler is set
                warning_count += _validate_remote_shader_settings()

    return warning_count


def copy_asset_files_to_layout(project_asset_folder, target_platform, layout_target):
    """
    Perform the specific rules for copying files to the root level of the layout.

    :param project_asset_folder: The source project asset folder to copy the files. (Will not traverse deeper than this folder)
    :param target_platform:      The target platform of the layout
    :param layout_target:        The target path of the target layout folder.
    """

    src_asset_contents = os.listdir(project_asset_folder)
    allowed_system_config_prefix = 'system_{}'.format(target_platform.lower())
    for src_file in src_asset_contents:

        # For each source file found in the root of the source project asset folder, apply various rules to determine
        # if we will copy the file to the layout destination or not
        if src_file in COPY_ASSET_FILE_GENERAL_DENYLIST_FILES:
            # The source file is denied from being copied
            continue

        if src_file.startswith('system_'):
            # For system files (system_<platform>_<asset_platform>), only allow the ones that are marked for the
            # current <platform>
            if not src_file.startswith(allowed_system_config_prefix):
                continue

        # Resolve the absolute paths for source and destination to perform more specific checks
        abs_src = os.path.join(project_asset_folder, src_file)
        abs_dst = os.path.join(layout_target, src_file)

        if os.path.isdir(abs_src):
            # Skip all source folders
            continue

        # The target file exists, check whats at the target
        if os.path.isdir(abs_dst):
            # The target destination is a folder, we will skip
            logging.warning("Skipping layout copying of file '%s' because the target '%s' refers to a directory",
                            src_file,
                            abs_dst)
            continue

        if os.path.isfile(abs_dst):
            # The target is a file, do a fingerprint check
            # TODO: Evaluate if we want to just junction the files instead of doing a copy
            src_hash = common.file_fingerprint(abs_src)
            dst_hash = common.file_fingerprint(abs_dst)
            if src_hash == dst_hash:
                logging.debug("Skipping layout copy of '%s', fingerprints of source and destination matches (%s)",
                              src_file,
                              src_hash)
                continue

            logging.debug("Copying %s -> %s", abs_src, abs_dst)
            shutil.copy2(abs_src, abs_dst)


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
                raise common.LmbrCmdError(f'Error trying remove directory {link}: {shutil_error}', shutil_error.errno)


def create_link(src:pathlib.Path, tgt:pathlib.Path, copy):
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
            if PLATFORM_NAME == "Windows":
                link_type = "junction"
                import _winapi
                _winapi.CreateJunction(str(src), str(tgt))
            else:
                if tgt.exists():
                    tgt.unlink()
                tgt.symlink_to(src, target_is_directory=True)
        except OSError as e:
            raise common.LmbrCmdError(f"Error trying to create {link_type} {src} => {tgt} : {e}", e.errno)


def construct_and_validate_cache_project_asset_folder(project_path, asset_type, warn_on_missing_project_cache):
    """
    Given the parameters for a project (project_path, asset type), construct and validate the absolute path
    of where the built assets are (for LOOSE and VFS modes)

    :param project_path:                   The path to the project
    :param asset_type:                     The type of asset
    :param warn_on_missing_project_cache:  Option to warn if the path is missing vs raising an exception
    :return: The validated constructed cache project asset folder if it exists, None if not
    """
    # Locate the Cache root folder
    cache_project_folder_root = os.path.join(project_path, CACHE_FOLDER_NAME)
    if not os.path.isdir(cache_project_folder_root) and not warn_on_missing_project_cache:
        raise common.LmbrCmdError(
            f"Missing Cache folder for the project at path {project_path}. Make sure that assets have been built ",
            common.ERROR_CODE_ERROR_DIRECTORY)

    # Locate based on the project's built asset type
    cache_project_asset_folder = os.path.join(cache_project_folder_root, asset_type)
    if os.path.isdir(cache_project_asset_folder):
        # TODO: Note, this is only checking the existence of the folder, not for any content validation
        return cache_project_asset_folder

    # Expected source of the project assets was not found
    if not warn_on_missing_project_cache:
        raise common.LmbrCmdError(
            f'Missing compiled assets folder for the project at path {project_path}."'
            f' Make sure that assets for "{asset_type}" have been built',
            common.ERROR_CODE_ERROR_DIRECTORY)

    return None


def sync_layout_vfs(target_platform, project_path, asset_type, warning_on_missing_assets, layout_target, override_pak_folder, copy):
    """
    Perform the logic to sync the layout folder with assets in VFS mode

    :param target_platform:             The target platform the layout is based on
    :param project_path:                The path to the project being synced
    :param asset_type:                  The asset type being synced
    :param warning_on_missing_assets:   If the built assets cannot be located (LOOSE or PAKs), then optionally warn vs raising an error
    :param layout_target:               The target layout folder to perform the sync on
    :param override_pak_folder:         The optional path to override the default pak folder for PAK asset mode (N/A for this function)
    :param copy:                        Option to copy instead of attempting to symlink/junction
    """

    logging.debug(f'Syncing VFS layout for project at path "{project_path}" to layout path "{layout_target}"')

    project_asset_folder = construct_and_validate_cache_project_asset_folder(project_path=project_path,
                                                                       asset_type=asset_type,
                                                                       warn_on_missing_project_cache=warning_on_missing_assets)

    vfs_asset_source = os.path.join(project_asset_folder, 'config')
    if not os.path.isdir(vfs_asset_source):
        raise common.LmbrCmdError("Cache folder for the project '{}' missing 'config' folder".format(project_path),
                                  common.ERROR_CODE_ERROR_DIRECTORY)

    # create a temporary folder that will serve as a working junction point into the layout
    hasher = hashlib.md5()
    hasher.update(project_path.encode('UTF-8'))
    result = hasher.hexdigest()

    temp_dir = tempfile.gettempdir()
    temp_vfs_layout_path = os.path.join(temp_dir, 'ly-layout-{}'.format(result), 'vfs')
    temp_vfs_layout_project_path = temp_vfs_layout_path

    temp_vfs_layout_project_config_path = os.path.join(temp_vfs_layout_project_path, 'config')

    # If the temporary folder was created previously, always reset it
    if os.path.isdir(temp_vfs_layout_project_path):
        if os.path.isdir(temp_vfs_layout_project_config_path):
            os.rmdir(temp_vfs_layout_project_config_path)
        shutil.rmtree(temp_vfs_layout_project_path)
    os.makedirs(temp_vfs_layout_project_path, exist_ok=True)

    # Create the 'project asset platform cache' junction before copying configuration files at the engine root to it
    layout_project_folder_target = layout_target
    # Remove previous layout folder if it is a directory
    if os.path.isdir(layout_project_folder_target):
        remove_link(layout_project_folder_target)
    if os.path.isdir(temp_vfs_layout_project_path):
        create_link(temp_vfs_layout_project_path, layout_project_folder_target, copy)

    # Create the link
    create_link(vfs_asset_source, temp_vfs_layout_project_config_path, copy)

    # Create the assets to the layout
    copy_asset_files_to_layout(project_asset_folder=project_asset_folder,
                               target_platform=target_platform,
                               layout_target=layout_target)

    # Reset the 'gems' junction if any in the layout
    layout_gems_folder_src = os.path.join(project_asset_folder, 'gems')
    layout_gems_folder_target = os.path.join(layout_target, 'gems')
    if os.path.isdir(layout_gems_folder_target):
        remove_link(layout_gems_folder_target)
    if os.path.isdir(layout_gems_folder_src):
        create_link(layout_gems_folder_src, layout_gems_folder_target, copy)



def sync_layout_non_vfs(mode, target_platform, project_path, asset_type, warning_on_missing_assets, layout_target, override_pak_folder, copy):
    """
    Perform the logic to sync the layout folder with assets in non-VFS mode (LOOSE or PAK)

    :param mode:                        'LOOSE' or 'PAK' mode
    :param target_platform:             The target platform the layout is based on
    :param project_path:                The path to the project being synced
    :param asset_type:                  The asset type being synced
    :param warning_on_missing_assets:   If the built assets cannot be located (LOOSE or PAKs), then optionally warn vs raising an error
    :param layout_target:               The target layout folder to perform the sync on
    :param override_pak_folder:         The optional path to override the default pak folder for PAK asset mode (N/A for this function)
    :param copy:                        Option to copy instead of attempting to symlink/junction
    """

    assert mode in (ASSET_MODE_PAK, ASSET_MODE_LOOSE)

    project_name = common.read_project_name_from_project_json(project_path)
    if not project_name:
        raise common.LmbrCmdError(f'Project at path {project_path} does not have a valid project.json')

    project_name_lower = project_name.lower()
    layout_gems_folder_target = os.path.join(layout_target, 'gems')
    if os.path.isdir(layout_gems_folder_target):
        remove_link(layout_gems_folder_target)

    if mode == ASSET_MODE_PAK:
        target_pak_folder_name = '{}_{}_paks'.format(project_name_lower, asset_type)
        project_asset_folder = os.path.join(project_path, override_pak_folder or PAK_FOLDER_NAME, target_pak_folder_name)
        if not os.path.isdir(project_asset_folder):
            if warning_on_missing_assets:
                logging.warning(f'Pak folder for the project at path "{project_path}" is missing'
                                f' (expected at "{project_asset_folder}"). Skipping layout sync')
                return
            else:
                raise common.LmbrCmdError(f'Pak folder for the project at path "{project_path}" is missing (expected at'
                                          f' "{project_asset_folder}")',
                                          common.ERROR_CODE_ERROR_DIRECTORY)

    elif mode == ASSET_MODE_LOOSE:
        project_asset_folder = construct_and_validate_cache_project_asset_folder(project_path=project_path,
                                                                           asset_type=asset_type,
                                                                           warn_on_missing_project_cache=warning_on_missing_assets)
        if not project_asset_folder:
            logging.warning(
                f'Cannot locate built assets for project at path "{project_path}" (expected at "{project_asset_folder}").'
                f' Skipping layout sync')
            return
    else:
        assert False, "Invalid Mode {}".format(mode)

    # Create the 'project asset platform cache' junction before copying additional files to it
    layout_project_folder_src = project_asset_folder
    # Remove previous layout folder if it is a directory
    if os.path.isdir(layout_target):
        remove_link(layout_target)
    if os.path.isdir(layout_project_folder_src):
        create_link(layout_project_folder_src, layout_target, copy)

    # Create the assets to the layout
    copy_asset_files_to_layout(project_asset_folder=project_asset_folder,
                               target_platform=target_platform,
                               layout_target=layout_target)

    # Reset the 'gems' junction if any in the layout (only in loose mode).
    layout_gems_folder_src = os.path.join(project_asset_folder, 'gems')

    # The gems link only is valid in LOOSE mode. If in PAK, then dont re-link
    if mode == ASSET_MODE_LOOSE and os.path.isdir(layout_gems_folder_src):
        if os.path.isdir(layout_gems_folder_src):
            create_link(layout_gems_folder_src, layout_gems_folder_target, copy)


def sync_layout_pak(target_platform, project_path, asset_type, warning_on_missing_assets, layout_target,
                    override_pak_folder, copy):
    sync_layout_non_vfs(mode=ASSET_MODE_PAK,
                        target_platform=target_platform,
                        project_path=project_path,
                        asset_type=asset_type,
                        warning_on_missing_assets=warning_on_missing_assets,
                        layout_target=layout_target,
                        override_pak_folder=override_pak_folder,
                        copy=copy)


def sync_layout_loose(target_platform, project_path, asset_type, warning_on_missing_assets, layout_target,
                      override_pak_folder, copy):
    sync_layout_non_vfs(mode=ASSET_MODE_LOOSE,
                        target_platform=target_platform,
                        project_path=project_path,
                        asset_type=asset_type,
                        warning_on_missing_assets=warning_on_missing_assets,
                        layout_target=layout_target,
                        override_pak_folder=override_pak_folder,
                        copy=copy)


ASSET_SYNC_MODE_FUNCTION = {
    ASSET_MODE_VFS: sync_layout_vfs,
    ASSET_MODE_PAK: sync_layout_pak,
    ASSET_MODE_LOOSE: sync_layout_loose
}


def main(args):
    parser = argparse.ArgumentParser(description="Synchronize a project's assets to a layout folder")

    parser.add_argument('--project-path',
                        help='The project path whose assets we will sync.',
                        required=True)
    parser.add_argument('-p', '--platform',
                        help='Target platform for the layout.',
                        required=True)
    parser.add_argument('-a', '--asset-type',
                        help='The asset type to use for this deployment',
                        default='pc')
    parser.add_argument('--debug',
                        action='store_true',
                        help='Enable debug logs.')
    parser.add_argument('--warn-on-missing-assets',
                        action='store_true',
                        help='If the project does not have any built assets, warn rather than return an error')
    parser.add_argument('-m', '--mode',
                        type=str,
                        choices=ALL_ASSET_MODES,
                        default=ASSET_MODE_LOOSE,
                        help='Asset Mode (vfs|pak|loose)')
    parser.add_argument('-l', '--layout-root',
                        help='The layout root to where the sync of the assets will occur',
                        required=True)
    parser.add_argument('--create-layout-root',
                        action='store_true',
                        help='If the layout root doesnt exist, create it')
    parser.add_argument('--override-pak-folder',
                        default='',
                        help='(optional) If provided, use this path as the path to the pak folder when creating layouts '
                             'in PAK mode. Otherwise, use the {project_path}/pak/${project}_${asset_type}_pak as the source pak folder')
    parser.add_argument('--build-config',
                        default='',
                        help='(optional) If provided, will adjust the asset mode if the provided build-config is "release"')
    parser.add_argument('-c', '--copy',
                        action='store_true',
                        help='Copy the files instead of symlinking.')
    parser.add_argument('--verify',
                        action='store_true',
                        help='Option to perform a verification and report warnings against bootstrap and system configs based on the asset mode and type.')
    parser.add_argument('--fail-on-warning',
                        action='store_true',
                        help='Option to perform a verification of the layout against the bootstrap and system configs.')


    parsed_args = parser.parse_args(args)

    # Prepare the logging
    logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.DEBUG if parsed_args.debug else logging.INFO)

    # Validate the asset mode
    input_asset_mode = parsed_args.mode.upper()
    if input_asset_mode not in ALL_ASSET_MODES:
        raise common.LmbrCmdError("Invalid asset mode '{}'. Must be one of : '{}'.".format(input_asset_mode, ','.join(ALL_ASSET_MODES)),
                                  common.ERROR_CODE_INVALID_PARAMETER)

    # Check if the build config is set, if so, check if its release
    build_config = parsed_args.build_config.upper()
    if build_config in PAK_ONLY_BUILD_CONFIGS:
        input_asset_mode = ASSET_MODE_PAK

    logging.info("Starting (%s) Asset Synchronization in %s mode and project %s", parsed_args.asset_type, input_asset_mode, parsed_args.project_path)
    start_time = timeit.default_timer()
    ASSET_SYNC_MODE_FUNCTION[input_asset_mode](target_platform=parsed_args.platform,
                                               project_path=parsed_args.project_path,
                                               asset_type=parsed_args.asset_type,
                                               warning_on_missing_assets=parsed_args.warn_on_missing_assets,
                                               layout_target=os.path.normpath(parsed_args.layout_root),
                                               override_pak_folder=parsed_args.override_pak_folder,
                                               copy=parsed_args.copy)
    duration = timeit.default_timer() - start_time
    logging.info("Asset Synchronization complete {:.2f} seconds".format(duration))

    # Remove broken symlinks/junctions to the layout folder
    if os.path.isdir(parsed_args.layout_root) and not os.path.exists(parsed_args.layout_root):
        remove_link(parsed_args.layout_root)

    if not os.path.isdir(parsed_args.layout_root):
        # If the layout target doesnt exist, check if we want to create it
        if parsed_args.create_layout_root:
            try:
                os.makedirs(parsed_args.layout_root, exist_ok=True)
            except OSError as e:
                raise common.LmbrCmdError("Unable to create layout folder '{}': {}".format(e,
                                                                                           parsed_args.layout_root),
                                          common.ERROR_CODE_ERROR_DIRECTORY)
        else:
            raise common.LmbrCmdError("Invalid layout folder (--layout-root): '{}'".format(parsed_args.layout_root),
                                      common.ERROR_CODE_ERROR_DIRECTORY)

    if parsed_args.verify:
        warnings = verify_layout(layout_dir=os.path.normpath(parsed_args.layout_root),
                                 platform_name=parsed_args.platform,
                                 project_path=parsed_args.project_path,
                                 asset_mode=input_asset_mode,
                                 asset_type=parsed_args.asset_type)
        if warnings > 0:
            if parsed_args.fail_on_warning:
                raise common.LmbrCmdError(f"Layout verification failed: {warnings} warnings.")
            logging.warning("%d layout warnings", warnings)


if __name__ == '__main__':

    try:
        main(sys.argv[1:])
        exit(0)

    except common.LmbrCmdError as err:
        print(str(err), file=sys.stderr)
        exit(err.code)
