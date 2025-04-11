#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# All platforms and configurations end up including this file
# Here we clean up the variables and add common compilation flags to all platforms. Each platform then will add their
# own definitions. Common configurations can be shared by moving those configurations to the "Common" folder and making
# each platform include them

# Clear all
set(O3DE_EXTRA_C_FLAGS ""       CACHE STRING "Additional C Compiler flags to apply globally")
set(O3DE_EXTRA_CXX_FLAGS ""     CACHE STRING "Additional Cxx Compiler flags to apply globally")
set(O3DE_EXTRA_LINK_OPTIONS ""  CACHE STRING "Additional link options to apply globally")

ly_set(CMAKE_C_FLAGS "${O3DE_EXTRA_C_FLAGS}")
ly_set(CMAKE_CXX_FLAGS "${O3DE_EXTRA_CXX_FLAGS}")
ly_set(LINK_OPTIONS "${O3DE_EXTRA_LINK_OPTIONS}")
foreach(conf ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${conf} UCONF)
    set(O3DE_EXTRA_C_FLAGS_${UCONF} ""       CACHE STRING "Additional C Compiler flags to add globally when compiling in ${conf}")
    set(O3DE_EXTRA_CXX_FLAGS_${UCONF} ""     CACHE STRING "Additional Cxx Compiler flags to add globally when compiling in ${conf}")
    set(O3DE_EXTRA_LINK_OPTIONS_${UCONF} ""  CACHE STRING "Additional link options to add globally when linking in ${conf}")
    ly_set(CMAKE_C_FLAGS_${UCONF} "${O3DE_EXTRA_C_FLAGS_${UCONF}}")
    ly_set(CMAKE_CXX_FLAGS_${UCONF} "${O3DE_EXTRA_CXX_FLAGS_${UCONF}}")
    ly_set(LINK_OPTIONS_${UCONF} "${O3DE_EXTRA_LINK_OPTIONS_${UCONF}}")
    ly_set(LY_BUILD_CONFIGURATION_TYPE_${UCONF} ${conf})
endforeach()

# Common configurations
ly_append_configurations_options(
    DEFINES
        # Since we disable exceptions, we need to define _HAS_EXCEPTIONS=0 so the STD does not add exception handling 
        _HAS_EXCEPTIONS=0
    DEFINES_DEBUG
        _DEBUG            # TODO: this should be able to removed since it gets added automatically by some compilation flags
        AZ_DEBUG_BUILD
        AZ_ENABLE_TRACING
        AZ_ENABLE_DEBUG_TOOLS
        AZ_BUILD_CONFIGURATION_TYPE="${LY_BUILD_CONFIGURATION_TYPE_DEBUG}"
    DEFINES_PROFILE
        _PROFILE
        AZ_PROFILE_BUILD
        NDEBUG
        AZ_ENABLE_TRACING
        AZ_ENABLE_DEBUG_TOOLS
        AZ_BUILD_CONFIGURATION_TYPE="${LY_BUILD_CONFIGURATION_TYPE_PROFILE}"
    DEFINES_RELEASE
        _RELEASE
        RELEASE
        AZ_RELEASE_BUILD
        NDEBUG
        AZ_BUILD_CONFIGURATION_TYPE="${LY_BUILD_CONFIGURATION_TYPE_RELEASE}"
)

# Ninja: parallel compile and link pool settings
if(CMAKE_GENERATOR MATCHES "Ninja")
    set(LY_PARALLEL_COMPILE_JOBS "" CACHE STRING "Number of compile jobs to use (Defaults to not set)")
    set(LY_PARALLEL_LINK_JOBS "" CACHE STRING "Number of link jobs to use (Defaults to not set)")

    if(LY_PARALLEL_COMPILE_JOBS)
        set_property(GLOBAL APPEND PROPERTY JOB_POOLS compile_job_pool=${LY_PARALLEL_COMPILE_JOBS})
        ly_set(CMAKE_JOB_POOL_COMPILE compile_job_pool)
    endif()
    if(LY_PARALLEL_LINK_JOBS)
        set_property(GLOBAL APPEND PROPERTY JOB_POOLS link_job_pool=${LY_PARALLEL_LINK_JOBS})
        ly_set(CMAKE_JOB_POOL_LINK link_job_pool)
    endif()
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE True)

include(CheckPIESupported)
check_pie_supported()

# Determine if lld is installed to use as a default linker by supported platforms/configurations
find_program(LLD_LINKER_INSTALLED lld)
