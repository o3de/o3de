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

if(TARGET 3rdParty::Qt::Core) # Check we are not called multiple times
    return()
endif()

set(QT_PACKAGE_NAME Qt)
set(QT_PACKAGE_VERSION 5.15.1.2-az)

set(BASE_PATH "${LY_3RDPARTY_PATH}/${QT_PACKAGE_NAME}/${QT_PACKAGE_VERSION}")
if(NOT EXISTS ${BASE_PATH})
    message(FATAL_ERROR "Cannot find 3rdParty library ${QT_PACKAGE_NAME} on path ${BASE_PATH}")
endif()

set(QT5_COMPONENTS
    Core
    Concurrent
    Gui
    LinguistTools
    Network
    OpenGL
    Svg
    Test
    WebEngineWidgets
    Widgets
    Xml
)

include(${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}/Qt_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

list(APPEND CMAKE_PREFIX_PATH ${QT_LIB_PATH}/cmake/Qt5)

# Clear the cache for found DIRs
unset(Qt5_DIR CACHE)
foreach(component ${QT5_COMPONENTS})
    unset(Qt5${component}_DIR CACHE)
endforeach()
unset(Qt5Positioning_DIR CACHE)
unset(Qt5PrintSupport_DIR CACHE)
unset(Qt5WebChannel_DIR CACHE)
unset(Qt5WebEngineCore_DIR CACHE)

# Populate the Qt5 configurations
find_package(Qt5
    COMPONENTS ${QT5_COMPONENTS}
    REQUIRED
    NO_CMAKE_PACKAGE_REGISTRY 
)
mark_as_advanced(Qt5_DIR) # Hiding from GUI
mark_as_advanced(Qt5LinguistTools_DIR) # Hiding from GUI, this variable comes from the LinguistTools module

# Now create libraries that wrap the dependency so we can refer to them in our format
foreach(component ${QT5_COMPONENTS})
    if(TARGET Qt5::${component})

        get_target_property(system_includes Qt5::${component} INTERFACE_INCLUDE_DIRECTORIES)
        set_target_properties(Qt5::${component} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "") # Clear it in case someone refers to it

        ly_target_include_system_directories(TARGET Qt5::${component}
            INTERFACE "${system_includes}"
        )

        add_library(3rdParty::Qt::${component} INTERFACE IMPORTED)
        set_target_properties(3rdParty::Qt::${component} PROPERTIES
            INTERFACE_LINK_LIBRARIES Qt5::${component}
            INTERFACE_COMPILE_DEFINITIONS QT_NO_EXCEPTIONS
        )
        mark_as_advanced(Qt5${component}_DIR) # Hiding from GUI

    endif()
endforeach()

# Some extra DIR variables we want to hide from GUI
mark_as_advanced(Qt5Positioning_DIR)
mark_as_advanced(Qt5PrintSupport_DIR)
mark_as_advanced(Qt5WebChannel_DIR)
mark_as_advanced(Qt5WebEngineCore_DIR)

# Special case for Qt::Gui, we are using the private headers...
ly_target_include_system_directories(TARGET 3rdParty::Qt::Gui
   INTERFACE "${Qt5Gui_PRIVATE_INCLUDE_DIRS}"
)

# Another special case: Qt:Widgets, we are also using private headers
ly_target_include_system_directories(TARGET 3rdParty::Qt::Widgets
    INTERFACE "${Qt5Widgets_PRIVATE_INCLUDE_DIRS}"
)

# UIC executable
unset(QT_UIC_EXECUTABLE CACHE)
find_program(QT_UIC_EXECUTABLE uic HINTS "${QT_PATH}/bin")
mark_as_advanced(QT_UIC_EXECUTABLE) # Hiding from GUI

#! ly_qt_uic_target: handles qt's ui files by injecting uic generation
#
# AUTOUIC has issues to detect changes in UIC files and trigger regeneration:
# https://gitlab.kitware.com/cmake/cmake/-/issues/18741
# So instead, we are going to manually wrap the files. We dont use qt5_wrap_ui because
# it outputs to ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.h and we want to follow the
# same folder structure that AUTOUIC uses
#
function(ly_qt_uic_target TARGET)
    
    get_target_property(all_ui_sources ${TARGET} SOURCES)
    list(FILTER all_ui_sources INCLUDE REGEX "^.*\\.ui$")
    if(NOT all_ui_sources)
        message(FATAL_ERROR "Target ${TARGET} contains AUTOUIC but doesnt have any .ui file")
    endif()
    
    if(AUTOGEN_BUILD_DIR)
        set(gen_dir ${AUTOGEN_BUILD_DIR})
    else()
        set(gen_dir ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_autogen/include)
    endif()

    foreach(ui_source ${all_ui_sources})
        
        get_filename_component(filename ${ui_source} NAME_WE)
        get_filename_component(dir ${ui_source} DIRECTORY)
        if(IS_ABSOLUTE ${dir})
            file(RELATIVE_PATH dir ${CMAKE_CURRENT_SOURCE_DIR} ${dir})
        endif()

        set(outfolder ${gen_dir}/${dir})
        set(outfile ${outfolder}/ui_${filename}.h)
        get_filename_component(infile ${ui_source} ABSOLUTE)

        file(MAKE_DIRECTORY ${outfolder})
        add_custom_command(OUTPUT ${outfile}
          COMMAND ${QT_UIC_EXECUTABLE} -o ${outfile} ${infile}
          MAIN_DEPENDENCY ${infile} VERBATIM
          COMMENT "UIC ${infile}"
        )

        set_source_files_properties(${infile} PROPERTIES SKIP_AUTOUIC TRUE)
        set_source_files_properties(${outfile} PROPERTIES 
            SKIP_AUTOMOC TRUE
            SKIP_AUTOUIC TRUE
            GENERATED TRUE
        )
        list(APPEND all_ui_wrapped_sources ${outfile})

    endforeach()

    # Add files to the target
    target_sources(${TARGET} PRIVATE ${all_ui_wrapped_sources})
    source_group("Generated Files" FILES ${all_ui_wrapped_sources})

    # Add include directories relative to the generated folder
    # query for the property first to avoid the "NOTFOUND" in a list
    get_property(has_includes TARGET ${TARGET} PROPERTY INCLUDE_DIRECTORIES SET)
    if(has_includes)
        get_property(all_include_directories TARGET ${TARGET} PROPERTY INCLUDE_DIRECTORIES)
        foreach(dir ${all_include_directories})
            if(IS_ABSOLUTE ${dir})
                file(RELATIVE_PATH dir ${CMAKE_CURRENT_SOURCE_DIR} ${dir})
            endif()
            list(APPEND new_includes ${gen_dir}/${dir})
        endforeach()
    endif()
    list(APPEND new_includes ${gen_dir})
    target_include_directories(${TARGET} PRIVATE ${new_includes})

endfunction()
