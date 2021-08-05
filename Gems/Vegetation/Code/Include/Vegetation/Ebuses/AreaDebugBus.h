/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>

namespace Vegetation
{
    struct AreaDebugDisplayData
    {
        AZ::Color m_instanceColor = AZ::Color::CreateOne();
        float m_instanceSize = 1.0f;
        bool m_instanceRender = true;
    };

    /**
    * This bus allows querying information about a vegetation area
    */
    class AreaDebugRequests : public AZ::ComponentBus
    {
    public:
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        //get default or configured base data
        virtual AreaDebugDisplayData GetBaseDebugDisplayData() const = 0;

        //blend data from multiple sources into accumulated display settings
        virtual void ResetBlendedDebugDisplayData() = 0;
        virtual void AddBlendedDebugDisplayData(const AreaDebugDisplayData& data) = 0;
        virtual AreaDebugDisplayData GetBlendedDebugDisplayData() const = 0;
    };

    typedef AZ::EBus<AreaDebugRequests> AreaDebugBus;
}
