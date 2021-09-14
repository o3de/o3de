/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ViewportEditorModeStateTrackerNotificationBus.h>
#include <AzToolsFramework/ViewportSelection/ViewportEditorModeStateTracker.h>

namespace AzToolsFramework
{
    static constexpr const char* ViewportEditorModeLogWindow = "ViewportEditorMode";

    void ViewportEditorModeState::SetModeActive(ViewportEditorMode mode)
    {
        if (const AZ::u32 modeIndex = static_cast<AZ::u32>(mode);
            modeIndex < NumEditorModes)
        {
            m_editorModes[modeIndex] = true;
        }
        else
        {
            AZ_Error(ViewportEditorModeLogWindow, false, "Cannot activate mode %u, mode is not recognized", modeIndex)
        }
    }

    void ViewportEditorModeState::SetModeInactive(ViewportEditorMode mode)
    {
        if (const AZ::u32 modeIndex = static_cast<AZ::u32>(mode); modeIndex < NumEditorModes)
        {
            m_editorModes[modeIndex] = false;
        }
        else
        {
            AZ_Error(ViewportEditorModeLogWindow, false, "Cannot deactivate mode %u, mode is not recognized", modeIndex)
        }
    }

    bool ViewportEditorModeState::IsModeActive(ViewportEditorMode mode) const
    {
        return m_editorModes[static_cast<AZ::u32>(mode)];
    }

    void ViewportEditorModeStateTracker::RegisterInterface()
    {
        if (AZ::Interface<ViewportEditorModeStateTrackerInterface>::Get() == nullptr)
        {
            AZ::Interface<ViewportEditorModeStateTrackerInterface>::Register(this);
        }
    }

    void ViewportEditorModeStateTracker::UnregisterInterface()
    {
        if (AZ::Interface<ViewportEditorModeStateTrackerInterface>::Get() != nullptr)
        {
            AZ::Interface<ViewportEditorModeStateTrackerInterface>::Unregister(this);
        }
    }

    void ViewportEditorModeStateTracker::EnterMode(const ViewportEditorModeInfo& viewportEditorModeInfo, ViewportEditorMode mode)
    {
        auto& editorModeStates = m_viewportEditorModeStates[viewportEditorModeInfo.m_id];
        AZ_Warning(
                ViewportEditorModeLogWindow, !editorModeStates.IsModeActive(mode),
                AZStd::string::format(
                    "Duplicate call to EnterMode for mode '%u' on id '%i'", static_cast<AZ::u32>(mode), viewportEditorModeInfo.m_id).c_str());
        editorModeStates.SetModeActive(mode);
        ViewportEditorModeNotificationsBus::Event(
            viewportEditorModeInfo.m_id, &ViewportEditorModeNotificationsBus::Events::OnEditorModeEnter, editorModeStates, mode);
    }

    void ViewportEditorModeStateTracker::ExitMode(const ViewportEditorModeInfo& viewportEditorModeInfo, ViewportEditorMode mode)
    {
        ViewportEditorModeState* editorModeStates = nullptr;
        if (m_viewportEditorModeStates.count(viewportEditorModeInfo.m_id))
        {
            editorModeStates = &m_viewportEditorModeStates.at(viewportEditorModeInfo.m_id);
            AZ_Warning(
                ViewportEditorModeLogWindow, editorModeStates->IsModeActive(mode),
                AZStd::string::format(
                    "Duplicate call to ExitMode for mode '%u' on id '%i'", static_cast<AZ::u32>(mode), viewportEditorModeInfo.m_id).c_str());
        }
        else
        {
            AZ_Warning(
                ViewportEditorModeLogWindow, false, "Call to ExitMode for mode '%u' on id '%i' without precursor call to EnterMode",
                static_cast<AZ::u32>(mode), viewportEditorModeInfo.m_id);

            editorModeStates = &m_viewportEditorModeStates[viewportEditorModeInfo.m_id];
        }

        editorModeStates->SetModeInactive(mode);
        ViewportEditorModeNotificationsBus::Event(
            viewportEditorModeInfo.m_id, &ViewportEditorModeNotificationsBus::Events::OnEditorModeExit, *editorModeStates, mode);
    }

    const ViewportEditorModeStateInterface* ViewportEditorModeStateTracker::GetEditorModeState(const ViewportEditorModeInfo& viewportEditorModeInfo) const
    {
        if (auto editorModeStates = m_viewportEditorModeStates.find(viewportEditorModeInfo.m_id);
            editorModeStates != m_viewportEditorModeStates.end())
        {
            return &editorModeStates->second;
        }
        else
        {
            return nullptr;
        }
    }

    size_t ViewportEditorModeStateTracker::GetNumTrackedViewports() const
    {
        return m_viewportEditorModeStates.size();
    }

    bool ViewportEditorModeStateTracker::IsViewportStateBeingTracked(const ViewportEditorModeInfo& viewportEditorModeInfo) const
    {
        return m_viewportEditorModeStates.count(viewportEditorModeInfo.m_id) > 0;
    }
} // namespace AzToolsFramework
