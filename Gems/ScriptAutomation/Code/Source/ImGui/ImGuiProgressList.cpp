/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Utils/ImGuiProgressList.h>
#include <Automation/ScriptableImGui.h>

namespace AtomSampleViewer
{
    void ImGuiProgressList::OpenPopup(AZStd::string_view title, AZStd::string_view description, const AZStd::vector<AZStd::string>& itemsList,
        AZStd::function<void()> onUserAction, bool automaticallyCloseOnAction, AZStd::string_view actionButtonLabel)
    {
        AZ_Assert(m_state == State::Closed, "Popup is already open");
        m_title = title;
        m_description = description;
        m_itemsList = itemsList;
        m_onUserAction = onUserAction;
        m_automaticallyCloseOnAction = automaticallyCloseOnAction;
        m_actionButtonLabel = actionButtonLabel;

        m_state = State::Opening;

        AZ::TickBus::Handler::BusConnect();
    }

    void ImGuiProgressList::ClosePopup()
    {
        if (m_state == State::Closed)
        {
            return;
        }

        AZ::TickBus::Handler::BusDisconnect();

        m_onUserAction = nullptr;
        m_state = State::Closed;
    }

    void ImGuiProgressList::AddItem(AZStd::string_view item)
    {
        AZ_Assert(m_state != State::Closed, "Can't add item while this widget is closed.");
        m_itemsList.push_back(item);
    }

    void ImGuiProgressList::RemoveItem(AZStd::string_view item)
    {
        AZ_Assert(m_state != State::Closed, "Can't remove item while this widget is closed.");
        m_itemsList.erase(AZStd::remove(m_itemsList.begin(), m_itemsList.end(), item), m_itemsList.end());
    }

    void ImGuiProgressList::TickPopup()
    {
        if (m_state == State::Opening)
        {
            ImGui::OpenPopup(m_title.c_str());
            m_state = State::Open;
        }

        if (ImGui::IsPopupOpen(m_title.c_str()))
        {
            if (m_itemsList.size() == 0)
            {
                ClosePopup();
                return;
            }
            const ImGuiWindowFlags windowFlags =
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_HorizontalScrollbar;

            if (ImGui::BeginPopupModal(m_title.c_str(), nullptr, windowFlags))
            {
                ImGui::Text("%s", m_description.c_str());

                ScriptableImGui::PushNameContext(m_title.c_str());

                auto listBoxGetter = +[](void* data, int idx, const char** out_text)
                {
                    const auto* itemsList = reinterpret_cast<const AZStd::vector<AZStd::string>*>(data);
                    *out_text = (*itemsList)[idx].c_str();
                    return true;
                };

                ImGui::PushItemWidth(-1.0f);
                ScriptableImGui::ListBox("", &m_selectedItemIndex, listBoxGetter, &m_itemsList, static_cast<int>(m_itemsList.size()));
                ImGui::PopItemWidth();

                if (ScriptableImGui::Button(m_actionButtonLabel.c_str()))
                {
                    if (m_onUserAction)
                    {
                        m_onUserAction();
                    }

                    if (m_automaticallyCloseOnAction)
                    {
                        ImGui::CloseCurrentPopup();
                        ClosePopup();
                    }
                }

                ScriptableImGui::PopNameContext();

                ImGui::EndPopup();
            }
        }
        else if(m_state == State::Open)
        {
            // If another ImGui window is opened while a modal popup is open, it kills the modal popup. So open it again.
            m_state = State::Opening;
        }
    }

    void ImGuiProgressList::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        TickPopup();
    }

} // namespace AtomSampleViewer
