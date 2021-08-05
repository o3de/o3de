
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace AzFramework
{
    struct CameraState;
}

namespace AzToolsFramework
{
    class EditorHelpers;
    class EditorVisibleEntityDataCache;

    namespace ViewportInteraction
    {
        struct MouseInteractionEvent;
    }
}

namespace PhysX
{
    class EditorViewportEntityPicker
        : private AzFramework::ViewportDebugDisplayEventBus::Handler
    {
    public:
        EditorViewportEntityPicker();
        ~EditorViewportEntityPicker();

        /// This function performs a simplified version of the entity picking feature in class EditorHelpers.
        AZ::EntityId PickEntity(
            const AzFramework::CameraState& cameraState
            , const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction
            , AZ::Vector3& pickPosition
            , AZ::Aabb& pickAabb);

    private:
        // AzToolsFramework::ViewportDebugDisplayEventBus
        void DisplayViewport(const AzFramework::ViewportInfo& viewportInfo
            , AzFramework::DebugDisplayRequests& debugDisplay) override;

        AZStd::unique_ptr<AzToolsFramework::EditorVisibleEntityDataCache> m_entityDataCache;
    };
} // namespace PhysX
