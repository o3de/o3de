#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    ../Common/Configurations_common.cmake
    ../Common/MSVC/Configurations_msvc.cmake
    ../Common/MSVC/CodeAnalysis.ruleset
    ../Common/MSVC/Directory.Build.props
    ../Common/MSVC/TestProject.props
    ../Common/MSVC/VisualStudio_common.cmake
    ../Common/Install_common.cmake
    ../Common/LYWrappers_default.cmake
    ../Common/PackagingPostBuild_common.cmake
    ../Common/PackagingPreBuild_common.cmake
    ../Common/TargetIncludeSystemDirectories_unsupported.cmake
    Configurations_windows.cmake
    LYTestWrappers_windows.cmake
    LYWrappers_windows.cmake
    PAL_windows.cmake
    PALDetection_windows.cmake
    Install_windows.cmake
    Packaging_windows.cmake
    PackagingCodeSign_windows.cmake
    PackagingPostBuild_windows.cmake
    PackagingPreBuild_windows.cmake
    Packaging/Bootstrapper.wxs
    Packaging/BootstrapperTheme.wxl.in
    Packaging/BootstrapperTheme.xml.in
    Packaging/Shortcuts.wxs
    Packaging/Template.wxs.in
)
