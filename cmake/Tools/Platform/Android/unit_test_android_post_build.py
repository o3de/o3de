#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

import pathlib
import platform
import shutil

from cmake.Tools.Platform.Android import android_post_build


def test_copy_or_create_link(tmpdir):

    tmpdir.ensure('src/level1', dir=True)

    src_file_l1 = tmpdir.join('src/level1/file_l1')
    src_file_l1.write('file_l1')
    src_file_l2 = tmpdir.join('src/level1/file_l2')
    src_file_l2.write('file_l2')
    src_file_1 = tmpdir.join('src/file_1')
    src_file_1.write('file_1')
    src_level1 = tmpdir.join('src/level1')

    tmpdir.ensure('tgt', dir=True)
    tgt_level1 = tmpdir.join('tgt/level1')
    tgt_file_1 = tmpdir.join('tgt/file_1')

    android_post_build.create_link(pathlib.Path(src_file_1.realpath()),
                                   pathlib.Path(tgt_file_1.realpath()))
    android_post_build.create_link(pathlib.Path(src_level1.realpath()),
                                   pathlib.Path(tgt_level1.realpath()))

    assert pathlib.Path(tgt_level1.realpath()).exists()
    assert pathlib.Path(tgt_file_1.realpath()).exists()


def test_safe_clear_folder(tmpdir):

    tmpdir.ensure('src/level1', dir=True)

    src_file1 = tmpdir.join('src/level1/file_l1')
    src_file1.write('file_l1')
    src_file2 = tmpdir.join('src/level1/file_l2')
    src_file2.write('file_l2')
    src_file3 = tmpdir.join('src/file_1')
    src_file3.write('file_1')
    src_level1 = tmpdir.join('src/level1')

    tmpdir.ensure('tgt', dir=True)
    tgt = tmpdir.join('tgt')
    tgt_level1 = tmpdir.join('tgt/level1')
    tgt_file1 = tmpdir.join('tgt/file_1')

    if platform.system() == "Windows":
        import _winapi
        _winapi.CreateJunction(str(src_level1.realpath()), str(tgt_level1.realpath()))
    else:
        pathlib.Path(src_level1.realpath()).symlink_to(tgt_level1.realpath())

    shutil.copy2(src_file3.realpath(), tgt_file1.realpath())

    target_path = pathlib.Path(tgt.realpath())
    android_post_build.safe_clear_folder(target_path)

    assert not pathlib.Path(tgt_level1.realpath()).exists()
    assert not pathlib.Path(tgt_file1.realpath()).exists()
    assert pathlib.Path(src_file1.realpath()).is_file()
    assert pathlib.Path(src_file2.realpath()).is_file()
    assert pathlib.Path(src_file3.realpath()).is_file()


def test_copy_folder_with_linked_directories(tmpdir):

    tmpdir.ensure('src/level1', dir=True)

    src_path = tmpdir.join('src')
    src_file_l1 = tmpdir.join('src/level1/file_l1')
    src_file_l1.write('file_l1')
    src_file_l2 = tmpdir.join('src/level1/file_l2')
    src_file_l2.write('file_l2')
    src_file_1 = tmpdir.join('src/file_1')
    src_file_1.write('file_1')

    tmpdir.ensure('tgt', dir=True)
    tgt_path = tmpdir.join('tgt')

    android_post_build.synchronize_folders(pathlib.Path(src_path.realpath()),
                                           pathlib.Path(tgt_path.realpath()))

    tgt_level1 = tmpdir.join('tgt/level1')
    tgt_file_1 = tmpdir.join('tgt/file_1')

    assert pathlib.Path(tgt_level1.realpath()).exists()
    assert pathlib.Path(tgt_file_1.realpath()).exists()


def test_apply_pak_layout_invalid_src_folder(tmpdir):

    tmpdir.ensure('src', dir=True)
    src_path = pathlib.Path(tmpdir.join('src').realpath())
    tmpdir.ensure('dst/android/app', dir=True)
    tgt_path = pathlib.Path(tmpdir.join('dst/android/app/assets').realpath())

    try:
        android_post_build.apply_pak_layout(project_root=src_path,
                                            asset_bundle_folder="Cache",
                                            target_layout_root=tgt_path)
    except android_post_build.AndroidPostBuildError as e:
        assert 'folder doesnt exist' in str(e)
    else:
        assert False, "Error expected"


def test_apply_pak_layout_no_engine_pak_file(tmpdir):
    tmpdir.ensure('src/Cache', dir=True)
    src_path = pathlib.Path(tmpdir.join('src').realpath())
    tmpdir.ensure('dst/android/app', dir=True)
    tgt_path = pathlib.Path(tmpdir.join('dst/android/app/assets').realpath())

    try:
        android_post_build.apply_pak_layout(project_root=src_path,
                                            asset_bundle_folder="Cache",
                                            target_layout_root=tgt_path)
    except android_post_build.AndroidPostBuildError as e:
        assert 'engine_android.pak' in str(e)
    else:
        assert False, "Error expected"


def test_apply_pak_layout_success(tmpdir):

    tmpdir.ensure('src/Cache', dir=True)
    src_path = pathlib.Path(tmpdir.join('src').realpath())

    test_engine_pak = tmpdir.join('src/Cache/engine_android.pak')
    test_engine_pak.write('engine')

    tmpdir.ensure('dst/android/app', dir=True)
    tgt_path = pathlib.Path(tmpdir.join('dst/android/app/assets').realpath())

    android_post_build.apply_pak_layout(project_root=src_path,
                                        asset_bundle_folder="Cache",
                                        target_layout_root=tgt_path)

    validate_engine_android_pak = tmpdir.join('dst/android/app/assets/engine_android.pak')
    assert pathlib.Path(validate_engine_android_pak.realpath()).exists()


def test_apply_loose_layout_no_loose_assets(tmpdir):

    tmpdir.ensure('src/Cache/android', dir=True)
    src_path = pathlib.Path(tmpdir.join('src').realpath())
    tmpdir.ensure('dst/android/app', dir=True)
    tgt_path = pathlib.Path(tmpdir.join('dst/android/app/assets').realpath())

    try:
        android_post_build.apply_loose_layout(project_root=src_path,
                                              target_layout_root=tgt_path)
    except android_post_build.AndroidPostBuildError as e:
        assert 'Assets have not been built' in str(e)
    else:
        assert False, "Error expected"


def test_apply_loose_layout_success(tmpdir):

    tmpdir.ensure('src/Cache/android', dir=True)
    src_path = pathlib.Path(tmpdir.join('src').realpath())

    test_engine_pak = tmpdir.join('src/Cache/android/engine.json')
    test_engine_pak.write('engine')

    tmpdir.ensure('dst/android/app', dir=True)
    tgt_path = pathlib.Path(tmpdir.join('dst/android/app/assets').realpath())

    android_post_build.apply_loose_layout(project_root=src_path,
                                          target_layout_root=tgt_path)

    validate_engine_android_pak = tmpdir.join('dst/android/app/assets/engine.json')
    assert pathlib.Path(validate_engine_android_pak.realpath()).exists()
