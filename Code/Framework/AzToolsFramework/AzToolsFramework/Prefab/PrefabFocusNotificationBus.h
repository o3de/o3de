/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Entity/EntityContext.h>

namespace AzToolsFramework::Prefab
{
    //! Used to notify when the editor focus changes.
    class PrefabFocusNotifications
        : public AZ::EBusTraits
    {
    protected:
        ~PrefabFocusNotifications() = default;

    public:
        //! Triggered when the editor focus is changed to a different prefab.
        virtual void OnPrefabFocusChanged() = 0;
    };

    using PrefabFocusNotificationBus = AZ::EBus<PrefabFocusNotifications>;

} // namespace AzToolsFramework::Prefab
