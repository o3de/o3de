#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(LY_MONOLITHIC_GAME FALSE CACHE BOOL "Indicates if the game will be built monolithically (other targets are not supported)")

if(LY_MONOLITHIC_GAME)
    add_compile_definitions(AZ_MONOLITHIC_BUILD)
    ly_set(PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE STATIC)
    ly_set(PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE GEM_STATIC)
    # Disable targets that are not supported with monolithic
    ly_set(PAL_TRAIT_BUILD_HOST_TOOLS FALSE)
    ly_set(PAL_TRAIT_BUILD_HOST_GUI_TOOLS FALSE)
    ly_set(PAL_TRAIT_BUILD_TESTS_SUPPORTED FALSE)
else()
    ly_set(PAL_TRAIT_MONOLITHIC_DRIVEN_LIBRARY_TYPE SHARED)
    ly_set(PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE GEM_MODULE)
endif()