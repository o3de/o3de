
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
set(no_op_compiler "${CMAKE_CURRENT_LIST_DIR}/no-op.cmd") # a utility that always returns 0, meaning, "success"

set(CMAKE_SYSTEM_PROCESSOR "x86_64") # there is no AARCH64 version of the 'no compile toolchain' and this is not set automatically
set(CMAKE_C_COMPILER "${no_op_compiler}")
set(CMAKE_CXX_COMPILER "${no_op_compiler}")
set(CMAKE_AR "${no_op_compiler}")
set(CMAKE_RANLIB "${no_op_compiler}")
set(CMAKE_C_COMPILER_ID "MSVC")
set(CMAKE_CXX_COMPILER_ID "MSVC")
set(CMAKE_C_COMPILER_VERSION 19.31)
set(CMAKE_CXX_COMPILER_VERSION 19.31)

include(${CMAKE_CURRENT_LIST_DIR}/../Common/Toolchain_scriptonly_common.cmake)
