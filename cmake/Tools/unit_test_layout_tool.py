#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#


import hashlib
import os
import pytest
import shutil
import subprocess
import sys
import tempfile

ROOT_DEV_PATH = os.path.realpath(os.path.join(os.path.dirname(__file__), '..', '..'))
if ROOT_DEV_PATH not in sys.path:
    sys.path.append(ROOT_DEV_PATH)

from cmake.Tools import common, layout_tool


def test_copy_asset_files_to_layout_success():
    
    # Mock functions and preserve the originals to restore
    old_os_listdir = os.listdir
    old_os_path_isdir = os.path.isdir
    old_os_path_isfile = os.path.isfile
    old_common_filefingerprint = common.file_fingerprint
    old_shutil_copy2 = shutil.copy2

    try:
        # Setup test vectors
        
        # Denied files, should not show up in the result
        test_denylist_file = [
            'assetprocessorplatformconfig.setreg'
        ]
        # System files that are not the same platform, so should skip
        test_skip_system_files = [
            'system_badplatform_pc',
            'system_badplatform_badplatform'
        ]
        # Source 'folders' will be skipped
        test_skip_source_folders = [
            'src_folder'
        ]
        # Destination 'folders' will be skipped
        test_skip_dest_is_folder = [
            'fake_dst_folder'
        ]
        # Skip files that are the same in the destination
        test_dest_same_as_src = [
            'dst_same_as_src'
        ]
        # COPY files that are the same in the destination
        test_dest_diff_as_src = [
            'dst_diff_as_src'
        ]
        # COPY files that are not in the destination
        test_src_not_in_dst = [
            'system_goodplatform_pc',
            'good_src_1',
            'good_src_2'
        ]
        test_expected_copied_files = test_dest_diff_as_src + test_src_not_in_dst

        test_game_asset_folder = 'game_cache'
        test_layout_target = 'layout_target'
        test_platform = 'goodplatform'

        def _mock_os_listdir(path):
            assert path == test_game_asset_folder
            mock_files = test_denylist_file + \
                         test_skip_system_files + \
                         test_skip_source_folders + \
                         test_skip_dest_is_folder + \
                         test_dest_same_as_src + \
                         test_dest_diff_as_src + \
                         test_src_not_in_dst
            return mock_files
        os.listdir = _mock_os_listdir
        
        def _mock_os_path_isdir(path):
            basename = os.path.basename(path)
            if basename in test_skip_source_folders:
                return True
            if basename in test_skip_dest_is_folder:
                return True
            return False
        os.path.isdir = _mock_os_path_isdir
        
        def _mock_os_path_isfile(path):
            basename = os.path.basename(path)
            if basename in test_dest_same_as_src:
                return True
            if basename in test_dest_diff_as_src:
                return True
            return False
        os.path.isfile = _mock_os_path_isfile

        def _mock_common_file_fingerprint(path):
            basename = os.path.basename(path)
            dirname = os.path.dirname(path)
            if basename in test_dest_same_as_src:
                return "SOURCE_FINGERPRINT"
            elif basename in test_dest_diff_as_src:
                if dirname == test_game_asset_folder:
                    return "SOURCE_FINGERPRINT"
                else:
                    return "TARGET_FINGERPRINT"
            else:
                assert False
        common.file_fingerprint = _mock_common_file_fingerprint
        
        result_copy_files = []

        def _mock_shutil_copy2(src, dst):
            assert os.path.basename(src) == os.path.basename(dst)
            basename = os.path.basename(dst)
            result_copy_files.append(basename)
        shutil.copy2 = _mock_shutil_copy2

        layout_tool.copy_asset_files_to_layout(project_asset_folder=test_game_asset_folder,
                                               target_platform=test_platform,
                                               layout_target=test_layout_target)
        
        assert len(test_expected_copied_files) == len(result_copy_files)
        for expected_copied_file in test_expected_copied_files:
            assert expected_copied_file in result_copy_files

    finally:
        os.listdir = old_os_listdir
        os.path.isdir = old_os_path_isdir
        os.path.isfile = old_os_path_isfile
        common.file_fingerprint = old_common_filefingerprint
        shutil.copy2 = old_shutil_copy2


def test_create_link_windows_success():
    old_platform = layout_tool.PLATFORM_NAME
    old_subprocess_check_call = subprocess.check_call
    try:
        layout_tool.PLATFORM_NAME = 'Windows'
        
        src = "test_src"
        dst = "test_dst"
        
        expected = ['cmd', '/c', 'mklink', '/J', dst, src]
        
        def _mock_subprocess_check_call(args, stdout=None, stderr=None):
            assert args == expected
        subprocess.check_call = _mock_subprocess_check_call

        layout_tool.create_link(src, dst, False)

    finally:
        layout_tool.PLATFORM_NAME = old_platform
        subprocess.check_call = old_subprocess_check_call


def test_create_link_mac_success():
    old_platform = layout_tool.PLATFORM_NAME
    old_subprocess_check_call = subprocess.check_call
    try:
        layout_tool.PLATFORM_NAME = 'Darwin'
        
        src = "test_src"
        dst = "test_dst"

        expected = ['ln', '-s', src, dst]

        def _mock_subprocess_check_call(args, stdout=None, stderr=None):
            assert args == expected
        subprocess.check_call = _mock_subprocess_check_call

        layout_tool.create_link(src, dst, False)
    finally:
        layout_tool.PLATFORM_NAME = old_platform
        subprocess.check_call = old_subprocess_check_call


def test_create_link_error():
    old_platform = layout_tool.PLATFORM_NAME
    old_subprocess_check_call = subprocess.check_call
    try:
        layout_tool.PLATFORM_NAME = 'Windows'
        
        src = "test_src"
        dst = "test_dst"
        
        def _mock_subprocess_check_call(args, stdout=None, stderr=None):
            raise subprocess.CalledProcessError(1, "Bad Call")
        
        subprocess.check_call = _mock_subprocess_check_call
        
        layout_tool.create_link(src, dst, False)
    
    except common.LmbrCmdError:
        pass
    else:
        assert False, "subprocess.CalledProcessError exception expected"
    
    finally:
        layout_tool.PLATFORM_NAME = old_platform
        subprocess.check_call = old_subprocess_check_call


@pytest.mark.parametrize(
    "project_path, asset_type, warn_on_missing, expected_result", [
        pytest.param('Foo', 'pc', False, 'Foo/Cache/pc'),
        pytest.param('Foo', 'pc', True, None),
        pytest.param('Foo', 'pc', True, None),
        pytest.param('Foo', 'pc', False, common.LmbrCmdError),
        pytest.param('Foo', 'pc', False, common.LmbrCmdError),
    ]
)
def test_construct_and_validate_cache_game_asset_folder_success(tmpdir, project_path, asset_type, warn_on_missing, expected_result):
    if isinstance(expected_result, str):
        expected_path_realpath = str(tmpdir.join(expected_result).realpath())
    elif expected_result == common.LmbrCmdError:
        expected_path_realpath = common.LmbrCmdError
    else:
        expected_path_realpath = None

    try:
        result = layout_tool.construct_and_validate_cache_project_asset_folder(project_path=project_path,
                                                                            asset_type=asset_type,
                                                                            warn_on_missing_project_cache=warn_on_missing)

        assert expected_result != common.LmbrCmdError, "Expecting an error result"
        if result == None:
            assert warn_on_missing, "Expecting a warn_on_missing==True if None is returned"
        elif isinstance(expected_result, str):
            assert os.path.normcase(result) == os.path.normcase(expected_path_realpath)
            
    except common.LmbrCmdError:
        assert expected_result == common.LmbrCmdError


@pytest.mark.parametrize(
    "existing_temp_vfs_folder, existing_gems_link, existing_game_link", [
        pytest.param(False, False, False),
        pytest.param(True, False, False),
        pytest.param(False, True, False),
        pytest.param(True, True, False),
        pytest.param(False, False, True),
        pytest.param(True, False, True),
        pytest.param(False, True, True),
        pytest.param(True, True, True)
    ]
)
def test_sync_layout_vfs_success(tmpdir, existing_temp_vfs_folder, existing_gems_link, existing_game_link):
    
    old_tempfile_gettempdir = tempfile.gettempdir
    old_create_link = layout_tool.create_link
    old_copy_asset_files_to_layout = layout_tool.copy_asset_files_to_layout
    old_rmdir = os.rmdir
    old_unlink = os.unlink
    
    try:
        # Simple Test Parameters
        test_engine_root = str(tmpdir.join('engine-root').realpath())
        test_project_path = str(tmpdir.join('Foo').realpath())
        test_project_name_lower = 'foo'
        test_target_platform = 'bogus'
        test_asset_type = 'pc'

        # Setup a test dev and game cache folder structure inside the temp folder

        # Capture relevant real paths in the temp folder so we can verify our assertions
        cache_game_folder = os.path.join(test_project_path, 'Cache')

        cache_game_folder_gems = os.path.join(cache_game_folder, test_asset_type, 'gems')

        layout_target_root_realpath = str(tmpdir.join('layout').realpath())
        layout_target_gems_realpath = os.path.join(layout_target_root_realpath, 'gems')
        layout_target_game_realpath = os.path.join(layout_target_root_realpath)
        
        # If we are optionally testing existing links in a layout folder, track the expected and actual rmdirs
        actual_rmdir_paths = set()
        expected_rmdir_paths = set()
        
        # The rmdir will serve as a wrapper to track the paths that are actually deleted
        def _mock_os_rmdir(path):
            actual_rmdir_paths.add(os.path.normcase(path))
            old_rmdir(path)
        def _mock_os_unlink(link):
            actual_rmdir_paths.add(os.path.normcase(link))

        if existing_gems_link:
            # Optionally make a dummy folder for the target layout for gems and add it to the expected folder to delete
            os.makedirs(layout_target_gems_realpath, exist_ok=False)
            expected_rmdir_paths.add(os.path.normcase(layout_target_gems_realpath))
            os.rmdir = _mock_os_rmdir
            os.unlink = _mock_os_unlink
            
        if existing_game_link:
            # Optionally make a dummy folder for the target layout for the game folder and add it to the expected folder to delete
            os.makedirs(layout_target_game_realpath, exist_ok=False)
            expected_rmdir_paths.add(os.path.normcase(layout_target_game_realpath))
            os.rmdir = _mock_os_rmdir
            os.unlink = _mock_os_unlink

        def _mock_gettempdir():
            # mock tempfile.gettempdir() to use tmpdir from pytest
            return str(tmpdir.realpath())
        tempfile.gettempdir = _mock_gettempdir

        # Predict the temp folder name
        hasher = hashlib.md5()
        hasher.update(test_project_path.encode('UTF-8'))
        result = hasher.hexdigest()
        tmp_folder_subfolder = 'ly-layout-{}'.format(result)
        test_layout_folder = str(tmpdir.join('{}/vfs/foo'.format(tmp_folder_subfolder)).realpath())
        test_layout_config_folder = str(tmpdir.join('{}/vfs/foo/config'.format(tmp_folder_subfolder)).realpath())
        test_override_pak_folder = ''
        
        if existing_temp_vfs_folder:
            # Optionally make a dummy folder for the temp vfs and add the test layout folder and its child config to the expected folders to delete
            os.makedirs(test_layout_config_folder, exist_ok=False)
            expected_rmdir_paths.add(os.path.normcase(test_layout_folder))
            expected_rmdir_paths.add(os.path.normcase(test_layout_config_folder))
            os.rmdir = _mock_os_rmdir
            
        mock_layout_tool_create_link_validation = {
            os.path.normcase(cache_game_folder_gems): os.path.normcase(layout_target_gems_realpath),
            os.path.normcase(test_layout_folder): os.path.normcase(layout_target_game_realpath)
        }

        def _mock_layout_tool_create_link(src, dst, copy):
            check_src = os.path.normcase(src)
            check_dst = os.path.normcase(dst)
            assert check_src in mock_layout_tool_create_link_validation, "Unexpected create link call to {}->{}".format(src, dst)
            assert mock_layout_tool_create_link_validation[check_src] == check_dst, "Assertion on create linked failed: {}->{}".format(src, dst)
            
        layout_tool.create_link = _mock_layout_tool_create_link

        def _mock_copy_asset_files_to_layout(project_path, project_asset_folder, target_platform, layout_target):
            # Validate the correct call to copy asset files
            assert target_platform == target_platform
            assert os.path.normcase(layout_target) == os.path.normcase(layout_target_root_realpath)
        layout_tool.copy_asset_files_to_layout = _mock_copy_asset_files_to_layout

        layout_tool.sync_layout_vfs(target_platform           = test_target_platform,
                                    project_path              = test_project_path,
                                    asset_type                = test_asset_type,
                                    warning_on_missing_assets = False,
                                    layout_target             = layout_target_root_realpath,
                                    override_pak_folder       = test_override_pak_folder,
                                    copy                      = False)

        # Verify if any the rmdir calls based on the test parameters
        assert actual_rmdir_paths == expected_rmdir_paths

    finally:
        tempfile.gettempdir = old_tempfile_gettempdir
        layout_tool.create_link = old_create_link
        layout_tool.copy_asset_files_to_layout = old_copy_asset_files_to_layout
        os.rmdir = old_rmdir
        os.unlink = old_unlink 


@pytest.mark.parametrize(
    "mode, existing_game_link, existing_gems_link, test_override_pak_folder", [
        pytest.param("LOOSE", False, False, None),
        pytest.param("LOOSE", False, True, None),
        pytest.param("LOOSE", True, False, None),
        pytest.param("LOOSE", True, True, None),
        pytest.param("PAK", False, None, None),
        pytest.param("PAK", True, None, None),
        pytest.param("PAK", False, None, 'override_paks'),
        pytest.param("PAK", True, None, 'override_paks')
    ]
)
def test_sync_layout_non_vfs_success(tmpdir, mode, existing_game_link, existing_gems_link, test_override_pak_folder):
    old_rmdir = os.rmdir
    old_copy_asset_files_to_layout = layout_tool.copy_asset_files_to_layout
    old_remove_link = layout_tool.remove_link
    try:
        # Simple Test Parameters
        engine_root_realpath = str(tmpdir.join('engine-root').realpath())
        test_project_path = str(tmpdir.join('Foo').realpath())
        test_project_name_lower = 'foo'
        test_target_platform = 'bogus'
        test_asset_type = 'pc'
        cache_game_folder_realpath = os.path.join(test_project_path, 'Cache')
        
        # Make sure a dummy layout folder is created
        tmpdir.ensure('layout/dummy.txt')
        test_layout_target_realpath = str(tmpdir.join('layout').realpath())
        test_layout_target_gems_realpath = os.path.join(test_layout_target_realpath, 'gems')
        test_layout_target_game_realpath = os.path.join(test_layout_target_realpath,)

        # If we are optionally testing existing links in a layout folder, track the expected and actual rmdirs
        actual_rmdir_paths = set()
        expected_rmdir_paths = set()
        
        def _mock_remove_link(link):
            actual_rmdir_paths.add(os.path.normcase(link))
        layout_tool.remove_link = _mock_remove_link 
        
        # The rmdir will serve as a wrapper to track the paths that are actually deleted
        def _mock_os_rmdir(path):
            actual_rmdir_paths.add(os.path.normcase(path))
            old_rmdir(path)

        if existing_game_link:
            # Optionally make a dummy folder for the target layout for the game folder and add it to the expected folder to delete
            os.makedirs(test_layout_target_game_realpath, exist_ok=False)
            expected_rmdir_paths.add(os.path.normcase(test_layout_target_game_realpath))
            os.rmdir = _mock_os_rmdir

        mock_layout_tool_create_link_validation = {}
        if mode == 'PAK':
            # In PAK Mode, the linking rules are slightly different. The 'game folder' link points to inside the pak folder, and there is no 'gems' link
            if test_override_pak_folder:
                test_game_asset_folder = os.path.join(engine_root_realpath, test_override_pak_folder,
                                                      f'{test_project_name_lower}_{test_asset_type}_paks')
                cache_game_folder_game_realpath = os.path.join(test_game_asset_folder)
            else:
                test_game_asset_folder = os.path.join(engine_root_realpath, 'Pak',
                                                      f'{test_project_name_lower}_{test_asset_type}_paks')
                cache_game_folder_game_realpath = os.path.join(test_game_asset_folder)

            mock_layout_tool_create_link_validation[os.path.normcase(cache_game_folder_game_realpath)] = os.path.normcase(test_layout_target_game_realpath)

        elif mode == "LOOSE":
            # In LOOSE Mode, both game and gems will be linked
    
            if existing_gems_link:
                # Optionally make a dummy folder for the target layout for gems and add it to the expected folder to delete
                os.makedirs(test_layout_target_gems_realpath, exist_ok=False)
                expected_rmdir_paths.add(os.path.normcase(test_layout_target_gems_realpath))
                os.rmdir = _mock_os_rmdir
    
            test_game_asset_folder = os.path.join(cache_game_folder_realpath, test_asset_type)
            cache_game_folder_gems_realpath = os.path.join(cache_game_folder_realpath, test_asset_type, 'gems')
            cache_game_folder_game_realpath = os.path.join(cache_game_folder_realpath, test_asset_type)
            mock_layout_tool_create_link_validation[os.path.normcase(cache_game_folder_gems_realpath)] = os.path.normcase(test_layout_target_gems_realpath)
            mock_layout_tool_create_link_validation[os.path.normcase(cache_game_folder_game_realpath)] = os.path.normcase(test_layout_target_game_realpath)

        else:
            assert False, "Invalid Mode {}".format(mode)
        os.makedirs(test_game_asset_folder, exist_ok=True)

        def _mock_copy_asset_files_to_layout(project_path, project_asset_folder, target_platform, layout_target):
            assert os.path.normcase(project_asset_folder) == os.path.normcase(test_game_asset_folder)
            assert target_platform == test_target_platform
            assert layout_target == test_layout_target_realpath
        layout_tool.copy_asset_files_to_layout = _mock_copy_asset_files_to_layout
        
        def _mock_layout_tool_create_link(src, dst, copy):
            check_src = os.path.normcase(src)
            check_dst = os.path.normcase(dst)
            assert check_src in mock_layout_tool_create_link_validation, "Unexpected create link call to {}->{}".format(src, dst)
            assert mock_layout_tool_create_link_validation[check_src] == check_dst, "Assertion on create linked failed: {}->{}".format(src, dst)
        layout_tool.create_link = _mock_layout_tool_create_link

        layout_tool.sync_layout_non_vfs(mode                        = mode,
                                        target_platform             = test_target_platform,
                                        project_path                = test_project_path,
                                        asset_type                  = test_asset_type,
                                        warning_on_missing_assets   = False,
                                        layout_target               = test_layout_target_realpath,
                                        override_pak_folder         = test_override_pak_folder,
                                        copy                        = False)
        
        assert actual_rmdir_paths == expected_rmdir_paths

        pass
    finally:
        os.rmdir = old_rmdir
        layout_tool.copy_asset_files_to_layout = old_copy_asset_files_to_layout
        layout_tool.remove_link = old_remove_link 
