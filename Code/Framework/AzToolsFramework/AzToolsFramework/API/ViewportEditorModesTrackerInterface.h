/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/API/ViewportEditorModesTrackerNotificationBus.h>

namespace AzToolsFramework
{
    //! The AZ::Interface of the central editor mode state tracker for all viewports.
    class ViewportEditorModesTrackerInterface
    {
    public:
        AZ_RTTI(ViewportEditorModesTrackerInterface, "{7D72A4F7-2147-4ED9-A315-E456A3BE3CF6}");

        virtual ~ViewportEditorModesTrackerInterface() = default;

        //! Enters the specified editor mode for the specified viewport.
        virtual void EnterMode(const ViewportEditorModeInfo& viewportEditorModeInfo, ViewportEditorMode mode) = 0;
       
        //! Exits the specified editor mode for the specified viewport.
        virtual void ExitMode(const ViewportEditorModeInfo& viewportEditorModeInfo, ViewportEditorMode mode) = 0;

        //! Attempts to retrieve the editor mode state for the specified viewport, otherwise returns nullptr.
        virtual const ViewportEditorModesInterface* GetEditorModeState(const ViewportEditorModeInfo& viewportEditorModeInfo) const = 0;

        //! Returns the number of viewports currently being tracked.
        virtual size_t GetNumTrackedViewports() const = 0;

        //! Returns true if the specified viewport is being tracked, otherwise false.
        virtual bool IsViewportStateBeingTracked(const ViewportEditorModeInfo& viewportEditorModeInfo) const = 0;

    private:
    };
} // namespace AzToolsFramework

