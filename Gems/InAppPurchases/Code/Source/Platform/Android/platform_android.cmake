#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

foreach(module_path ${CMAKE_MODULE_PATH})
    cmake_path(SET PALTools_cmake_path "${module_path}/PALTools.cmake")
    if(EXISTS "${PALTools_cmake_path}")
        include("${PALTools_cmake_path}")

        set(RELATIVE_PATH Source/Platform/Android) 

        set(JAVA_FILES
            java/com/amazon/lumberyard/iap/LumberyardInAppBilling.java
        )

        copy_java_to_build_dir(${RELATIVE_PATH} ${JAVA_FILES})

        return()
    endif()
endforeach()

message(FATAL_ERROR "Unable to find PALTools.cmake in ${CMAKE_MODULE_PATH}")
