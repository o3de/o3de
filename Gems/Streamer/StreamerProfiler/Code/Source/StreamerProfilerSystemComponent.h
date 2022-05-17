/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/IO/IStreamerProfiler.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/limits.h>

namespace AZ::IO
{
    class IStreamer;
}

namespace Streamer
{
    class StreamerProfilerSystemComponent final
        : public AZ::Component
        , public AZ::IO::IStreamerProfiler
    {
    public:
        AZ_COMPONENT(StreamerProfilerSystemComponent, "{6b5a5e7f-81ee-4fb1-a005-107773dfc531}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        StreamerProfilerSystemComponent();
        ~StreamerProfilerSystemComponent() override;

        void DrawStatistics(bool& keepDrawing) override;

    protected:
        static constexpr size_t GraphStoreElementCount = 256; // Needs to be a power of 2.
        class GraphStore
        {
        public:
            GraphStore();
            GraphStore(float minValue, float maxValue);

            void AddValue(float value);
            float operator[](size_t index);
            float GetMin() const;
            float GetMax() const;

        private:
            AZStd::array<float, GraphStoreElementCount> m_values;
            float m_minValue{ AZStd::numeric_limits<float>::max() };
            float m_maxValue{ AZStd::numeric_limits<float>::min() };
            size_t m_front{ 0 };
        };
        using FullStatName = AZStd::fixed_string<1024>;
        using StatsContainer = AZStd::vector<AZ::IO::Statistic>;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        void DrawLiveStats(AZ::IO::IStreamer& streamer);
        void DrawHardwareInfo(AZ::IO::IStreamer& streamer);
        void DrawStackConfiguration(AZ::IO::IStreamer& streamer);
        void DrawFileLocks(AZ::IO::IStreamer& streamer);
        void DrawGraph(const AZ::IO::Statistic::Value& value, GraphStore& values, bool useHistogram);
        void DrawStatisticValue(
            const AZ::IO::Statistic::Value& value,
            float capturedMin = AZStd::numeric_limits<float>::max(),
            float capturedMax = AZStd::numeric_limits<float>::min());
        static void AppendByteSize(AZStd::fixed_string<256>& text, AZ::u64 value);
        static void AppendTime(AZStd::fixed_string<256>& text, AZ::IO::Statistic::TimeValue value);
        static void AppendBytesPerSecond(AZStd::fixed_string<256>& text, double value);
        static GraphStore CreateGraph(const AZ::IO::Statistic::Value& value);
        static void DrawToolTip(AZStd::string_view text);
        static void DrawToolTipV(const char* text, ...);

        AZStd::unordered_map<FullStatName, GraphStore> m_graphInfo;
        StatsContainer m_stats;
        StatsContainer m_stackConfiguration;
        StatsContainer m_fileLocks[2];
        StatsContainer* m_readingFileLocks{ m_fileLocks };
        AZStd::atomic<StatsContainer*> m_transferFileLocks{  };
        StatsContainer* m_displayingFileLocks{ m_fileLocks + 1};
        AZStd::atomic_bool m_stackConfigurationAvailable{ false };
    };

} // namespace Streamer
