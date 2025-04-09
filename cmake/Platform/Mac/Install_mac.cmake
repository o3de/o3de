#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

include(cmake/Platform/Common/Install_common.cmake)

# This is used to generate a setreg file which will be placed inside the bundle
# for targets that request it(eg. AssetProcessor/Editor). This is the relative path
# to the bundle from the installed engine's root. This will be used to compute the
# absolute path to bundle which may contain dependent dylibs(eg. Gems) used by a project.
set(installed_binaries_path_template [[
{
    "Amazon": {
        "AzCore": {
            "Runtime": {
                "FilePaths": {
                    "InstalledBinariesFolder": "@runtime_output_directory@"
                }
            }
        }
    }
}]]
)

# This setreg file will be used by all of our installed app bundles to locate installed
# runtime dependencies. It contains the path to binary install directory relative to
# the installed engine root.
string(CONFIGURE "${installed_binaries_path_template}" configured_setreg_file)
file(GENERATE
    OUTPUT ${CMAKE_BINARY_DIR}/runtime_install/$<CONFIG>/BinariesInstallPath.setreg
    CONTENT "${configured_setreg_file}"
)

# ly_install_run_script isn't defined yet so we use install(SCRIPT) directly.
# This needs to be done here because it needs to update the install prefix
# before cmake does anything else in the install process.
configure_file(${LY_ROOT_FOLDER}/cmake/Platform/Mac/PreInstallSteps_mac.cmake.in ${CMAKE_BINARY_DIR}/runtime_install/PreInstallSteps_mac.cmake @ONLY)
ly_install(SCRIPT ${CMAKE_BINARY_DIR}/runtime_install/PreInstallSteps_mac.cmake COMPONENT ${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME})

#! ly_setup_target_install_targets_override: Mac specific target installation
function(ly_setup_target_install_targets_override)

    set(options)
    set(oneValueArgs TARGET ARCHIVE_DIR LIBRARY_DIR RUNTIME_DIR LIBRARY_SUBDIR RUNTIME_SUBDIR)
    set(multiValueArgs)
    cmake_parse_arguments(ly_platform_install_target "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(target_name "${ly_platform_install_target_TARGET}")

    # For bundles on Mac, we set the icons by passing in a path to the Images.xcassets directory.
    # However, the CMake install command expects paths to files for the RESOURCE property.
    # More details can be found in the CMake issue: https://gitlab.kitware.com/cmake/cmake/-/issues/22409
    get_property(is_bundle TARGET ${target_name} PROPERTY MACOSX_BUNDLE)
    if (is_bundle)
        get_target_property(cached_resources_dir ${target_name} RESOURCE)
        set_property(TARGET ${target_name} PROPERTY RESOURCE "")
        # Set the target_bundle_dir and target_bundle_content_dir variables to CONFIGURE into
        # the InstallUtils_mac.cmake.in template file
        set(target_bundle_dir "$<TARGET_BUNDLE_DIR:${target_name}>")
        set(target_bundle_content_dir "$<TARGET_BUNDLE_CONTENT_DIR:${target_name}>")
    endif()

    # Set the target_file_dir variables to CONFIGURE into
    # the InstallUtils_mac.cmake.in template file
    set(target_file_dir "$<TARGET_FILE_DIR:${target_name}>")

    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} UCONF)
        ly_install(TARGETS ${target_name}
            ARCHIVE
                DESTINATION ${ly_platform_install_target_ARCHIVE_DIR}
                COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
                CONFIGURATIONS ${conf}
            LIBRARY
                DESTINATION ${ly_platform_install_target_LIBRARY_DIR}/${ly_platform_install_target_LIBRARY_SUBDIR}
                COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
                CONFIGURATIONS ${conf}
            RUNTIME
                DESTINATION ${ly_platform_install_target_RUNTIME_DIR}/${ly_platform_install_target_RUNTIME_SUBDIR}
                COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
                CONFIGURATIONS ${conf}
            BUNDLE
                DESTINATION ${ly_platform_install_target_RUNTIME_DIR}/${ly_platform_install_target_RUNTIME_SUBDIR}
                COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
                CONFIGURATIONS ${conf}
            RESOURCE
                DESTINATION ${ly_platform_install_target_RUNTIME_DIR}/${ly_platform_install_target_RUNTIME_SUBDIR}
                COMPONENT ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
                CONFIGURATIONS ${conf}
        )
    endforeach()

    set(install_relative_binaries_path "${ly_platform_install_target_RUNTIME_DIR}/${ly_platform_install_target_RUNTIME_SUBDIR}")

    if (is_bundle)
        set_property(TARGET ${target_name} PROPERTY RESOURCE ${cached_resources_dir})
        set(runtime_output_filename "$<TARGET_FILE_NAME:${target_name}>.app")
    else()
        set(runtime_output_filename "$<TARGET_FILE_NAME:${target_name}>")
    endif()

    get_target_property(target_type ${target_name} TYPE)
    if(target_type IN_LIST LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS)
        get_target_property(entitlement_file ${target_name} ENTITLEMENT_FILE_PATH)
        if (NOT entitlement_file)
            set(entitlement_file "none")
        endif()

        ly_file_read(${LY_ROOT_FOLDER}/cmake/Platform/Mac/runtime_install_mac.cmake.in template_file)
        string(CONFIGURE "${template_file}" configured_template_file @ONLY)
        file(GENERATE
            OUTPUT ${CMAKE_BINARY_DIR}/runtime_install/$<CONFIG>/${target_name}.cmake
            CONTENT "${configured_template_file}"
        )
    endif()
endfunction()

#! ly_setup_runtime_dependencies_copy_function_override: Mac specific copy function to handle frameworks
function(ly_setup_runtime_dependencies_copy_function_override)

    configure_file(${LY_ROOT_FOLDER}/cmake/Platform/Mac/InstallUtils_mac.cmake.in ${CMAKE_BINARY_DIR}/runtime_install/InstallUtils_mac.cmake @ONLY)
    foreach(conf IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER ${conf} UCONF)
        ly_install(SCRIPT "${CMAKE_BINARY_DIR}/runtime_install/InstallUtils_mac.cmake"
            COMPONENT  ${LY_INSTALL_PERMUTATION_COMPONENT}_${UCONF}
        )
    endforeach()

endfunction()

#! ly_post_install_steps: Any additional platform specific post install steps
function(ly_post_install_steps)

    # On Mac, after CMake is done installing, the code signatures on all our built binaries will be invalid.
    # We need to now codesign each dynamic library, executable, and app bundle. It's specific to each target
    # because there could potentially be different entitlements for different targets.
    get_property(all_targets GLOBAL PROPERTY LY_ALL_TARGETS)
    foreach(alias_target IN LISTS all_targets)
        ly_de_alias_target(${alias_target} target)
        # Exclude targets that dont produce runtime outputs
        get_target_property(target_type ${target} TYPE)
        if(NOT target_type IN_LIST LY_TARGET_TYPES_WITH_RUNTIME_OUTPUTS)
            continue()
        endif()

        ly_install_run_script(${CMAKE_BINARY_DIR}/runtime_install/$<CONFIG>/${target}.cmake)
    endforeach()

    ly_install_run_code("
        ly_download_and_codesign_sdk_python()
        ly_codesign_sdk()
        set(CMAKE_INSTALL_PREFIX ${LY_INSTALL_PATH_ORIGINAL})
    ")

endfunction()
