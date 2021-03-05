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

