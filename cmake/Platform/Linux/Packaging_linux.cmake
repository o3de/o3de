#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${CPACK_PACKAGE_NAME}/${LY_VERSION_STRING}")

set(_cmake_package_name "cmake-${CPACK_DESIRED_CMAKE_VERSION}-linux-x86_64")
set(CPACK_CMAKE_PACKAGE_FILE "${_cmake_package_name}.tar.gz")
set(CPACK_CMAKE_PACKAGE_HASH "3f827544f9c82e74ddf5016461fdfcfea4ede58a26f82612f473bf6bfad8bfc2")

# get all the package dependencies, extracted from scripts\build\build_node\Platform\Linux\package-list.ubuntu-focal.txt
set(package_dependencies
    libffi7
    clang-12
    ninja-build
    # Build Libraries
    libglu1-mesa-dev                        # For Qt (GL dependency)
    libxcb-xinerama0                        # For Qt plugins at runtime
    libxcb-xinput0                          # For Qt plugins at runtime
    libfontconfig1-dev                      # For Qt plugins at runtime
    libcurl4-openssl-dev                    # For HttpRequestor
    # libsdl2-dev                             # for WWise/Audio
    libxcb-xkb-dev                          # For xcb keyboard input
    libxkbcommon-x11-dev                    # For xcb keyboard input
    libxkbcommon-dev                        # For xcb keyboard input
    libxcb-xfixes0-dev                      # For mouse input
    libxcb-xinput-dev                       # For mouse input
    zlib1g-dev
    mesa-common-dev
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
