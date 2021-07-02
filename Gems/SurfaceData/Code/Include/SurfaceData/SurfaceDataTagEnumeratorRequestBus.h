/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <SurfaceData/SurfaceDataTypes.h>

namespace SurfaceData
{
    /**
    * the EBus is used to request tags
    */
    class SurfaceDataTagEnumeratorRequests : public AZ::ComponentBus
    {
    public:
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        //tags are accumulated from all enumerators so implementers should not clear container
        virtual void GetInclusionSurfaceTags([[maybe_unused]] SurfaceTagVector& tags, [[maybe_unused]] bool& includeAll) const {}
        virtual void GetExclusionSurfaceTags([[maybe_unused]] SurfaceTagVector& tags) const {}
    };

    typedef AZ::EBus<SurfaceDataTagEnumeratorRequests> SurfaceDataTagEnumeratorRequestBus;
}
