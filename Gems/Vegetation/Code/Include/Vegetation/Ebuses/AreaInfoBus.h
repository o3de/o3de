/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Aabb.h>

namespace Vegetation
{
    /**
    * This bus allows querying information about a vegetation area
    */
    class AreaInfoRequests : public AZ::ComponentBus
    {
    public:
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        //! to handle overlapping vegetation layers
        virtual AZ::u32 GetLayer() const = 0;

        //! to handle overlapping vegetation layers
        virtual AZ::u32 GetPriority() const = 0;

        //! Get bounds of volume affected by placement
        virtual AZ::Aabb GetEncompassingAabb() const = 0;

        //! Get number of generated objects
        virtual AZ::u32 GetProductCount() const = 0;

        //! support for tracking the number of changes applied to the area
        virtual AZ::u32 GetChangeIndex() const = 0;
    };

    typedef AZ::EBus<AreaInfoRequests> AreaInfoBus;
}
