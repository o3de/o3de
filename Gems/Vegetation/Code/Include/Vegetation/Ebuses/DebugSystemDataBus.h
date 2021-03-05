/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
