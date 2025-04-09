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
    }

    void ImGuiInputMonitor::Shutdown()
    {
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
        using namespace ImGui;
        if (CollapsingHeader("Input Monitor", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
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
                        Text("%s", childText.c_str());
                    }
                    else if (TreeNodeEx(childText.c_str(), 0))
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

                        if (BeginTable("InputChannels", 5, ImGuiTableFlags_Borders))
                        {
                            TableSetupColumn("Channel");
                            TableSetupColumn("Value");
                            TableSetupColumn("Delta");
                            TableSetupColumn("State");
                            TableSetupColumn("Active");
                            TableHeadersRow();
                            for (const auto& inputChannelId : channelsSortedByName)
                            {
                                auto foundChannel = inputChannelsById.find(inputChannelId);
                                if (foundChannel == inputChannelsById.end())
                                {
                                    continue;
                                }

                                const AzFramework::InputChannel* channel = foundChannel->second;

                                if (channel)
                                {
                                    TableNextColumn();
                                    Text("%s", channel->GetInputChannelId().GetName());
                                    TableNextColumn();
                                    Text("%0.4f", channel->GetValue());
                                    TableNextColumn();
                                    Text("%0.4f", channel->GetDelta());
                                    TableNextColumn();
                                    Text("%s%s%s%s", 
                                        channel->IsStateIdle() ? "IDLE" : "",
                                        channel->IsStateBegan() ? "BEGAN" : "",
                                        channel->IsStateUpdated() ? "UPDATED" : "",
                                        channel->IsStateEnded() ? "ENDED" : "");
                                    TableNextColumn();
                                    Text("%s", channel->IsActive() ? "ACTIVE" : "INACTIVE");
                                }
                            }
                            EndTable();
                        }

                        TreePop(); // End Tree
                    }
                }
            }
        }
    }
} // namespace ImGui

#endif // IMGUI_ENABLED
