#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(NOT INSTALLED_ENGINE)
    ly_get_absolute_pal_filename(pal_dir ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Platform/${PAL_PLATFORM_NAME})
    include(${pal_dir}/Install_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
endif()