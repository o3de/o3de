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
        AuditTrailInput(AuditCategory category, ClientInputId inputId, HostFrameId hostFrameId,
            const AZStd::string name, const AZStd::vector<MultiplayerAuditingElement>&& children)
            : m_category(category)
            , m_inputId(inputId)
            , m_hostFrameId(hostFrameId)
            , m_name(name)
            , m_children(children)
        {
        }

        AuditCategory m_category;
        ClientInputId m_inputId;
        HostFrameId m_hostFrameId;
        AZStd::string m_name;
        AZStd::vector<MultiplayerAuditingElement> m_children;
    };

    //! Buffer size for ImGui Search bar in Audit Trail UI
    const int AUDIT_SEARCH_BUFFER_SIZE = 1024;

    /**
     * /brief Provides ImGui driven UX for multiplayer audit trail
     */
    class MultiplayerDebugAuditTrail
    {
    public:
        static constexpr char DESYNC_TITLE[] = "Desync on %s";
        static constexpr char INPUT_TITLE[] = "%s Inputs";
        static constexpr char EVENT_TITLE[] = "DevEvent on %s";

        MultiplayerDebugAuditTrail();
        ~MultiplayerDebugAuditTrail() = default;

        //! Main update loop.
        void OnImGuiUpdate(const AZStd::deque<AuditTrailInput>& auditTrailElems);

        //! Draws hierarchy information over hierarchy root entities.
        void UpdateDebugOverlay();

        //! Returns if the audit trail should be pumped and resets the pump flag
        bool TryPumpAuditTrail();

        //! Gets string filter for the audit trail
        AZStd::string_view GetAuditTrialFilter();

        //! Sets string filter for the audit trail
        void SetAuditTrailFilter(AZStd::string filter);
    private:
        AZ::ScheduledEvent m_updateDebugOverlay;

        AZStd::string m_filter;
        AzFramework::DebugDisplayRequests* m_debugDisplay = nullptr;
        [[maybe_unused]] char m_inputBuffer[AUDIT_SEARCH_BUFFER_SIZE] = {};
        bool m_canPumpTrail = false;
    };
}
