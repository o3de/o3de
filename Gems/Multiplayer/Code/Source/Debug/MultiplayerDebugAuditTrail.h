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
    struct AuditTrailInput
    {
        ClientInputId inputId;
        HostFrameId hostFrameId;
        AZStd::string name;
        AZStd::vector<MultiplayerAuditingElement> children;
    };

    /**
     * /brief Provides ImGui driven UX for multiplayer audit trail
     */
    class MultiplayerDebugAuditTrail
    {
    public:
        MultiplayerDebugAuditTrail();
        ~MultiplayerDebugAuditTrail();

        //! Main update loop.
        void OnImGuiUpdate(AZStd::deque<AuditTrailInput> auditTrailElems);

        //! Draws hierarchy information over hierarchy root entities.
        void UpdateDebugOverlay();

    private:
        AZ::ScheduledEvent m_updateDebugOverlay;

        AzFramework::DebugDisplayRequests* m_debugDisplay = nullptr;
    };
}
