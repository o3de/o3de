/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformIncl.h> // windows.h is included by d3d12.h so include PlatformIncl.h to keep windows.h specific defines consistent
#include <d3d12.h>

namespace AZ
{
    namespace DX12
    {
        static constexpr const char* APINameString = "dx12";
        static const RHI::APIType RHIType(APINameString);
    }
}
