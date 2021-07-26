/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

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

        virtual void OnCompositionChanged() {}
    };

    typedef AZ::EBus<DependencyNotifications> DependencyNotificationBus;
}
