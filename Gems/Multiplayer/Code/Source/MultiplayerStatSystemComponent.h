/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <Multiplayer/MultiplayerStatSystemInterface.h>

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
        void DeclareStat(int uniqueGroupId, int uniqueStatId, const char* statName) override;
        void SetStat(int uniqueStatId, double value) override;
        void IncrementStat(int uniqueStatId) override;
        //! @}

    private:
        void RecordMetrics();
        AZ::ScheduledEvent m_metricsEvent{ [this]()
                                           {
                                               RecordMetrics();
                                           },
                                           AZ::Name("MultiplayerStats") };

        struct CumulativeAverage
        {
            using AverageWindowType = AZ::IO::AverageWindow<double, double, AZ::IO::s_statisticsWindowSize>;

            AZStd::string m_name;
            AverageWindowType m_average;
            double m_lastValue = 0;
            AZ::u64 m_counterValue = 0; // Used by counters.
        };

        //! A custom combined data structure for fast iteration and fast insertion.
        //! Items can only be added, never removed.
        template<typename ID, typename Value>
        class MappedArrayWithNonRemovableItems
        {
        public:
            Value* AddNew(ID newId)
            {
                auto newItem = &m_items.emplace_back();
                m_idToItems[newId] = m_items.size() - 1;
                return newItem;
            }

            Value* Find(ID byId)
            {
                auto valueIterator = m_idToItems.find(byId);
                if (valueIterator != m_idToItems.end())
                {
                    return &m_items[valueIterator->second];
                }

                return nullptr;
            }

            AZStd::vector<Value> m_items;
            AZStd::unordered_map<ID, AZStd::size_t> m_idToItems;
        };

        struct StatGroup
        {
            AZStd::string m_name;

            // For fast iteration and fast insertion
            MappedArrayWithNonRemovableItems<int, CumulativeAverage> m_stats;
        };

        MappedArrayWithNonRemovableItems<int, StatGroup> m_statGroups;

        AZStd::unordered_map<int, int> m_statIdToGroupId;

        AZStd::mutex m_access;
    };
} // namespace Multiplayer
