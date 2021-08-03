#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    ../Common/Directory.Build.props
    ../Common/VisualStudio_common.cmake
    ../Common/Configurations_common.cmake
    ../Common/MSVC/Configurations_msvc.cmake
    ../Common/Install_common.cmake
    ../Common/LYWrappers_default.cmake
    ../Common/TargetIncludeSystemDirectories_unsupported.cmake
    Configurations_windows.cmake
    LYTestWrappers_windows.cmake
    LYWrappers_windows.cmake
    PAL_windows.cmake
    PALDetection_windows.cmake
    Install_windows.cmake
    Packaging_windows.cmake
    PackagingPostBuild.cmake
    Packaging/Bootstrapper.wxs
    Packaging/BootstrapperTheme.wxl.in
    Packaging/BootstrapperTheme.xml.in
    Packaging/Shortcuts.wxs
    Packaging/Template.wxs.in
)
