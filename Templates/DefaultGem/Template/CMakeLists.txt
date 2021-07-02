# {BEGIN_LICENSE}
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
# {END_LICENSE}

set(o3de_gem_path ${CMAKE_CURRENT_LIST_DIR})
set(o3de_gem_json ${o3de_gem_path}/gem.json)
o3de_read_json_key(o3de_gem_name ${o3de_gem_json} "gem_name")
o3de_restricted_path(${o3de_gem_json} o3de_gem_restricted_path)

# Currently we are in the DefaultProjectSource folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: ly_get_list_relative_pal_filename will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here: DefaultProjectSource/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/DefaultProjectSource
ly_get_list_relative_pal_filename(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} ${o3de_gem_restricted_path} ${o3de_gem_path} ${o3de_gem_name})

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# project cmake for this platform.
include(${pal_dir}/${PAL_PLATFORM_NAME_LOWERCASE}_gem.cmake)

ly_add_external_target_path(${CMAKE_CURRENT_LIST_DIR}/3rdParty)

add_subdirectory(Code)
