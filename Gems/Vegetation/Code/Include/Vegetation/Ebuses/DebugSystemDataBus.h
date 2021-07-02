/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Component/ComponentBus.h>

namespace Vegetation
{
    struct DebugData
    {
        AZStd::atomic_int m_areaTaskQueueCount{ 0 };
        AZStd::atomic_int m_areaTaskActiveCount{ 0 };
    };

    class DebugSystemData
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using MutexType = AZStd::recursive_mutex;

        virtual DebugData* GetDebugData() { return nullptr; };
    };
    using DebugSystemDataBus = AZ::EBus<DebugSystemData>;
}
