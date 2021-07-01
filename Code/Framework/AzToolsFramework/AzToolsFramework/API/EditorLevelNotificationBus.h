/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>

namespace AzToolsFramework
{
    class EditorLevelNotifications : public AZ::EBusTraits
    {
    public:
        virtual ~EditorLevelNotifications() = default;

        //! Invoked when a new level is created in the editor
        virtual void OnNewLevelCreated() {}
    };

    using EditorLevelNotificationBus = AZ::EBus<EditorLevelNotifications>;
}
