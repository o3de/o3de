#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(LY_MONOLITHIC_GAME)
    set(PIX_LIBS ${BASE_PATH}/bin/x64/WinPixEventRuntime.lib)
else()
    set(PIX_LIBS ${BASE_PATH}/bin/x64/WinPixEventRuntime.lib)
    set(PIX_RUNTIME_DEPENDENCIES ${BASE_PATH}/bin/x64/WinPixEventRuntime.dll)
endif()
