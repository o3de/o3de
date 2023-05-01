/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <StreamerProfilerSystemComponent.h>

#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/Streamer/FileRequest.h>

#if defined(IMGUI_ENABLED)
#include <imgui/imgui.h>
#endif

namespace Streamer
{
    StreamerProfilerSystemComponent::GraphStore::GraphStore()
    {
        memset(m_values.data(), 0, sizeof(float) * GraphStoreElementCount);
    }

    StreamerProfilerSystemComponent::GraphStore::GraphStore(float minValue, float maxValue)
        : m_minValue(minValue)
        , m_maxValue(maxValue)
    {
        AZ_Assert(
            minValue <= maxValue,
            "A GraphStore object in the Streamer Profiler received a min value (%f) that's not smaller or equal to the max value (%f).",
            minValue, maxValue);
        memset(m_values.data(), 0, sizeof(float) * GraphStoreElementCount);
    }

    void StreamerProfilerSystemComponent::GraphStore::AddValue(float value)
    {
        m_minValue = AZStd::min(value, m_minValue);
        m_maxValue = AZStd::max(value, m_maxValue);
        m_values[m_front] = value;
        m_front = (m_front + 1) & (GraphStoreElementCount - 1);
    }

    float StreamerProfilerSystemComponent::GraphStore::operator[](size_t index)
    {
        return m_values[(index + m_front) & (GraphStoreElementCount - 1)];
    }

    float StreamerProfilerSystemComponent::GraphStore::GetMin() const
    {
        return m_minValue;
    }

    float StreamerProfilerSystemComponent::GraphStore::GetMax() const
    {
        return m_maxValue;
    }

    void StreamerProfilerSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<StreamerProfilerSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<StreamerProfilerSystemComponent>("Streamer Profiler", "Provides profiling visualization for AZ::IO::Streamer.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void StreamerProfilerSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("StreamerProfilerService"));
    }

    void StreamerProfilerSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("StreamerProfilerService"));
    }

    void StreamerProfilerSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void StreamerProfilerSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    StreamerProfilerSystemComponent::StreamerProfilerSystemComponent()
    {
        if (AZ::IO::StreamerProfiler::Get() == nullptr)
        {
            AZ::IO::StreamerProfiler::Register(this);
        }
    }

    StreamerProfilerSystemComponent::~StreamerProfilerSystemComponent()
    {
        if (AZ::IO::StreamerProfiler::Get() == this)
        {
            AZ::IO::StreamerProfiler::Unregister(this);
        }
    }

    void StreamerProfilerSystemComponent::Init()
    {
    }

    void StreamerProfilerSystemComponent::Activate()
    {
    }

    void StreamerProfilerSystemComponent::Deactivate()
    {
    }

    void StreamerProfilerSystemComponent::DrawStatistics([[maybe_unused]] bool& keepDrawing)
    {
#if defined(IMGUI_ENABLED)
        ImGui::SetNextWindowSize({1024.0f, 800.0f}, ImGuiCond_Once);
        if (ImGui::Begin("File IO Profiler", &keepDrawing, ImGuiWindowFlags_None))
        {
            if (auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get(); streamer)
            {
                if (ImGui::CollapsingHeader("Hardware Info"))
                {
                    DrawHardwareInfo(*streamer);
                }
                else
                {
                    DrawToolTip("Lists the lowest hardware specs across all used hardware. The presented information are recommendations "
                                "to consider for various use cases. The information in the live stats will refer to these values.");
                }

                if (ImGui::CollapsingHeader("Stack configuration"))
                {
                    DrawStackConfiguration(*streamer);
                }
                else
                {
                    DrawToolTip("The configuration of the Streamer stack. These are the nodes that process requests or provide information "
                                "to the scheduler to schedule requests. Requests are added to the top of the stack and the move down the "
                                "stack or are completed early if possible.");
                }

                if (ImGui::CollapsingHeader("Live stats", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    DrawLiveStats(*streamer);
                }
                else
                {
                    DrawToolTip("The live metrics retrieved from all parts of Streamer. These contain all information that can be "
                                "retrieved without issuing requests to Streamer. As such these values should be viewed as coming from a "
                                "sampling profiler, which means not all changes in the values are captured. Note though that a large "
                                "number of values are recorded inside Streamer using a sliding window so this limitation typically doesn't"
                                "impact the ability to retrieve meaningful information.");
                }

                if (ImGui::CollapsingHeader("File locks"))
                {
                    DrawFileLocks(*streamer);
                }
                else
                {
                    DrawToolTip("A list of all the files that are locked by Streamer and by what node. Retrieving this information "
                                "requires repeatedly issuing requests with Streamer, which will show up in the live stats.");
                }
            }
        }
#endif // #if defined(IMGUI_ENABLED)
    }

    void StreamerProfilerSystemComponent::DrawLiveStats([[maybe_unused]] AZ::IO::IStreamer& streamer)
    {
#if defined(IMGUI_ENABLED)
        if (ImGui::Button("Reset graphs"))
        {
            m_graphInfo.clear();
        }

        ImGui::SameLine();
        bool suspend = streamer.IsSuspended();
        if (ImGui::Checkbox("Suspend", &suspend))
        {
            if (suspend)
            {
                streamer.SuspendProcessing();
            }
            else
            {
                streamer.ResumeProcessing();
            }
        }

        if (ImGui::BeginTable("Stats", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
        {
            streamer.CollectStatistics(m_stats);

            ImGui::TableSetupColumn("Owner", ImGuiTableColumnFlags_WidthStretch, 0.15f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.15f);
            ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthFixed, 0.50f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.20f);
            ImGui::TableHeadersRow();

            for (AZ::IO::Statistic& stat : m_stats)
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%.*s", AZ_STRING_ARG(stat.GetOwner()));

                ImGui::TableNextColumn();
                ImGui::Text("%.*s", AZ_STRING_ARG(stat.GetName()));
                if (!stat.GetDescription().empty())
                {
                    DrawToolTip(stat.GetDescription());
                }

                float minValue = AZStd::numeric_limits<float>::max();
                float maxValue = AZStd::numeric_limits<float>::min();
                ImGui::TableNextColumn();
                if (stat.GetGraphType() != AZ::IO::Statistic::GraphType::None)
                {
                    FullStatName fullStatName;
                    fullStatName += stat.GetOwner();
                    fullStatName += '.';
                    fullStatName += stat.GetName();
                    auto graphIt = m_graphInfo.find(fullStatName);
                    if (graphIt == m_graphInfo.end())
                    {
                        graphIt = m_graphInfo.emplace(AZStd::move(fullStatName), CreateGraph(stat.GetValue())).first;
                    }
                    else
                    {
                        minValue = graphIt->second.GetMin();
                        maxValue = graphIt->second.GetMax();
                    }
                    DrawGraph(stat.GetValue(), graphIt->second, stat.GetGraphType() == AZ::IO::Statistic::GraphType::Histogram);
                }

                ImGui::TableNextColumn();
                DrawStatisticValue(stat.GetValue(), minValue, maxValue);
            }

            ImGui::EndTable();

            m_stats.clear();
        }
#endif
    }

    void StreamerProfilerSystemComponent::DrawHardwareInfo([[maybe_unused]] AZ::IO::IStreamer& streamer)
    {
#if defined(IMGUI_ENABLED)
        if (ImGui::BeginTable("Hardware", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
        {
            const AZ::IO::IStreamerTypes::Recommendations& recommendations = streamer.GetRecommendations();

            ImGui::TableNextColumn();
            ImGui::Text("Memory alignment");
            DrawToolTip("The minimal memory alignment that's required to avoid intermediate buffers. If the memory provided "
                        "to the read request isn't aligned to this size it may require a temporary or cached buffer to "
                        "first read to and copy the result from to the provided memory.");
            ImGui::TableNextColumn();
            ImGui::Text("%llu bytes", recommendations.m_memoryAlignment);
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("Size alignment");
            DrawToolTip("The minimal size alignment that's required to avoid intermediate buffers. If the size and/or "
                        "offset provided to the read request isn't aligned to this size it may require a temporary or "
                        "cached buffer to first read to and copy the result from to the provided memory.");
            ImGui::TableNextColumn();
            ImGui::Text("%llu bytes", recommendations.m_sizeAlignment);
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("Granularity");
            DrawToolTip("The recommended size for partial reads. It's recommended to read entire files at once, but for "
                        "streaming systems such as video and audio this is not always practical. The granularity will give "
                        "the most optimal size for partial file reads. Note for partial reads it's also recommended to "
                        "store the data uncompressed and to align the offset of the rest to the granularity.");
            ImGui::TableNextColumn();
            ImGui::Text("%.2f kilobytes (%llu bytes)", (recommendations.m_granularity / 1024.0f), recommendations.m_granularity);
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text("Max concurrent reads");
            DrawToolTip(
                    "The number of requests that the scheduler will try to keep active in the stack. Additional requests are considered "
                    "pending and are subject to scheduling. There are no restrictions on the number of requests that can be send and "
                    "generally there is no need to throttle the number of requests. The exception is for streaming systems such as video "
                    "and audio that could flood the scheduler with requests in a short amount of time if not capped. For those systems "
                    "it's recommended that no more than the provided number of requests are issued.");
            ImGui::TableNextColumn();
            ImGui::Text("%d", recommendations.m_maxConcurrentRequests);
            ImGui::TableNextRow();

            ImGui::EndTable();
        }
#endif //#if defined(IMGUI_ENABLED)
    }

    void StreamerProfilerSystemComponent::DrawStackConfiguration([[maybe_unused]] AZ::IO::IStreamer& streamer)
    {
#if defined(IMGUI_ENABLED)
        if (!m_stackConfigurationAvailable)
        {
            AZ::IO::FileRequestPtr request = streamer.Report(m_stackConfiguration, AZ::IO::IStreamerTypes::ReportType::Config);
            auto callback = [this](AZ::IO::FileRequestHandle)
            {
                m_stackConfigurationAvailable = true;
            };
            streamer.SetRequestCompleteCallback(request, AZStd::move(callback));
            streamer.QueueRequest(request);
        }
        else
        {
            if (ImGui::BeginTable("Stack configuration", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
            {
                ImGui::TableSetupColumn("Node", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                AZStd::string_view currentNode = "";

                for (AZ::IO::Statistic& stat : m_stackConfiguration)
                {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    if (currentNode != stat.GetOwner())
                    {
                        ImGui::Text("%.*s", AZ_STRING_ARG(stat.GetOwner()));
                        currentNode = stat.GetOwner();

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                    }

                    ImGui::TableNextColumn();
                    ImGui::Text("%.*s", AZ_STRING_ARG(stat.GetName()));
                    if (!stat.GetDescription().empty())
                    {
                        DrawToolTip(stat.GetDescription());
                    }

                    ImGui::TableNextColumn();
                    DrawStatisticValue(stat.GetValue());
                }

                ImGui::EndTable();
            }
        }
#endif // #if defined(IMGUI_ENABLED)
    }

    void StreamerProfilerSystemComponent::DrawFileLocks([[maybe_unused]] AZ::IO::IStreamer& streamer)
    {
#if defined(IMGUI_ENABLED)
        // Queue next request for update if needed.
        if (m_readingFileLocks)
        {
            AZ::IO::FileRequestPtr request = streamer.Report(*m_readingFileLocks, AZ::IO::IStreamerTypes::ReportType::FileLocks);
            auto callback = [this, readingFileLocks = m_readingFileLocks](AZ::IO::FileRequestHandle)
            {
                // Once the request has been completed, enable the stats to draw.
                StatsContainer* displayingLocks = nullptr;
                while (!m_transferFileLocks.compare_exchange_strong(displayingLocks, readingFileLocks))
                {
                    displayingLocks = nullptr;
                }
            };
            streamer.SetRequestCompleteCallback(request, AZStd::move(callback));
            streamer.QueueRequest(request);

            m_readingFileLocks = nullptr;
        }

        // Try to get a handle to the fresh stats.
        StatsContainer* transferFileLocks = m_transferFileLocks;
        while (!m_transferFileLocks.compare_exchange_strong(transferFileLocks, nullptr))
        {
            transferFileLocks = m_transferFileLocks;
        }
        if (transferFileLocks)
        {
            // Rotate the buffers
            m_displayingFileLocks->clear();
            m_readingFileLocks = m_displayingFileLocks;
            m_displayingFileLocks = transferFileLocks;
        }

        // Draw the display list. One should always be assigned.
        AZ_Assert(m_displayingFileLocks, "Expected to always have a valid list of file locks, even if it's empty.");
        ImGui::Text("Total file lock count: %zu", m_displayingFileLocks->size());
        if (ImGui::Button("Flush all"))
        {
            streamer.QueueRequest(streamer.FlushCaches());
        }
        if (ImGui::BeginTable("File Locks", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable))
        {
            ImGui::TableSetupColumn("Node", ImGuiTableColumnFlags_WidthStretch, 0.20f);
            ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch, 0.70f);
            ImGui::TableSetupColumn("Flush", ImGuiTableColumnFlags_WidthStretch, 0.10f);
            ImGui::TableHeadersRow();

            for (AZ::IO::Statistic& stat : *m_displayingFileLocks)
            {
                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text("%.*s", AZ_STRING_ARG(stat.GetOwner()));

                ImGui::TableNextColumn();
                DrawStatisticValue(stat.GetValue());

                const AZStd::string* path = AZStd::get_if<AZStd::string>(&stat.GetValue());
                if (path)
                {
                    constexpr static size_t StringCacheSize = AZ::IO::MaxPathLength + 7; // +7 for the characters in "Flush##".
                    ImGui::TableNextColumn();
                    AZStd::fixed_string<StringCacheSize> name = "Flush##";
                    size_t remainingStringSpace = StringCacheSize - name.length() - 1;
                    if (path->length() < remainingStringSpace)
                    {
                        name += *path;
                    }
                    else
                    {
                        name += AZStd::string_view(*path).substr(path->length() - remainingStringSpace);
                    }

                    if (ImGui::Button(name.c_str()))
                    {
                        streamer.QueueRequest(streamer.FlushCache(*path));
                    }
                }
            }

            ImGui::EndTable();
        }
#endif // #if defined(IMGUI_ENABLED)
    }

    void StreamerProfilerSystemComponent::DrawGraph(
        [[maybe_unused]] const AZ::IO::Statistic::Value& value, [[maybe_unused]] GraphStore& values, [[maybe_unused]] bool useHistogram)
    {
#if defined(IMGUI_ENABLED)
        auto visitor = [&values](auto&& value)
        {
            using Type = AZStd::decay_t<decltype(value)>;
            if constexpr (AZStd::is_same_v<Type, bool>)
            {
                values.AddValue(value ? 1.0f : 0.0f);
            }
            else if constexpr (AZStd::is_same_v<Type, double> || AZStd::is_same_v<Type, AZ::s64>)
            {
                values.AddValue(azlossy_cast<float>(value));
            }
            else if constexpr (
                AZStd::is_same_v<Type, AZ::IO::Statistic::FloatRange> || AZStd::is_same_v<Type, AZ::IO::Statistic::IntegerRange> ||
                AZStd::is_same_v<Type, AZ::IO::Statistic::Percentage> || AZStd::is_same_v<Type, AZ::IO::Statistic::PercentageRange> ||
                AZStd::is_same_v<Type, AZ::IO::Statistic::ByteSize> || AZStd::is_same_v<Type, AZ::IO::Statistic::ByteSizeRange> ||
                AZStd::is_same_v<Type, AZ::IO::Statistic::BytesPerSecond>)
            {
                values.AddValue(azlossy_cast<float>(value.m_value));
            }
            else if constexpr (AZStd::is_same_v<Type, AZ::IO::Statistic::Time> || AZStd::is_same_v<Type, AZ::IO::Statistic::TimeRange>)
            {
                values.AddValue(azlossy_cast<float>(value.m_value.count()));
            }
        };
        AZStd::visit(visitor, value);

        auto valueRetriever = [](void* data, int idx) -> float
        {
            return (*reinterpret_cast<GraphStore*>(data))[idx];
        };

        if (useHistogram)
        {
            ImGui::PlotHistogram("", valueRetriever, &values, GraphStoreElementCount, 0, nullptr, values.GetMin(), values.GetMax());
        }
        else
        {
            ImGui::PlotLines("", valueRetriever, &values, GraphStoreElementCount, 0, nullptr, values.GetMin(), values.GetMax());
        }
#endif // #if defined(IMGUI_ENABLED)
    }

    void StreamerProfilerSystemComponent::DrawStatisticValue(
        [[maybe_unused]] const AZ::IO::Statistic::Value& value, [[maybe_unused]] float capturedMin, [[maybe_unused]] float capturedMax)
    {
#if defined(IMGUI_ENABLED)
        auto visitor = [capturedMin, capturedMax](auto&& value)
        {
            using Type = AZStd::decay_t<decltype(value)>;
            if constexpr (AZStd::is_same_v<Type, bool>)
            {
                static ImColor colorEnabled = ImColor(0.0f, 1.0f, 0.0f, 0.75f);
                static ImColor colorDisabled = ImColor(1.0f, 0.0f, 0.0f, 0.75f);
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, value ? colorEnabled : colorDisabled);
                ImGui::Text(value ? "True" : "False");
            }
            else if constexpr (AZStd::is_same_v<Type, double>)
            {
                ImGui::Text("%.2f", value);
                DrawToolTipV("Min: %.2f\nMax: %.2f", capturedMin, capturedMax);
            }
            else if constexpr (AZStd::is_same_v<Type, AZ::IO::Statistic::FloatRange>)
            {
                if (value.m_min != AZStd::numeric_limits<decltype(value.m_min)>::max() &&
                    value.m_max != AZStd::numeric_limits<decltype(value.m_max)>::min())
                {
                    ImGui::Text("%.2f", value.m_value);
                    DrawToolTipV("Min: %.2f\nMax: %.2f", value.m_min, value.m_max);
                }
                else
                {
                    ImGui::Text("Unused");
                }
            }
            else if constexpr (AZStd::is_same_v<Type, AZ::s64>)
            {
                ImGui::Text("%lld", value);
                DrawToolTipV("Min: %i\nMax: %i", azlossy_cast<int>(capturedMin), azlossy_cast<int>(capturedMax));
            }
            else if constexpr (AZStd::is_same_v<Type, AZ::IO::Statistic::IntegerRange>)
            {
                if (value.m_min != AZStd::numeric_limits<decltype(value.m_min)>::max() &&
                    value.m_max != AZStd::numeric_limits<decltype(value.m_max)>::min())
                {
                    ImGui::Text("%lld", value.m_value);
                    DrawToolTipV("Min: %lld\nMax: %lld", value.m_min, value.m_max);
                }
                else
                {
                    ImGui::Text("Unused");
                }
            }
            else if constexpr (AZStd::is_same_v<Type, AZ::IO::Statistic::Percentage>)
            {
                ImGui::ProgressBar(aznumeric_cast<float>(value.m_value));
            }
            else if constexpr (AZStd::is_same_v<Type, AZ::IO::Statistic::PercentageRange>)
            {
                ImGui::ProgressBar(aznumeric_cast<float>(value.m_value));
                DrawToolTipV("Min: %.2f%%\nMax: %.2f%%", value.m_min * 100.0, value.m_max * 100.0);
            }
            else if constexpr (AZStd::is_same_v<Type, AZ::IO::Statistic::ByteSize>)
            {
                AZStd::fixed_string<256> text;
                AppendByteSize(text, value.m_value);
                ImGui::TextUnformatted(text.data(), text.data() + text.size());

                text.clear();
                text += "Min: ";
                AppendByteSize(text, azlossy_cast<AZ::u64>(capturedMin));
                text += "\nMax: ";
                AppendByteSize(text, azlossy_cast<AZ::u64>(capturedMax));
                DrawToolTip(text);
            }
            else if constexpr (AZStd::is_same_v<Type, AZ::IO::Statistic::ByteSizeRange>)
            {
                if (value.m_min != AZStd::numeric_limits<decltype(value.m_min)>::max() &&
                    value.m_max != AZStd::numeric_limits<decltype(value.m_max)>::min())
                {
                    AZStd::fixed_string<256> text;
                    AppendByteSize(text, value.m_value);
                    ImGui::TextUnformatted(text.data(), text.data() + text.size());

                    text.clear();
                    text += "Min: ";
                    AppendByteSize(text, value.m_min);
                    text += "\nMax: ";
                    AppendByteSize(text, value.m_max);
                    DrawToolTip(text);
                }
                else
                {
                    ImGui::Text("Unused");
                }
            }
            else if constexpr (AZStd::is_same_v<Type, AZ::IO::Statistic::Time>)
            {
                AZStd::fixed_string<256> text;
                AppendTime(text, value.m_value);
                ImGui::TextUnformatted(text.data(), text.data() + text.size());

                text.clear();
                text += "Min: ";
                AppendTime(text, AZ::IO::Statistic::TimeValue{ azlossy_cast<AZ::s64>(capturedMin) });
                text += "\nMax: ";
                AppendTime(text, AZ::IO::Statistic::TimeValue{ azlossy_cast<AZ::s64>(capturedMax) });
                DrawToolTip(text);
            }
            else if constexpr (AZStd::is_same_v<Type, AZ::IO::Statistic::TimeRange>)
            {
                if (value.m_min != AZ::IO::Statistic::TimeValue::max() && value.m_max != AZ::IO::Statistic::TimeValue::min())
                {
                    AZStd::fixed_string<256> text;
                    AppendTime(text, value.m_value);
                    ImGui::TextUnformatted(text.data(), text.data() + text.size());

                    text.clear();
                    text += "Min: ";
                    AppendTime(text, value.m_min);
                    text += "\nMax: ";
                    AppendTime(text, value.m_max);
                    DrawToolTip(text);
                }
                else
                {
                    ImGui::Text("Unused");
                }
            }
            else if constexpr (AZStd::is_same_v<Type, AZ::IO::Statistic::BytesPerSecond>)
            {
                AZStd::fixed_string<256> text;
                AppendBytesPerSecond(text, value.m_value);
                ImGui::TextUnformatted(text.data(), text.data() + text.size());

                text.clear();
                text += "Min: ";
                AppendBytesPerSecond(text, capturedMin);
                text += "\nMax: ";
                AppendBytesPerSecond(text, capturedMax);
                DrawToolTip(text);
            }
            else if constexpr (AZStd::is_same_v<Type, AZStd::string> || AZStd::is_same_v<Type, AZStd::string_view>)
            {
                ImGui::TextUnformatted(value.begin(), value.end());
            }
        };
        AZStd::visit(visitor, value);
#endif // #if defined(IMGUI_ENABLED)
    }

    void StreamerProfilerSystemComponent::AppendByteSize(AZStd::fixed_string<256>& text, AZ::u64 value)
    {
        static constexpr AZ::u64 kilobyte = 1024;
        static constexpr AZ::u64 megabyte = 1024 * kilobyte;
        static constexpr AZ::u64 gigabyte = 1024 * megabyte;
        static constexpr AZ::u64 terabyte = 1024 * gigabyte;

        if (value > terabyte)
        {
            text += AZStd::fixed_string<64>::format("%.2f terabytes (%llu bytes)", value * (1.0f / terabyte), value);
        }
        else if (value > gigabyte)
        {
            text += AZStd::fixed_string<64>::format("%.2f gigabytes (%llu bytes)", value * (1.0f / gigabyte), value);
        }
        else if (value > megabyte)
        {
            text += AZStd::fixed_string<64>::format("%.2f megabytes (%llu bytes)", value * (1.0f / megabyte), value);
        }
        else if (value > kilobyte)
        {
            text += AZStd::fixed_string<32>::format("%.2f kilobytes (%llu bytes)", value * (1.0f / kilobyte), value);
        }
        else
        {
            text += AZStd::fixed_string<16>::format("%llu bytes", value);
        }
    }

    void StreamerProfilerSystemComponent::AppendTime(AZStd::fixed_string<256>& text, AZ::IO::Statistic::TimeValue value)
    {
        static constexpr AZ::s64 microseconds = 1000;
        static constexpr AZ::s64 milliseconds = microseconds * 1000;
        static constexpr AZ::s64 seconds = milliseconds * 1000;
        static constexpr AZ::s64 minutes = 60 * seconds;
        static constexpr AZ::s64 hours = 60 * minutes;

        auto count = value.count();
        if (count > hours)
        {
            text += AZStd::fixed_string<32>::format("%.2f hours", count * (1.0 / hours));
        }
        else if (count > minutes)
        {
            text += AZStd::fixed_string<32>::format("%.2f minutes", count * (1.0 / minutes));
        }
        else if (count > seconds)
        {
            text += AZStd::fixed_string<32>::format("%.2f seconds", count * (1.0 / seconds));
        }
        else if (count > milliseconds)
        {
            text += AZStd::fixed_string<32>::format("%.2f milliseconds", count * (1.0 / milliseconds));
        }
        else if (count > microseconds)
        {
            text += AZStd::fixed_string<32>::format("%.2f microseconds", count * (1.0 / microseconds));
        }
        else
        {
            text += AZStd::fixed_string<32>::format("%lld nanoseconds", static_cast<long long>(count));
        }
    }

    void StreamerProfilerSystemComponent::AppendBytesPerSecond(AZStd::fixed_string<256>& text, double value)
    {
        static constexpr AZ::u64 kilobyte = 1024;
        static constexpr AZ::u64 megabyte = 1024 * kilobyte;
        static constexpr AZ::u64 gigabyte = 1024 * megabyte;
        static constexpr AZ::u64 terabyte = 1024 * gigabyte;

        if (value > terabyte)
        {
            text += AZStd::fixed_string<64>::format("%.2f terabytes per second", value * (1.0 / terabyte));
        }
        else if (value > gigabyte)
        {
            text += AZStd::fixed_string<64>::format("%.2f gigabytes per second", value * (1.0 / gigabyte));
        }
        else if (value > megabyte)
        {
            text += AZStd::fixed_string<32>::format("%.2f megabytes per second", value * (1.0 / megabyte));
        }
        else if (value > kilobyte)
        {
            text += AZStd::fixed_string<32>::format("%.2f kilobytes per second", value * (1.0 / kilobyte));
        }
        else
        {
            text += AZStd::fixed_string<32>::format("%.2f bytes per second", value);
        }
    }

    auto StreamerProfilerSystemComponent::CreateGraph(const AZ::IO::Statistic::Value& value) -> GraphStore
    {
        auto visitor = [](auto&& value) -> GraphStore
        {
            using Type = AZStd::decay_t<decltype(value)>;
            if constexpr (AZStd::is_same_v<Type, bool> || AZStd::is_same_v<Type, AZ::IO::Statistic::Percentage>)
            {
                return GraphStore(0.0f, 1.0f);
            }
            else if constexpr (
                AZStd::is_same_v<Type, AZ::IO::Statistic::FloatRange> ||
                AZStd::is_same_v<Type, AZ::IO::Statistic::IntegerRange> ||
                AZStd::is_same_v<Type, AZ::IO::Statistic::PercentageRange>)
            {
                return GraphStore(azlossy_cast<float>(value.m_min), azlossy_cast<float>(value.m_max));
            }
            else
            {
                return GraphStore{};
            }
        };
        return AZStd::visit(visitor, value);
    }

    void StreamerProfilerSystemComponent::DrawToolTip([[maybe_unused]] AZStd::string_view text)
    {
#if defined(IMGUI_ENABLED)
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(text.data(), text.data() + text.size());
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
#endif // #if defined(IMGUI_ENABLED)
    }

    void StreamerProfilerSystemComponent::DrawToolTipV([[maybe_unused]] const char* text, ...)
    {
#if defined(IMGUI_ENABLED)
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            va_list args;
            va_start(args, text);
            ImGui::TextV(text, args);
            va_end(args);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
#endif // #if defined(IMGUI_ENABLED)
    }
} // namespace Streamer
