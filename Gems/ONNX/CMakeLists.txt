#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(gem_path ${CMAKE_CURRENT_LIST_DIR})
set(gem_json ${gem_path}/gem.json)

add_compile_definitions("ENABLE_CUDA=true") # Change to ENABLE_CUDA=false to disable CUDA inferencing

add_compile_definitions("W_GEM_ASSETS_PATH=L\"${CMAKE_CURRENT_LIST_DIR}/Assets\"" "GEM_ASSETS_PATH=\"${CMAKE_CURRENT_LIST_DIR}/Assets\"")

o3de_restricted_path(${gem_json} gem_restricted_path gem_parent_relative_path)

o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

ly_add_external_target_path(${CMAKE_CURRENT_LIST_DIR}/3rdParty)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/External")

add_subdirectory(Code)
