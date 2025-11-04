/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "MultiplayerDebugAuditTrail.h"
#include "MultiplayerDebugHierarchyReporter.h"
#include "MultiplayerDebugPerEntityReporter.h"

#include <AzCore/Component/Component.h>
#include <AzCore/Interface/Interface.h>
#include <Debug/MultiplayerDebugNetworkMetrics.h>
#include <Debug/MultiplayerDebugMultiplayerMetrics.h>
#include <Multiplayer/IMultiplayerDebug.h>

#ifdef IMGUI_ENABLED
#   include <imgui/imgui.h>
#   include <ImGuiBus.h>
#   include <LYImGuiUtils/HistogramContainer.h>
#endif

namespace Multiplayer
{
    class MultiplayerDebugSystemComponent final
        : public AZ::Component
        , public AZ::Interface<IMultiplayerDebug>::Registrar
#ifdef IMGUI_ENABLED
        , public ImGui::ImGuiUpdateListenerBus::Handler
#endif
    {
    public:
        static constexpr char HostLevelMenuTitle[] = "Host Level";
        static constexpr char LaunchLocalClientButtonTitle[] = "Launch Local Client";
        static constexpr char NoMultiplayerLevelsFound[] = "No multiplayer levels found";

        AZ_COMPONENT(MultiplayerDebugSystemComponent, "{060BF3F1-0BFE-4FCE-9C3C-EE991F0DA581}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ~MultiplayerDebugSystemComponent() override = default;

        //! AZ::Component overrides
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! IMultiplayerDebug overrides
        //! @{
        void ShowEntityBandwidthDebugOverlay() override;
        void HideEntityBandwidthDebugOverlay() override;
        void AddAuditEntry(
            const AuditCategory category,
            const ClientInputId inputId,
            const HostFrameId frameId,
            const AZStd::string& name,
            AZStd::vector<MultiplayerAuditingElement>&& entryDetails) override;
        //! @}

#ifdef IMGUI_ENABLED
        //! ImGui::ImGuiUpdateListenerBus overrides
        //! @{
        void OnImGuiMainMenuUpdate() override;
        void OnImGuiUpdate() override;
        //! @}
    private:

        //! Constructs a filtered version of the audit trail based on a search string
        void FilterAuditTrail();

        bool m_displayNetworkingStats = false;
        AZStd::unique_ptr<MultiplayerDebugNetworkMetrics> m_networkMetrics;

        bool m_displayMultiplayerStats = false;
        AZStd::unique_ptr<MultiplayerDebugMultiplayerMetrics> m_multiplayerMetrics;

        bool m_displayPerEntityStats = false;
        AZStd::unique_ptr<MultiplayerDebugPerEntityReporter> m_reporter;

        bool m_displayHierarchyDebugger = false;
        AZStd::unique_ptr<MultiplayerDebugHierarchyReporter> m_hierarchyDebugger;

        bool m_displayNetAuditTrail = false;
        AZStd::unique_ptr<MultiplayerDebugAuditTrail> m_auditTrail;

        AZStd::string m_lastFilter;
        AZStd::deque<AuditTrailInput> m_auditTrailElems;
        AZStd::deque<AuditTrailInput> m_committedAuditTrail;
        AZStd::deque<AuditTrailInput> m_pendingAuditTrail;
        AZStd::deque<AuditTrailInput> m_filteredAuditTrail;

        AZ::ApplicationTypeQuery m_applicationType;
#endif
    };
}
