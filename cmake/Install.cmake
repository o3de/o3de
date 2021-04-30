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

if(NOT INSTALLED_ENGINE)
    ly_get_absolute_pal_filename(pal_dir ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Platform/${PAL_PLATFORM_NAME})
    include(${pal_dir}/Install_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)
else()
    
    # Provide empty implementation so ly_add_target continues working
    function(ly_install_target ly_install_target_NAME)
    endfunction()

endif()