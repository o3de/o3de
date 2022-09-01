#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

cmake_minimum_required(VERSION 3.20)

# The name of the Python import module we're generating.
set(BINDINGS_LIBRARY "azqtpyside")

# Header file containing all the types/classes we're exposing to Python.
set(wrapped_header ${CMAKE_CURRENT_LIST_DIR}/bindings_azqtcomponents.h)

# Typesystem xml.
set(typesystem_file ${CMAKE_CURRENT_LIST_DIR}/bindings_azqtcomponents.xml)

# Allow AUTOMOC/AUTOUIC on generated files.
if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()

project(AzQtComponent_Bindings LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)
             
# The library we're creating bindings for.
set(AZQTCOMPONENTS_LIBRARY "AzQtComponents")

# Check we have a valid version of Python.
if(NOT python_interpreter)
    find_program(python_interpreter "python")
endif()

if(NOT python_interpreter)
    message(FATAL_ERROR "Error: Failed to locate Python installation.")
endif()

set(LIB_EXTENSION "cp310-win_amd64")

# Find the paths for the pyside2/shiboken modules.
ly_get_package_association("pyside2" PYSIDE_PACKAGE_NAME)
ly_package_get_target_folder(${PYSIDE_PACKAGE_NAME} PYSIDE2_DOWNLOAD_LOCATION)
set(PYSIDE2_MODULE_DIR ${PYSIDE2_DOWNLOAD_LOCATION}/${PYSIDE_PACKAGE_NAME})

# Find the qt package location.
ly_get_package_association("Qt" QT_PACKAGE_NAME)
ly_package_get_target_folder(${QT_PACKAGE_NAME} QT_DOWNLOAD_LOCATION)
set(QT_MODULE_DIR ${QT_DOWNLOAD_LOCATION}/${QT_PACKAGE_NAME}/qt)

set(PYSIDE2_PATH "${PYSIDE2_MODULE_DIR}/pyside2")
set(INCLUDE_DIR "${PYSIDE2_PATH}/include")
set(PYSIDE2_SHARE_PATH "${PYSIDE2_PATH}/share/PySide2")
set(PYSIDE2_TYPESYSTEM_PATH "${PYSIDE2_SHARE_PATH}/typesystems")
set(PYSIDE2_BIN_PATH "${PYSIDE2_PATH}/bin")
set(PYSIDE2_INCLUDE_DIR ${INCLUDE_DIR}/PySide2)
set(SHIBOKEN_GENERATOR_EXE_PATH "${PYSIDE2_BIN_PATH}/shiboken2${CMAKE_EXECUTABLE_SUFFIX}") 

if(NOT EXISTS ${SHIBOKEN_GENERATOR_EXE_PATH})
    message(FATAL_ERROR "Shiboken executable not found at path: ${SHIBOKEN_GENERATOR_EXE_PATH}")
endif()
  
set(SHIBOKEN_MODULE_PATH "${PYSIDE2_MODULE_DIR}/pyside2/lib/site-packages/shiboken2")

set(SHIBOKEN_INCLUDE_DIR "${INCLUDE_DIR}/shiboken2")
set(SHIBOKEN_SHARED_LIBRARIES "${SHIBOKEN_MODULE_PATH}/shiboken2.abi3.lib")
set(SHIBOKEN_DLL "${SHIBOKEN_MODULE_PATH}/shiboken2.abi3.dll")
set(SHIBOKEN_SHARED_LIBRARIES_D "${SHIBOKEN_MODULE_PATH}/shiboken2_d.${LIB_EXTENSION}.lib")
set(SHIBOKEN_DLL_D "${SHIBOKEN_MODULE_PATH}/shiboken2_d.${LIB_EXTENSION}.dll")

set(PYSIDE2_LIB_DIR ${PYSIDE2_PATH}/lib/site-packages/PySide2)

set(PYSIDE2_SHARED_LIBRARIES "${PYSIDE2_LIB_DIR}/pyside2.abi3.lib")
set(PYSIDE2_SHARED_LIBRARIES_D "${PYSIDE2_LIB_DIR}/pyside2_d.${LIB_EXTENSION}.lib")

set(PYSIDE2_DLL "${PYSIDE2_LIB_DIR}/pyside2.abi3.dll")
set(PYSIDE2_DLL_D "${PYSIDE2_LIB_DIR}/pyside2_d.${LIB_EXTENSION}.dll")
  
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH ${SHIBOKEN_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Define the shared library for which we will create bindings.
set_property(TARGET ${AZQTCOMPONENTS_LIBRARY} PROPERTY PREFIX "")

# Ensure the generated wrapper files have the correct header.
set(LICENSE_FILE ${CMAKE_CURRENT_LIST_DIR}/license_header.txt)

# Set up the options to pass to shiboken.
set(shiboken_options --generator-set=shiboken --enable-parent-ctor-heuristic
    --enable-pyside-extensions --enable-return-value-heuristic --use-isnull-as-nb_nonzero
    --avoid-protected-hack --language-level=c++17
    --license-file=${LICENSE_FILE}
    ${INCLUDES}
    -I${CMAKE_SOURCE_DIR}
    -I${CMAKE_CURRENT_LIST_DIR}/..
    -I"${LY_ROOT_FOLDER}/code/Framework/AzCore"
    -I"${LY_ROOT_FOLDER}/code/Framework/AzQtComponents"
    -I"${PROJECT_BINARY_DIR}/../AzQtComponents_autogen/include"
    -I"${LY_ROOT_FOLDER}/code/Framework/AzCore/Platform/${PAL_PLATFORM_NAME}"
    -I"${PYSIDE2_INCLUDE_DIR}"
    -I"${SHIBOKEN_INCLUDE_DIR}"
    -I"${PYSIDE2_INCLUDE_DIR}/QtWidgets"
    -I"${PYSIDE2_INCLUDE_DIR}/QtCore"
    -I"${PYSIDE2_INCLUDE_DIR}/QtGui"
    -I"${PYSIDE2_TYPESYSTEM_PATH}"   
    -I"${QT_MODULE_DIR}/include"
    -I"${QT_MODULE_DIR}/include/QtWidgets"
    -I"${QT_MODULE_DIR}/include/QtCore"
    -I"${QT_MODULE_DIR}/include/QtGui"
    -T"${PYSIDE2_PATH}/typesystems"
    -T"${PYSIDE2_PATH}"
    -T"${SHIBOKEN_INCLUDE_DIR}"
    -T"${PYSIDE2_TYPESYSTEM_PATH}"
    -T"${PYSIDE2_SHARE_PATH}"
    --output-directory=${CMAKE_CURRENT_BINARY_DIR}
)

set(generated_sources_dependencies ${wrapped_header} ${typesystem_file})

# Dummy library to run the custom shiboken command.
ly_add_target(
    NAME ${BINDINGS_LIBRARY}.bindings STATIC
    NAMESPACE Gem
    FILES_CMAKE
        dummy_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/..
            ${PYSIDE2_INCLUDE_DIR}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzQtComponents
            Gem::EditorPythonBindings.Static
)

# Add the custom shiboken command to the dummy library.
add_custom_command(TARGET ${BINDINGS_LIBRARY}.bindings
                    POST_BUILD
                    COMMAND ${SHIBOKEN_GENERATOR_EXE_PATH} ${shiboken_options} ${wrapped_header} ${typesystem_file}
                    DEPENDS ${generated_sources_dependencies} 
                        Gem::AzQtComponentsForPython.WrappedFiles
                    COMMENT "Running generator for ${typesystem_file}.")
    
# Create dummy wrapper files so the second project has something to work with while being created.
include(wrapped_files.cmake)
foreach(WRAPPER_FILE ${WRAPPED_FILES})
    if(NOT EXISTS "${WRAPPER_FILE}")
        file(WRITE ${WRAPPER_FILE} "")
    endif()
endforeach()
    
# Set up the project for building the code generated by shiboken.
ly_add_target(
    NAME ${BINDINGS_LIBRARY} SHARED
    NAMESPACE Gem
    FILES_CMAKE
        wrapped_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            ${PYSIDE2_INCLUDE_DIR}
            ${PYTHON_INCLUDE_DIR}
            ${SHIBOKEN_INCLUDE_DIR}
            "${PYSIDE2_INCLUDE_DIR}\\QtCore"
            "${PYSIDE2_INCLUDE_DIR}\\QtGui"
            "${PYSIDE2_INCLUDE_DIR}\\QtWidgets"
            "${PYSIDE2_INCLUDE_DIR}\\QtNetwork"
    BUILD_DEPENDENCIES
        PUBLIC
            Qt5::Widgets
            Qt5::Core
            Qt5::Widgets
            Qt5::Gui
            ${AZQTCOMPONENTS_LIBRARY}
            ${BINDINGS_LIBRARY}.bindings
)

target_link_libraries( ${BINDINGS_LIBRARY} PRIVATE 
    debug ${PYSIDE2_SHARED_LIBRARIES_D}
    optimized ${PYSIDE2_SHARED_LIBRARIES}
)

target_link_libraries( ${BINDINGS_LIBRARY} PRIVATE 
    debug ${SHIBOKEN_SHARED_LIBRARIES_D}
    optimized ${SHIBOKEN_SHARED_LIBRARIES}
)

target_sources(${BINDINGS_LIBRARY} PRIVATE ${WRAPPED_FILES})

# Building the wrapper files will not work with a unity build.
set_target_properties(${BINDINGS_LIBRARY} PROPERTIES UNITY_BUILD OFF)

# Append _d to the module name in a debug build.
set_target_properties(${BINDINGS_LIBRARY} PROPERTIES DEBUG_POSTFIX "_d")

# Disable various warnings in shiboken generated wrapper code.
#"conditional expression is constant"
#"unreferenced formal parameter"
#"declaration of 'x' hides previous local declaration."
#"declaration of 'x' hides class member."
target_compile_options(${BINDINGS_LIBRARY} PRIVATE /wd4127 /wd4100 /wd4456 /wd4458)

# Fix the name of the module.
set_property(TARGET ${BINDINGS_LIBRARY} PROPERTY PREFIX "")
set_property(TARGET ${BINDINGS_LIBRARY} PROPERTY OUTPUT_NAME
             "${BINDINGS_LIBRARY}${PYTHON_EXTENSION_SUFFIX}")
set_property(TARGET ${BINDINGS_LIBRARY} PROPERTY SUFFIX ".pyd")

# Ensure linking doesn't happen against the wrong libraries during #pragma comment(lib)
if (CMAKE_BUILD_TYPE STREQUAL "debug")
    set(python_additional_link_flags "/NODEFAULTLIB:\"python37_d.lib\"")
    set(python_additional_link_flags ${python_additional_link_flags} "/NODEFAULTLIB:\"python37_d.lib\"")
else()
    set(python_additional_link_flags "/NODEFAULTLIB:\"python37.lib\"")
    set(python_additional_link_flags ${python_additional_link_flags} "/NODEFAULTLIB:\"python37.lib\"")
endif()

set_target_properties(${BINDINGS_LIBRARY} PROPERTIES LINK_FLAGS "${python_additional_link_flags}")


        