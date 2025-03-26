#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if (TARGET 3rdParty::v-hacd)
  return()
endif()

add_library(3rdParty::v-hacd IMPORTED INTERFACE GLOBAL)

message(STATUS "PhysX gem uses v-hacd v4.1.0 (BSD-3-Clause) https://github.com/kmammou/v-hacd.git")
set(v-hacd_FOUND TRUE)
