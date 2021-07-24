#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

string(TIMESTAMP current_year "%Y")
set(LY_VERSION_COPYRIGHT_YEAR ${current_year} CACHE STRING "REngine copyright year")
set(LY_VERSION_STRING "0.0.0.0" CACHE STRING "REngine version")
set(LY_VERSION_BUILD_NUMBER 0 CACHE STRING "REngine build number")
set(LY_VERSION_ENGINE_NAME "REngine" CACHE STRING "REngine engine name")
