/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZFRAMEWORK_EDITORENTITYEVENTS_H
#define AZFRAMEWORK_EDITORENTITYEVENTS_H

#include <AzCore/Math/Transform.h>

namespace AzFramework
{
    class DebugDisplayRequests;

    /**
     * Inherit the EditorEntityEvents interface to receive editor-time events on a non-editor
     * component.
     *
     * The preferred model is to implement a separate editor component inheriting from
     * AzToolsFramework::EditorComponentBase in order to provide richer in-editor functionality.
     * This is _specifically_ to accelerate teams willing to blend editor and runtime component code,
     * and/or activate components at edit time.
     */
    class EditorEntityEvents
    {
    public:

        AZ_RTTI(EditorEntityEvents, "{A6ECB561-1C69-4438-92E5-1CC4EC0C9E93}");

        virtual ~EditorEntityEvents() {}

        virtual void EditorInit(AZ::EntityId /*entityId*/) {}
        virtual void EditorActivate(AZ::EntityId /*entityId*/) {}
        virtual void EditorDeactivate(AZ::EntityId /*entityId*/) {}
        virtual void EditorDisplay(
            AZ::EntityId /*entityId*/, DebugDisplayRequests& /*displayInterface*/, const AZ::Transform& /*world*/) {}

        /**
        * This API allows a component associated with a primary asset to participate in drag and drop asset events without an editor counterpart
        */
        virtual void EditorSetPrimaryAsset(const AZ::Data::AssetId& /*assetId*/) {}
    };
} // namespace AzFramework

#endif // AZFRAMEWORK_EDITORENTITYEVENTS_H
