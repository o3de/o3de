/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/vector.h>

namespace Vegetation
{
    /**
    * the EBus is used to query entity and asset dependencies
    */      
    class DependencyRequests : public AZ::ComponentBus
    {
    public: 
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        virtual void GetEntityDependencies(AZStd::vector<AZ::EntityId>& dependencies) const = 0;
        virtual void GetAssetDependencies(AZStd::vector<AZ::Data::AssetId>& dependencies) const = 0;
    };

    typedef AZ::EBus<DependencyRequests> DependencyRequestBus;
}
