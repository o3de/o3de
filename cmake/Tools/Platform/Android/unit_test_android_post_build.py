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

    android_post_build.copy_or_create_link(pathlib.Path(src_file_1.realpath()),
                                           pathlib.Path(tgt_file_1.realpath()))
    android_post_build.copy_or_create_link(pathlib.Path(src_level1.realpath()),
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
                                              target_layout_root=tgt_path,
                                              android_app_root_path=pathlib.Path(tmpdir.join('dst/android/app')),
                                              native_build_folder='o3de',
                                              build_config='profile')
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
                                          target_layout_root=tgt_path,
                                          android_app_root_path=pathlib.Path(tmpdir.join('dst/android/app')),
                                          native_build_folder='o3de',
                                          build_config='profile')

    validate_engine_android_pak = tmpdir.join('dst/android/app/assets/engine.json')
    assert pathlib.Path(validate_engine_android_pak.realpath()).exists()


def test_determine_intermediate_folder_from_compile_commands(tmpdir):
    test_intermediate_folder = "6g15r5p3"
    test_native_build_path = 'o3de'
    test_build_config = 'profile'

    test_compile_commands = f"""
    "directory": "D:/github/NewspaperDeliveryGame/build/android_loose/app/{test_native_build_path}/{test_build_config}/{test_intermediate_folder}/arm64-v8a",
    "command": "C:\\Tools\\android_sdk\\ndk\\25.2.9519653\\toolchains\\llvm\\prebuilt\\windows-x86_64\\bin\\clang++.exe --target=aarch64-none-linux-android31 --sysroot=C:/Tools/android_sdk/ndk/25.2.9519653/toolchains/llvm/prebuilt/windows-x86_64/sysroot -DANDROID -DAZ_BUILD_CONFIGURATION_TYPE=\\\"profile\\\" -DAZ_ENABLE_DEBUG_TOOLS -DAZ_ENABLE_TRACING -DAZ_MONOLITHIC_BUILD -DAZ_PROFILE_BUILD -DENABLE_TYPE_INFO -DLINUX -DLINUX64 -DMOBILE -DNDEBUG -DNDK_REV_MAJOR=25 -DNDK_REV_MINOR=2 -D_FORTIFY_SOURCE=2 -D_HAS_C9X -D_HAS_EXCEPTIONS=0 -D_LINUX -D_PROFILE -ID:/github/o3de/Code/Framework/AtomCore/. -ID:/github/o3de/Code/Framework/AzCore/. -ID:/github/o3de/Code/Framework/AzCore/Platform/Android -ID:/github/o3de/Code/Framework/AzCore/Platform/Common -isystem C:/Users/o3de/.o3de/3rdParty/packages/Lua-5.4.4-rev1-android/Lua/include -isystem C:/Users/o3de/.o3de/3rdParty/packages/RapidJSON-1.1.0-rev1-multiplatform/RapidJSON/include -isystem C:/Users/o3de/.o3de/3rdParty/packages/RapidXML-1.13-rev1-multiplatform/RapidXML/include -isystem C:/Users/o3de/.o3de/3rdParty/packages/zlib-1.2.11-rev5-android/zlib/include -isystem C:/Users/o3de/.o3de/3rdParty/packages/zstd-1.35-multiplatform/zstd/lib -isystem C:/Users/o3de/.o3de/3rdParty/packages/cityhash-1.1-multiplatform/cityhash/src  -fno-exceptions -fvisibility=hidden -fvisibility-inlines-hidden -Wall -Werror -Wno-inconsistent-missing-override -Wrange-loop-analysis -Wno-unknown-warning-option -Wno-parentheses -Wno-reorder -Wno-switch -Wno-undefined-var-template -femulated-tls -ffast-math -fno-aligned-allocation -fms-extensions -fno-aligned-allocation -stdlib=libc++  -O2 -g -fstack-protector-all -fstack-check -g -gdwarf-2 -fPIC -std=c++17 -o o3de\\Code\\Framework\\AtomCore\\CMakeFiles\\AtomCore.dir\\Unity\\unity_0_cxx.cxx.o -c D:\\github\\NewProject\\build\\android_loose\\app\\{test_native_build_path}\\{test_build_config}\\{test_intermediate_folder}\\arm64-v8a\\{test_native_build_path}\\Code\\Framework\\AtomCore\\CMakeFiles\\AtomCore.dir\\Unity\\unity_0_cxx.cxx",
    "file": "D:\\github\\NewProject\\build\\android_loose\\app\\{test_native_build_path}\\{test_build_config}\\{test_intermediate_folder}\\arm64-v8a\\{test_native_build_path}\\Code\\Framework\\AtomCore\\CMakeFiles\\AtomCore.dir\\Unity\\unity_0_cxx.cxx"
      """

    tmpdir.ensure(f'android/o3de/tools/profile/{android_post_build.ANDROID_ARCH}', dir=True)

    compile_commands_file= tmpdir.join(f'android/o3de/tools/profile/{android_post_build.ANDROID_ARCH}/compile_commands.json')
    compile_commands_file.write(test_compile_commands)

    result = android_post_build.determine_intermediate_folder_from_compile_commands(android_app_root_path=pathlib.Path(tmpdir.join('android').realpath()),
                                                                                    native_build_folder=test_native_build_path,
                                                                                    build_config=test_build_config)
    assert test_intermediate_folder == result
