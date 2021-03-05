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