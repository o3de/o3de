/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifdef IMGUI_ENABLED

#include <AzCore/std/sort.h>
#include <AzFramework/Input/Buses/Requests/InputDeviceRequestBus.h>
#include <AzFramework/Input/Devices/InputDevice.h>

#include "ImGuiInputMonitor.h"

namespace ImGui
{
    ImGuiInputMonitor::ImGuiInputMonitor()
        : m_enabled(false)
    {
    }

    void ImGuiInputMonitor::Initialize()
    {
        // Connect to EBUSes
        AZ::TickBus::Handler::BusConnect();
    }

    void ImGuiInputMonitor::Shutdown()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    void ImGuiInputMonitor::ImGuiUpdate()
    {
        // Manage main window visibility
        if (m_enabled)
        {
            if (ImGui::Begin("Input Monitor", &m_enabled, ImGuiWindowFlags_MenuBar|ImGuiWindowFlags_HorizontalScrollbar|ImGuiWindowFlags_NoSavedSettings))
            {
                // Draw the Entire Main Menu Window Area
                ImGuiUpdate_DrawMenu();
            }
            ImGui::End();
        }
    }

    void ImGuiInputMonitor::ImGuiUpdate_DrawMenu()
    {
        if (ImGui::CollapsingHeader("Input Monitor", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            using deviceBus = AzFramework::InputDeviceRequestBus;
            using deviceBusEvents = AzFramework::InputDeviceRequestBus::Events;
            AzFramework::InputDeviceRequests::InputDeviceIdSet deviceIds;
            deviceBus::Broadcast(&deviceBusEvents::GetInputDeviceIds, deviceIds);
            for (auto deviceId : deviceIds)
            {
                const AzFramework::InputDevice* device = nullptr;
                deviceBus::EventResult(device, deviceId, &deviceBusEvents::GetInputDevice);
                if (device)
                {
                    const auto& inputChannelsById = device->GetInputChannelsById();

                    AZStd::string childText = AZStd::string::format("Index: %d - Name '%s' - %s - player: %d - %d input channels", 
                        device->GetInputDeviceId().GetIndex(), 
                        device->GetInputDeviceId().GetName(),
                        device->IsConnected() ? "CONNECTED" : "NOT CONNECTED",
                        device->GetAssignedLocalUserId(),
                        static_cast<int>(inputChannelsById.size()));

                    if (inputChannelsById.size() == 0)
                    {
                        ImGui::Text("%s", childText.c_str());
                    }
                    else if (ImGui::TreeNodeEx(childText.c_str(), 0))
                    {
                        AZStd::vector<AzFramework::InputChannelId> channelsSortedByName;
                        for (const auto& inputChannelById : inputChannelsById)
                        {
                            channelsSortedByName.push_back(inputChannelById.first);
                        }
                        auto sortFn = [](const AzFramework::InputChannelId& lhs, const AzFramework::InputChannelId& rhs)
                        {
                            return azstricmp(lhs.GetName(), rhs.GetName()) < 0;
                        };
                        
                        AZStd::sort(channelsSortedByName.begin(), channelsSortedByName.end(), sortFn);

                        for (const auto& inputChannelId : channelsSortedByName)
                        {
                            auto foundChannel = inputChannelsById.find(inputChannelId);
                            if (foundChannel == inputChannelsById.end())
                            {
                                continue;
                            }

                            const AzFramework::InputChannel* channel = foundChannel->second;

                            // for prettiness sake, sort the channels by name.
                            if (channel)
                            {
                                ImGui::Text("Channel '%s' - Value: %0.4f - Delta %0.4f - State: %s%s%s%s [%s]", 
                                    channel->GetInputChannelId().GetName(),
                                    channel->GetValue(),
                                    channel->GetDelta(),
                                    channel->IsStateIdle() ? "IDLE" : "",
                                    channel->IsStateBegan() ? "BEGAN" : "",
                                    channel->IsStateUpdated() ? "UPDATED" : "",
                                    channel->IsStateEnded() ? "ENDED" : "",
                                    channel->IsActive() ? "ACTIVE" : "INACTIVE");
                            }
                        }
                        ImGui::TreePop(); // End Tree
                    }
                }
            }
        }
    }

    void ImGuiInputMonitor::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }
} // namespace ImGui

#endif // IMGUI_ENABLED
