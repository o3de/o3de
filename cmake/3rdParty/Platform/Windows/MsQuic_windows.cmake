#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

if(LY_MONOLITHIC_GAME)
    set(MSQUIC_LIBS ${BASE_PATH}/lib/x64/msquic.lib)
else()
    set(MSQUIC_LIBS ${BASE_PATH}/lib/x64/msquic.lib)
    set(MSQUIC_RUNTIME_DEPENDENCIES ${BASE_PATH}/bin/x64/msquic.dll)
endif()
