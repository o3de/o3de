#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Use system default OpenSSL library instead of maintaining an O3DE version for Linux
find_package(OpenSSL)
if (NOT OpenSSL_FOUND)
   message(FATAL_ERROR "Compiling on linux requires the development headers for OpenSSL.  Try using your package manager to install the OpenSSL development libraries following https://wiki.openssl.org/index.php/Libssl_API")
else()
    # OpenSSL targets should be considered as provided by the system
    set_target_properties(OpenSSL::SSL OpenSSL::Crypto PROPERTIES LY_SYSTEM_LIBRARY TRUE)

    # Alias the O3DE name to the official name
    add_library(3rdParty::OpenSSL ALIAS OpenSSL::SSL)
endif()
