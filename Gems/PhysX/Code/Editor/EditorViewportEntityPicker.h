
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
