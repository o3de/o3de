#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

o3de_pal_dir(pal_source_dir ${CMAKE_CURRENT_LIST_DIR}/Source/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

ly_add_target(
    NAME InAppPurchases.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        inapppurchases_files.cmake
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}_files.cmake
    PLATFORM_INCLUDE_FILES
        ${pal_source_dir}/platform_${PAL_PLATFORM_NAME_LOWERCASE}.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PUBLIC
            Legacy::CryCommon
)

ly_add_target(
    NAME InAppPurchases ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        inapppurchases_shared_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            Source
        PUBLIC
            Include
    BUILD_DEPENDENCIES
        PRIVATE
            Gem::InAppPurchases.Static
)
# Load the above "Gem::InAppPurchases" module in all app types
ly_create_alias(NAME InAppPurchases.Clients  NAMESPACE Gem TARGETS Gem::InAppPurchases)
ly_create_alias(NAME InAppPurchases.Servers  NAMESPACE Gem TARGETS Gem::InAppPurchases)
ly_create_alias(NAME InAppPurchases.Builders NAMESPACE Gem TARGETS Gem::InAppPurchases)
ly_create_alias(NAME InAppPurchases.Tools    NAMESPACE Gem TARGETS Gem::InAppPurchases)
