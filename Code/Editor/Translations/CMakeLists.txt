#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
if(NOT LY_I18N_BUILD)
    return()
endif()

find_package(Qt5 COMPONENTS LinguistTools REQUIRED)
set(QM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../Assets/Editor/Translations")
file(MAKE_DIRECTORY ${QM_DIR})
find_program(LRELEASE_EXECUTABLE lrelease PATHS "${Qt5_DIR}/../../../bin" REQUIRED)
set(LANGUAGES ${LY_I18N_LANGUAGE})
foreach(_language ${LANGUAGES})
    set(TS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${_language}")
    file(MAKE_DIRECTORY "${QM_DIR}/${_language}")
    file(GLOB TS_FILES "${TS_DIR}/*.ts")
    foreach(_ts_file ${TS_FILES})
        set(QM_NAME "")
        cmake_path(GET _ts_file STEM QM_NAME)
        execute_process(COMMAND ${LRELEASE_EXECUTABLE} ${_ts_file} -qm "${QM_DIR}/${_language}/${QM_NAME}.qm" -silent)
    endforeach()
endforeach()
