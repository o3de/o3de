#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

#! ly_install_target_override: Mac specific target installation
function(ly_install_target_override)

    set(options)
    set(oneValueArgs TARGET ARCHIVE_DIR LIBRARY_DIR RUNTIME_DIR LIBRARY_SUBDIR RUNTIME_SUBDIR)
    set(multiValueArgs)
    cmake_parse_arguments(ly_platform_install_target "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_property(install_component TARGET ${ly_platform_install_target_TARGET} PROPERTY INSTALL_COMPONENT)

    # For bundles on Mac, we set the icons by passing in a path to the Images.xcassets directory.
    # However, the CMake install command expects paths to files for the the RESOURCE property.
    # More details can be found in the CMake issue: https://gitlab.kitware.com/cmake/cmake/-/issues/22409
    get_target_property(is_bundle ${ly_platform_install_target_TARGET} MACOSX_BUNDLE)
    if (${is_bundle})
        get_target_property(cached_resources_dir ${ly_platform_install_target_TARGET} RESOURCE)
        set_property(TARGET ${ly_platform_install_target_TARGET} PROPERTY RESOURCE "")
    endif()
    
    install(
        TARGETS ${ly_platform_install_target_TARGET}
        ARCHIVE
            DESTINATION ${ly_platform_install_target_ARCHIVE_DIR}/${PAL_PLATFORM_NAME}/$<CONFIG>
            COMPONENT ${install_component}
        LIBRARY
            DESTINATION ${ly_platform_install_target_LIBRARY_DIR}/${PAL_PLATFORM_NAME}/$<CONFIG>/${ly_platform_install_target_LIBRARY_SUBDIR}
            COMPONENT ${install_component}
        RUNTIME
            DESTINATION ${ly_platform_install_target_RUNTIME_DIR}/${PAL_PLATFORM_NAME}/$<CONFIG>/${ly_platform_install_target_RUNTIME_SUBDIR}
            COMPONENT ${install_component}
        BUNDLE
            DESTINATION ${ly_platform_install_target_RUNTIME_DIR}/${PAL_PLATFORM_NAME}/$<CONFIG>/${ly_platform_install_target_RUNTIME_SUBDIR}
            COMPONENT ${install_component}
        RESOURCE
            DESTINATION ${ly_platform_install_target_RUNTIME_DIR}/${PAL_PLATFORM_NAME}/$<CONFIG>/${ly_platform_install_target_RUNTIME_SUBDIR}/
            COMPONENT ${install_component}
    )

    if (${is_bundle})
        set_property(TARGET ${ly_platform_install_target_TARGET} PROPERTY RESOURCE ${cached_resources_dir})
    endif()
endfunction()

include(cmake/Platform/Common/Install_common.cmake)
