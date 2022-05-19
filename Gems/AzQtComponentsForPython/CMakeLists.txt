cmake_minimum_required(VERSION 3.20)

#Temp: Modify this to match the location you unpacked the pyside package to.
set(pyside2_module_dir "c:/ly/3rd_party/packages/pyside2-qt-5.15.2-rev1-windows/pyside2")

# The name of the Python import module we're generating.
set(bindings_library "azqtpyside")

# Configuration - To be modified if types/classes are added/removed.

# Header file containing all the types/classes we're exposing to Python.
set(wrapped_header ${CMAKE_CURRENT_LIST_DIR}/bindings_azqtcomponents.h)

# Typesystem xml.
set(typesystem_file ${CMAKE_CURRENT_LIST_DIR}/bindings_azqtcomponents.xml)


# End configuration


# Do not use RPATH to affect install_name on macOS.
if(POLICY CMP0068)
  cmake_policy(SET CMP0068 NEW)
endif()

# Allow AUTOMOC/AUTOUIC on generated files.
if(POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW)
endif()

project(AzQtComponent_Bindings LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
find_package(Qt5 5.12 REQUIRED COMPONENTS Core Gui Widgets)

# The library we're creating bindings for.
set(azqtcomponents_library "AzQtComponents")

# Check we have a valid version of Python.
if(NOT python_interpreter)
    find_program(python_interpreter "python")
endif()

if(NOT python_interpreter)
    message(FATAL_ERROR "Error: Failed to locate Python installation.")
endif()

# Find the include directory for Python.
macro(find_python_includes out_python_includes_loc)
    execute_process(
        COMMAND ${python_interpreter} "${CMAKE_CURRENT_LIST_DIR}/utils/find_python_includes.py"
        OUTPUT_VARIABLE ${out_python_includes_loc}
    )
    
    if ("${${out_python_includes_loc}}" STREQUAL "")
        message(FATAL_ERROR "Error: Failed to locate Python include directory.")
    endif()
endmacro()

# Find the Python library and link flags.
macro(find_python_link_data out_python_link_data)
    execute_process(
        COMMAND ${python_interpreter} "${CMAKE_CURRENT_LIST_DIR}/utils/find_python_link_data.py"
        OUTPUT_VARIABLE ${out_python_link_data}
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
endmacro()

find_python_includes(python_include_dir)
find_python_link_data(python_linking_data)


# Find the paths for the pyside2/shiboken modules - TODO restore this when 3rd_party setup is sorted.
#ly_get_package_association("pyside2" pyside_package_name)
#ly_package_get_target_folder(${pyside_package_name} pyside2_DOWNLOAD_LOCATION)
#set(pyside2_module_dir ${pyside2_DOWNLOAD_LOCATION}/${pyside_package_name}/pyside2)




set(pyside2_path "${pyside2_module_dir}/PySide2")
set(pyside2_include_dir "${pyside2_path}/include")
set(pyside2_typesystem_path "${pyside2_path}/typesystems")
   
set(shiboken_generator_dir "${pyside2_module_dir}/shiboken2_generator")   
set(shiboken_generator_exe_path "${shiboken_generator_dir}/shiboken2${CMAKE_EXECUTABLE_SUFFIX}") 
if(NOT EXISTS ${shiboken_generator_exe_path})
    message(FATAL_ERROR "Shiboken executable not found at path: ${shiboken_generator_dir}")
endif()
  
set(shiboken_module_path "${pyside2_module_dir}/shiboken2")

set(shiboken_include_dir "${shiboken_generator_dir}/include")
set(shiboken_shared_libraries "${shiboken_module_path}/shiboken2.abi3.lib")

set(pyside2_shared_libraries "${pyside2_path}/pyside2.abi3.lib")

# Find the qt package location.
ly_get_package_association("Qt" qt_package_name)
ly_package_get_target_folder(${qt_package_name} qt_DOWNLOAD_LOCATION)
set(qt_module_dir ${qt_DOWNLOAD_LOCATION}/${qt_package_name}/qt)


set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH ${shiboken_module_path} ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

ly_download_associated_package(Qt) 

# MacOS: handle paths differently if Qt is being used from a framework.
get_target_property(QtCore_is_framework Qt5::Core FRAMEWORK)
if (QtCore_is_framework)
    get_target_property(qt_core_library_location Qt5::Core LOCATION)
    get_filename_component(qt_core_library_location_dir "${qt_core_library_location}" DIRECTORY)
    get_filename_component(lib_dir "${qt_core_library_location_dir}/../" ABSOLUTE)
    list(APPEND INCLUDES "--framework-include-paths=${lib_dir}")
endif()

# Add the pyside2 include subdirectories to the include path.
set(pyside2_additional_includes "")
foreach(INCLUDE_DIR ${pyside2_include_dir})
    list(APPEND pyside2_additional_includes "${INCLUDE_DIR}/QtCore")
    list(APPEND pyside2_additional_includes "${INCLUDE_DIR}/QtGui")
    list(APPEND pyside2_additional_includes "${INCLUDE_DIR}/QtWidgets")
endforeach()

# Define the shared library for which we will create bindings.
set_property(TARGET ${azqtcomponents_library} PROPERTY PREFIX "")

# Set up the options to pass to shiboken.
set(shiboken_options --generator-set=shiboken --enable-parent-ctor-heuristic
    --enable-pyside-extensions --enable-return-value-heuristic --use-isnull-as-nb_nonzero
    --avoid-protected-hack --language-level=c++17
    ${INCLUDES}
    -I${CMAKE_SOURCE_DIR}
    -I${CMAKE_CURRENT_LIST_DIR}/..
    -I"${LY_ROOT_FOLDER}/code/Framework/AzCore"
    -I"${LY_ROOT_FOLDER}/code/Framework/AzQtComponents"
    -I"${PROJECT_BINARY_DIR}/../AzQtComponents_autogen/include"
    -I"${LY_ROOT_FOLDER}/code/Framework/AzCore/Platform/${PAL_PLATFORM_NAME}"
    -I${pyside2_include_dir}
    -I"${shiboken_include_dir}"
    -I"${pyside2_include_dir}/QtWidgets"
    -I"${pyside2_include_dir}/QtCore"
    -I"${pyside2_include_dir}/QtGui"
    -I"${pyside2_path}/typesystems"   
    -I"${qt_module_dir}/include"
    -I"${qt_module_dir}/include/QtWidgets"
    -I"${qt_module_dir}/include/QtCore"
    -I"${qt_module_dir}/include/QtGui"
    -T"${pyside2_path}/typesystems"
    -T"${pyside2_path}"
    -T${shiboken_include_dir}
    --output-directory=${CMAKE_CURRENT_BINARY_DIR}
)

set(generated_sources_dependencies ${wrapped_header} ${typesystem_file})

# Dummy library to run the custom shiboken command.
# Shiboken needs to be run before the build process in the final bindings library
# which according to the cmake docs, is only possible in visual studio. On
# other platforms, the PRE_BUILD step happens directly before PRE_LINK.
ly_add_target(
    NAME ${bindings_library}.bindings STATIC
    NAMESPACE Gem
    FILES_CMAKE
        dummy_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/..
            ${pyside2_include_dir}
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzQtComponents
            Gem::EditorPythonBindings.Static
)

add_custom_command(TARGET ${bindings_library}.bindings
                    POST_BUILD
                    COMMAND ${shiboken_generator_exe_path} ${shiboken_options} ${wrapped_header} ${typesystem_file}
                    DEPENDS ${generated_sources_dependencies} 
                        Gem::AzQtComponentsForPython.WrappedFiles
                    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                    COMMENT "Running generator for ${typesystem_file}.")
                    
ly_add_target(
    NAME ${bindings_library} SHARED
    NAMESPACE Gem
    FILES_CMAKE
        wrapped_files.cmake
    INCLUDE_DIRECTORIES
        PRIVATE
            ${pyside2_additional_includes}
            ${pyside2_include_dir}
            ${python_include_dir}
            ${shiboken_include_dir}
            "${pyside2_include_dir}\\QtCore"
            "${pyside2_include_dir}\\QtGui"
            "${pyside2_include_dir}\\QtWidgets"
            "${pyside2_include_dir}\\QtQuick"
            "${pyside2_include_dir}\\QtNetwork"
            "${pyside2_include_dir}\\QtQml"
    BUILD_DEPENDENCIES
        PUBLIC
            Qt5::Widgets
            ${azqtcomponents_library}
            ${pyside2_shared_libraries}
            ${shiboken_shared_libraries}
            ${bindings_library}.bindings
)

# Building the wrapper files will not work with a unity build.
set_target_properties(${bindings_library} PROPERTIES UNITY_BUILD OFF)

# Disable various warnings in generated wrapper code.
#"conditional expression is constant"
#"unreferenced formal parameter"
#"declaration of 'x' hides previous local declaration."
#"declaration of 'x' hides class member."
target_compile_options(${bindings_library} PRIVATE /wd4127 /wd4100 /wd4456 /wd4458)

# Fix the name of the module.
set_property(TARGET ${bindings_library} PROPERTY PREFIX "")
set_property(TARGET ${bindings_library} PROPERTY OUTPUT_NAME
             "${bindings_library}${PYTHON_EXTENSION_SUFFIX}")
if(WIN32)
    set_property(TARGET ${bindings_library} PROPERTY SUFFIX ".pyd")
endif()

# Make sure the linker doesn't complain about not finding Python symbols on MacOS.
if(APPLE)
  set_target_properties(${bindings_library} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif(APPLE)

# Find and link to the python import library only on Windows.
# On Linux and macOS, the undefined symbols will get resolved by the dynamic linker
# (the symbols will be picked up in the Python executable).
if (WIN32)
    list(GET python_linking_data 0 python_libdir)
    list(GET python_linking_data 1 python_lib)
    find_library(python_link_flags ${python_lib} PATHS ${python_libdir} HINTS ${python_libdir})
    target_link_libraries(${bindings_library} PRIVATE ${python_link_flags})
endif()

set(windows_shiboken_shared_libraries)

if(WIN32)
    # Ensure linking doesn't happen against the wrong libraries during #pragma comment(lib)
    set(python_additional_link_flags "/NODEFAULTLIB:\"python37.lib\" /NODEFAULTLIB:\"python37_d.lib\"")
    set(python_additional_link_flags ${python_additional_link_flags} "/NODEFAULTLIB:\"python3.lib\" /NODEFAULTLIB:\"python3_d.lib\"")

    set_target_properties(${bindings_library} PROPERTIES LINK_FLAGS "${python_additional_link_flags}")
endif()

install(FILES ${windows_shiboken_shared_libraries} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Temp: Copy dependencies.
set(runtime_output_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")

add_custom_command(TARGET ${bindings_library} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${qt_module_dir}/bin/Qt5Qml.dll"
    ${runtime_output_directory})
add_custom_command(TARGET ${bindings_library} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${qt_module_dir}/bin/Qt5Quick.dll"
    ${runtime_output_directory})
add_custom_command(TARGET ${bindings_library} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${qt_module_dir}/bin/Qt5QmlModels.dll"
    ${runtime_output_directory})
add_custom_command(TARGET ${bindings_library} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${qt_module_dir}/bin/Qt5Core.dll"
    ${runtime_output_directory})
add_custom_command(TARGET ${bindings_library} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${qt_module_dir}/bin/Qt5Widgets.dll"
    ${runtime_output_directory})
add_custom_command(TARGET ${bindings_library} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${qt_module_dir}/bin/Qt5Gui.dll"
    ${runtime_output_directory})


        