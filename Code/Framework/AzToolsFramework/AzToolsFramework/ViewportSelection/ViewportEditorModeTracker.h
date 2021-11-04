/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerInterface.h>

namespace AzToolsFramework
{
    //! The encapsulation of the editor modes for a given viewport.
    class ViewportEditorModes
        : public ViewportEditorModesInterface
    {
    public:
        //! The number of currently supported viewport editor modes.
        static constexpr AZ::u8 NumEditorModes = 4;

        //! Sets the specified mode as active.
        AZ::Outcome<void, AZStd::string> ActivateMode(ViewportEditorMode mode);

        // Sets the specified mode as inactive.
        AZ::Outcome<void, AZStd::string> DeactivateMode(ViewportEditorMode mode);

        // ViewportEditorModesInterface ...
        bool IsModeActive(ViewportEditorMode mode) const override;
    private:
        AZStd::array<bool, NumEditorModes> m_editorModes{}; //!< State flags to track active/inactive status of viewport editor modes.
    };

    //! The implementation of the central editor mode state tracker for all viewports.
    class ViewportEditorModeTracker
        : public ViewportEditorModeTrackerInterface
    {
    public:
        // ViewportEditorModeTrackerInterface overrides ...
        AZ::Outcome<void, AZStd::string> ActivateMode(const ViewportEditorModeTrackerInfo& ViewportEditorModeTrackerInfo, ViewportEditorMode mode) override;
        AZ::Outcome<void, AZStd::string> DeactivateMode(const ViewportEditorModeTrackerInfo& ViewportEditorModeTrackerInfo, ViewportEditorMode mode) override;
        const ViewportEditorModesInterface* GetViewportEditorModes(const ViewportEditorModeTrackerInfo& ViewportEditorModeTrackerInfo) const override;
        size_t GetTrackedViewportCount() const override;
        bool IsViewportModeTracked(const ViewportEditorModeTrackerInfo& ViewportEditorModeTrackerInfo) const override;

    private:
        using ViewportEditorModesMap = AZStd::unordered_map<typename ViewportEditorModeTrackerInfo::IdType, ViewportEditorModes>;
        ViewportEditorModesMap m_viewportEditorModesMap; //!< Editor mode states per tracker.
    };
} // namespace AzToolsFramework
