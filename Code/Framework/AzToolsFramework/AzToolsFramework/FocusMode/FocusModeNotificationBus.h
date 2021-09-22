/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    class FocusModeNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~FocusModeNotifications() = default;

        virtual void OnEditorFocusChanged(AZ::EntityId entityId) = 0;
    };

    using FocusModeNotificationBus = AZ::EBus<FocusModeNotifications>;

} // namespace AzToolsFramework
