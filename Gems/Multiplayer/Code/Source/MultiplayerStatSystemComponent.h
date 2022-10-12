/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Metrics/IEventLogger.h>
#include <Multiplayer/MultiplayerStatSystem.h>

namespace Multiplayer
{
    //! @class MultiplayerStatSystemComponent
    //! Periodically writes the metrics to AZ::EventLogger. See MultiplayerStatSystem.h for documentation.
    class MultiplayerStatSystemComponent final
        : public AZ::Component
        , public IMultiplayerStatSystem
    {
    public:
        AZ_COMPONENT(MultiplayerStatSystemComponent, "{890831db-3ca4-4d8c-a43e-d53d1197044d}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        MultiplayerStatSystemComponent();
        ~MultiplayerStatSystemComponent() override;

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! IMultiplayerStatSystem overrides.
        //! @{
        void Register() override;
        void Unregister() override;
        void SetReportPeriod(AZ::TimeMs period) override;
        void DeclareStatGroup(int uniqueGroupId, const char* groupName) override;
        void DeclareStatTypeIntU64(int uniqueGroupId, int uniqueStatId, const char* statName) override;
        void SetStatTypeIntU64(int uniqueStatId, AZ::u64 value) override;
        //! @}

    private:
        void RecordMetrics();
        AZ::ScheduledEvent m_metricsEvent{ [this]()
                                           {
                                               RecordMetrics();
                                           },
                                           AZ::Name("MultiplayerStats") };
        
        struct Group
        {
            const char* m_name = nullptr;
            AZStd::unordered_map<int, AZ::Metrics::EventField> m_stats;
        };

        AZStd::unordered_map<int, Group> m_groups;
        AZStd::unordered_map<int, Group*> m_statToGroupId;

        AZStd::mutex m_access;
    };
} // namespace Multiplayer
