#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(pal_dir ${LY_ROOT_FOLDER}/LauncherGenerator/Platform/${PAL_PLATFORM_NAME})
include(${pal_dir}/LauncherUnified_traits_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
include(${LY_ROOT_FOLDER}/LauncherGenerator/launcher_generator.cmake)
