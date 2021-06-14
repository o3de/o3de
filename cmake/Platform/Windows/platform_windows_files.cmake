#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
