/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/EBus/EBus.h>

#pragma once

namespace AzToolsFramework
{
    class EditorOnlyEntityComponentRequests
        : public AZ::ComponentBus
    {
    public:
        virtual bool IsEditorOnlyEntity() = 0;
        virtual void SetIsEditorOnlyEntity(bool isEditorOnly) = 0;
    };

    using EditorOnlyEntityComponentRequestBus = AZ::EBus<EditorOnlyEntityComponentRequests>;

    /**
     * This bus will notify handlers when an entity's "editor only" flag has changed
     */
    class EditorOnlyEntityComponentNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual~EditorOnlyEntityComponentNotifications() = default;

        virtual void OnEditorOnlyChanged(AZ::EntityId entityId, bool isEditorOnly) = 0;
    };

    using EditorOnlyEntityComponentNotificationBus = AZ::EBus<EditorOnlyEntityComponentNotifications>;

} // namespace AzToolsFramework

