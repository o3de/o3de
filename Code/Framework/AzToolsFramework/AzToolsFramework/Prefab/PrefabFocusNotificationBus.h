/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework::Prefab
{
    class PrefabFocusNotifications : public AZ::EBusTraits
    {
    public:
        virtual ~PrefabFocusNotifications() = default;

        virtual void OnPrefabFocusChanged() = 0;
    };

    using PrefabFocusNotificationBus = AZ::EBus<PrefabFocusNotifications>;

} // namespace AzToolsFramework::Prefab
