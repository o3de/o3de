/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/ViewportSelection/ViewportEditorModeTracker.h>

namespace AzToolsFramework
{
    static constexpr const char* ViewportEditorModeLogWindow = "ViewportEditorMode";

    void ViewportEditorModes::SetModeActive(ViewportEditorMode mode)
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

    void ViewportEditorModes::SetModeInactive(ViewportEditorMode mode)
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

    bool ViewportEditorModes::IsModeActive(ViewportEditorMode mode) const
    {
        return m_editorModes[static_cast<AZ::u32>(mode)];
    }

    void ViewportEditorModeTracker::RegisterInterface()
    {
        if (AZ::Interface<ViewportEditorModeTrackerInterface>::Get() == nullptr)
        {
            AZ::Interface<ViewportEditorModeTrackerInterface>::Register(this);
        }
    }

    void ViewportEditorModeTracker::UnregisterInterface()
    {
        if (AZ::Interface<ViewportEditorModeTrackerInterface>::Get() != nullptr)
        {
            AZ::Interface<ViewportEditorModeTrackerInterface>::Unregister(this);
        }
    }

    void ViewportEditorModeTracker::RegisterMode(const ViewportEditorModeInfo& viewportEditorModeInfo, ViewportEditorMode mode)
    {
        auto& editorModes = m_viewportEditorModesMap[viewportEditorModeInfo.m_id];
        AZ_Warning(
                ViewportEditorModeLogWindow, !editorModes.IsModeActive(mode),
                AZStd::string::format(
                    "Duplicate call to RegisterMode for mode '%u' on id '%i'", static_cast<AZ::u32>(mode), viewportEditorModeInfo.m_id).c_str());
        editorModes.SetModeActive(mode);
        ViewportEditorModeNotificationsBus::Event(
            viewportEditorModeInfo.m_id, &ViewportEditorModeNotificationsBus::Events::OnEditorModeEnter, editorModes, mode);
    }

    void ViewportEditorModeTracker::UnregisterMode(const ViewportEditorModeInfo& viewportEditorModeInfo, ViewportEditorMode mode)
    {
        ViewportEditorModes* editorModes = nullptr;
        if (m_viewportEditorModesMap.count(viewportEditorModeInfo.m_id))
        {
            editorModes = &m_viewportEditorModesMap.at(viewportEditorModeInfo.m_id);
            AZ_Warning(
                ViewportEditorModeLogWindow, editorModes->IsModeActive(mode),
                AZStd::string::format(
                    "Duplicate call to UnregisterMode for mode '%u' on id '%i'", static_cast<AZ::u32>(mode), viewportEditorModeInfo.m_id).c_str());
        }
        else
        {
            AZ_Warning(
                ViewportEditorModeLogWindow, false, "Call to UnregisterMode for mode '%u' on id '%i' without precursor call to RegisterMode",
                static_cast<AZ::u32>(mode), viewportEditorModeInfo.m_id);

            editorModes = &m_viewportEditorModesMap[viewportEditorModeInfo.m_id];
        }

        editorModes->SetModeInactive(mode);
        ViewportEditorModeNotificationsBus::Event(
            viewportEditorModeInfo.m_id, &ViewportEditorModeNotificationsBus::Events::OnEditorModeExit, *editorModes, mode);
    }

    const ViewportEditorModesInterface* ViewportEditorModeTracker::GetViewportEditorModes(const ViewportEditorModeInfo& viewportEditorModeInfo) const
    {
        if (auto editorModes = m_viewportEditorModesMap.find(viewportEditorModeInfo.m_id);
            editorModes != m_viewportEditorModesMap.end())
        {
            return &editorModes->second;
        }
        else
        {
            return nullptr;
        }
    }

    size_t ViewportEditorModeTracker::GetTrackedViewportCount() const
    {
        return m_viewportEditorModesMap.size();
    }

    bool ViewportEditorModeTracker::IsViewportModeTracked(const ViewportEditorModeInfo& viewportEditorModeInfo) const
    {
        return m_viewportEditorModesMap.count(viewportEditorModeInfo.m_id) > 0;
    }
} // namespace AzToolsFramework
