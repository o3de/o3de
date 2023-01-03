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

namespace LmbrCentral
{
    /**
    * the EBus is used to notify about component/entity changes
    */      
    class DependencyNotifications : public AZ::ComponentBus
    {
    public: 
        //! allows multiple threads to call
        using MutexType = AZStd::recursive_mutex;

        //! Notification that something about the dependent entity has changed.
        virtual void OnCompositionChanged() {}

        //! Notification that something about a region from the dependent entity has changed.
        //! For backwards compatibility, the default implementation of this notification will call the previously-existing
        //! OnCompositionChanged(). Listeners that want a more granular understanding of the region that has changed should
        //! implement handlers for both events.
        virtual void OnCompositionRegionChanged([[maybe_unused]] const AZ::Aabb& dirtyRegion)
        {
            OnCompositionChanged();
        }
    };

    typedef AZ::EBus<DependencyNotifications> DependencyNotificationBus;
}
