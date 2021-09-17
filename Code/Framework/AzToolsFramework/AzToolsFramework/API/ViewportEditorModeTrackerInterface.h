/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/API/ViewportEditorModeTrackerNotificationBus.h>

namespace AzToolsFramework
{
    //! The AZ::Interface of the central editor mode tracker for all viewports.
    class ViewportEditorModeTrackerInterface
    {
    public:
        AZ_RTTI(ViewportEditorModeTrackerInterface, "{7D72A4F7-2147-4ED9-A315-E456A3BE3CF6}");

        virtual ~ViewportEditorModeTrackerInterface() = default;

        //! Registers the specified editor mode as active for the specified viewport.
        virtual void RegisterMode(const ViewportEditorModeInfo& viewportEditorModeInfo, ViewportEditorMode mode) = 0;
       
        //! Unregisters the specified editor mode as active for the specified viewport.
        virtual void UnregisterMode(const ViewportEditorModeInfo& viewportEditorModeInfo, ViewportEditorMode mode) = 0;

        //! Attempts to retrieve the editor mode state for the specified viewport, otherwise returns nullptr.
        virtual const ViewportEditorModesInterface* GetViewportEditorModes(const ViewportEditorModeInfo& viewportEditorModeInfo) const = 0;

        //! Returns the number of viewports currently being tracked.
        virtual size_t GetTrackedViewportCount() const = 0;

        //! Returns true if the specified viewport is being tracked, otherwise false.
        virtual bool IsViewportModeTracked(const ViewportEditorModeInfo& viewportEditorModeInfo) const = 0;
    };
} // namespace AzToolsFramework

