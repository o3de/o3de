/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>

namespace AzToolsFramework
{
    //! The AZ::Interface of the central editor mode tracker for all viewports.
    class ViewportEditorModeTrackerInterface
    {
    public:
        AZ_RTTI(ViewportEditorModeTrackerInterface, "{7D72A4F7-2147-4ED9-A315-E456A3BE3CF6}");

        virtual ~ViewportEditorModeTrackerInterface() = default;

        //! Activates the specified editor mode for the specified viewport editor mode tracker.
        virtual AZ::Outcome<void, AZStd::string> ActivateMode(
            const ViewportEditorModeTrackerInfo& ViewportEditorModeTrackerInfo, ViewportEditorMode mode) = 0;
       
        //! Deactivates the specified editor mode for the specified viewport editor mode tracker.
        virtual AZ::Outcome<void, AZStd::string> DeactivateMode(
            const ViewportEditorModeTrackerInfo& ViewportEditorModeTrackerInfo, ViewportEditorMode mode) = 0;

        //! Attempts to retrieve the editor mode state for the specified viewport editor mode tracker, otherwise returns nullptr.
        virtual const ViewportEditorModesInterface* GetViewportEditorModes(const ViewportEditorModeTrackerInfo& ViewportEditorModeTrackerInfo) const = 0;

        //! Returns the number of viewport editor mode trackers.
        virtual size_t GetTrackedViewportCount() const = 0;

        //! Returns true if viewport editor modes are being tracked for the specified od, otherwise false.
        virtual bool IsViewportModeTracked(const ViewportEditorModeTrackerInfo& ViewportEditorModeTrackerInfo) const = 0;
    };
} // namespace AzToolsFramework

