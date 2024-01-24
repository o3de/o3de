#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(_cmake_package_name "cmake-${CPACK_DESIRED_CMAKE_VERSION}-linux-x86_64")
set(CPACK_CMAKE_PACKAGE_FILE "${_cmake_package_name}.tar.gz")
set(CPACK_CMAKE_PACKAGE_HASH "dc73115520d13bb64202383d3df52bc3d6bbb8422ecc5b2c05f803491cb215b0")

set(O3DE_INCLUDE_INSTALL_IN_PACKAGE FALSE CACHE BOOL "Option to copy the contents of the most recent install from CMAKE_INSTALL_PREFIX into CPACK_PACKAGING_INSTALL_PREFIX.  Useful for including a release build in a profile SDK.")

if("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "SNAP")

    set(CPACK_GENERATOR External)
    set(CPACK_EXTERNAL_ENABLE_STAGING YES)
    set(CPACK_EXTERNAL_PACKAGE_SCRIPT "${LY_ROOT_FOLDER}/cmake/Platform/${PAL_PLATFORM_NAME}/Packaging_Snapcraft.cmake")
    set(CPACK_MONOLITHIC_INSTALL 1)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/${CPACK_PACKAGE_NAME}/${CPACK_PACKAGE_VERSION}")

    if(O3DE_INCLUDE_INSTALL_IN_PACKAGE)
        # Snap uses the external packaging script folder so just copy the files
        # into the destination root
        set(CPACK_INSTALLED_DIRECTORIES "${CMAKE_INSTALL_PREFIX};.")
    endif()

elseif("$ENV{O3DE_PACKAGE_TYPE}" STREQUAL "DEB")

    set(CPACK_GENERATOR DEB)

    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${CPACK_PACKAGE_NAME}/${CPACK_PACKAGE_VERSION}")

    # Define all the debian package dependencies needed to build and run
    set(package_dependencies
        # Required Tools
        "cmake (>=3.22)"                        # Cmake required (minimum version 3.22.0)
        "clang (>=12.0)"                        # Clang required (minimum version 12.0)
        ninja-build
        # Build Libraries
        libglu1-mesa-dev                        # For Qt (GL dependency)
        libxcb-xinerama0                        # For Qt plugins at runtime
        libxcb-xinput0                          # For Qt plugins at runtime
        libfontconfig1-dev                      # For Qt plugins at runtime
        libxcb-xkb-dev                          # For xcb keyboard input
        libxkbcommon-x11-dev                    # For xcb keyboard input
        libxkbcommon-dev                        # For xcb keyboard input
        libxcb-xfixes0-dev                      # For mouse input
        libxcb-xinput-dev                       # For mouse input
        libpcre2-16-0
        zlib1g-dev
        mesa-common-dev
        libunwind-dev
        libzstd-dev
        pkg-config
    )
    list(JOIN package_dependencies "," CPACK_DEBIAN_PACKAGE_DEPENDS)

    # Post-installation and pre/post removal scripts
    configure_file("${LY_ROOT_FOLDER}/cmake/Platform/${PAL_PLATFORM_NAME}/Packaging/postinst.in"
        "${CMAKE_BINARY_DIR}/cmake/Platform/${PAL_PLATFORM_NAME}/Packaging/postinst"
        @ONLY
    )
    configure_file("${LY_ROOT_FOLDER}/cmake/Platform/${PAL_PLATFORM_NAME}/Packaging/prerm.in"
        "${CMAKE_BINARY_DIR}/cmake/Platform/${PAL_PLATFORM_NAME}/Packaging/prerm"
        @ONLY
    )
    configure_file("${LY_ROOT_FOLDER}/cmake/Platform/${PAL_PLATFORM_NAME}/Packaging/postrm.in"
        "${CMAKE_BINARY_DIR}/cmake/Platform/${PAL_PLATFORM_NAME}/Packaging/postrm"
        @ONLY
    )
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        ${CMAKE_BINARY_DIR}/cmake/Platform/Linux/Packaging/postinst 
        ${CMAKE_BINARY_DIR}/cmake/Platform/Linux/Packaging/prerm 
        ${CMAKE_BINARY_DIR}/cmake/Platform/Linux/Packaging/postrm
    )

    if(O3DE_INCLUDE_INSTALL_IN_PACKAGE)
        set(CPACK_INSTALLED_DIRECTORIES "${CMAKE_INSTALL_PREFIX};${CPACK_PACKAGING_INSTALL_PREFIX}")
    endif()

endif()

