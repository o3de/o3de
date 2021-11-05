#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(CPACK_GENERATOR DEB)

#set(CPACK_DEBIAN_PACKAGE_NAME ${CPACK_PACKAGE_NAME})

set(_cmake_package_name "cmake-${CPACK_DESIRED_CMAKE_VERSION}-linux-x86_64")
set(CPACK_CMAKE_PACKAGE_FILE "${_cmake_package_name}.tar.gz")
set(CPACK_CMAKE_PACKAGE_HASH "3f827544f9c82e74ddf5016461fdfcfea4ede58a26f82612f473bf6bfad8bfc2")
