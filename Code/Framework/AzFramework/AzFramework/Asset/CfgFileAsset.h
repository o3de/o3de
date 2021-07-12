/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzFramework/Asset/SimpleAsset.h>

namespace AzFramework
{
    class CfgFileAsset
    {
    public:
        AZ_TYPE_INFO(CfgFileAsset, "{117A80A5-206B-4D85-9445-33B446D94C35}")
        static const char* GetFileFilter()
        {
            return "*.cfg";
        }
    };
}
