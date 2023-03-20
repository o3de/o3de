/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityBus.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>

namespace Multiplayer
{
    /**
     * /brief Provides ImGui and debug draw hierarchy information at runtime.
     */
    class MultiplayerDebugHierarchyReporter
        : public AZ::EntitySystemBus::Handler
    {
    public:
        MultiplayerDebugHierarchyReporter();
        ~MultiplayerDebugHierarchyReporter() override;

        //! Main update loop.
        void OnImGuiUpdate();

        //! Draws hierarchy information over hierarchy root entities.
        void UpdateDebugOverlay();

        //! EntitySystemBus overrides.
        //! @{
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        void OnEntityDeactivated(const AZ::EntityId& entityId) override;
        //! @}

    private:
        AZ::ScheduledEvent m_updateDebugOverlay;

        AzFramework::DebugDisplayRequests* m_debugDisplay = nullptr;

        struct HierarchyRootInfo
        {
            NetworkHierarchyRootComponent* m_rootComponent = nullptr;
            NetworkHierarchyChangedEvent::Handler m_changedEvent;
            NetworkHierarchyLeaveEvent::Handler m_leaveEvent;
        };

        AZStd::unordered_map<NetworkHierarchyRootComponent*, HierarchyRootInfo> m_hierarchyRoots;
        void CollectHierarchyRoots();

        char m_statusBuffer[100] = {};

        float m_awarenessRadius = 1000.f;
    };
}
