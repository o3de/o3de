#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(CMAKE_SYSTEM_PROCESSOR "x86_64") # there is no AARCH64 version of the 'no compile toolchain' and this is not set automatically
set(CMAKE_C_COMPILER "/usr/bin/true")
set(CMAKE_CXX_COMPILER "/usr/bin/true")
set(CMAKE_AR "/usr/bin/true")
set(CMAKE_RANLIB "/usr/bin/true")
set(CMAKE_C_COMPILER_ID "Clang")
set(CMAKE_CXX_COMPILER_ID "Clang")
set(CMAKE_C_COMPILER_VERSION 14.0.0)
set(CMAKE_CXX_COMPILER_VERSION 14.0.0)
include(${CMAKE_CURRENT_LIST_DIR}/../Common/Toolchain_scriptonly_common.cmake)
