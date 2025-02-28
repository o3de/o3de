#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# If not set explicitly, look for a splash image in the project directory
if(NOT DEFINED SPLASH_FILE)
    # start by looking for file in the project directory
    if(EXISTS ${PROJECT_SOURCE_DIR}/Resources/Splash.png)
        set(SPLASH_FILE ${PROJECT_SOURCE_DIR}/Resources/Splash.png)
    endif()
    # if not found, assume it's engine centric build
    if(DEFINED LY_PROJECTS)
        if(EXISTS ${LY_PROJECTS}/Resources/Splash.png)
            set(SPLASH_FILE ${LY_PROJECTS}/Resources/Splash.png)
        endif()
    endif ()
endif()
# If value is set, check if it exists and generate the header file
if(DEFINED SPLASH_FILE)
    # check if xdd is available
    find_program(XXD xxd)
    if(NOT XXD)
        message(WARNING "xxd not found, no splash screen will be displayed"
                " (install xxd to create splash screen header file)")
        unset(SPLASH_FILE)
    else()
        file(TO_CMAKE_PATH ${SPLASH_FILE} SPLASH_FILE_RESOLVED_PATH)
        # check if it's a png file
        if(NOT ${SPLASH_FILE_RESOLVED_PATH} MATCHES ".*\\.png")
            message(WARNING "Splash file must be a PNG image, omitting splash screen generation.")
        else()
            if(EXISTS ${SPLASH_FILE_RESOLVED_PATH})
                execute_process(COMMAND ${CMAKE_COMMAND} -E copy ${SPLASH_FILE_RESOLVED_PATH} ${CMAKE_BINARY_DIR}/Splash.png)
                file(REMOVE ${CMAKE_BINARY_DIR}/Splash.h)
                add_definitions(-DSPLASH_IMAGE_EXISTS)
                execute_process(COMMAND xxd -i Splash.png Splash.h
                        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                )
                include_directories(${CMAKE_BINARY_DIR})
            else ()
                message(WARNING "${SPLASH_FILE} not found, no splash screen will be displayed"
                        " (set SPLASH_FILE to a valid path to a splash image to display it)")
            endif()
        endif()
    endif()
endif()
