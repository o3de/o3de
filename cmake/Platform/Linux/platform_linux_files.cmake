#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    ../Common/Configurations_common.cmake
    ../Common/Clang/Configurations_clang.cmake
    ../Common/Install_common.cmake
    ../Common/PackagingPostBuild_common.cmake
    ../Common/PackagingPreBuild_common.cmake
    ../Common/Toolchain_scriptonly_common.cmake
    CompilerSettings_linux.cmake
    Configurations_linux_aarch64.cmake
    Configurations_linux_x86_64.cmake
    Install_linux.cmake
    libzstd_linux.cmake
    LYTestWrappers_linux.cmake
    LYWrappers_linux.cmake
    Packaging_linux.cmake
    PackagingCodeSign_linux.cmake
    PackagingPostBuild_linux.cmake
    PackagingPreBuild_linux.cmake
    PAL_linux.cmake
    PALDetection_linux.cmake
    RPathChange.cmake
    runtime_dependencies_linux.cmake.in
    RuntimeDependencies_linux.cmake
    Toolchain_scriptonly_linux.cmake
)
