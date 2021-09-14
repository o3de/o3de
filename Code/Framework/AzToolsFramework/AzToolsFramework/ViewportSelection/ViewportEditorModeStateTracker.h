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
#include <AzToolsFramework/API/ViewportEditorModeStateTrackerNotificationBus.h>
#include <AzToolsFramework/API/ViewportEditorModeStateTrackerInterface.h>

namespace AzToolsFramework
{
    //! The encapsulation of the editor modes for a given viewport.
    class ViewportEditorModeState
        : public ViewportEditorModeStateInterface
    {
    public:

        //! The number of currently supported viewport editor modes.
        static constexpr AZ::u8 NumEditorModes = 4;

        //! Sets the specified mode as active.
        void SetModeActive(ViewportEditorMode mode);

        // Sets the specified mode as inactive.
        void SetModeInactive(ViewportEditorMode mode);

        // ViewportEditorModeStateInterface ...
        bool IsModeActive(ViewportEditorMode mode) const override;
    private:
        AZStd::array<bool, NumEditorModes> m_editorModes{}; //!< State flags to track active/inactive status of viewport editor modes.
    };

    //! The implementation of the central editor mode state tracker for all viewports.
    class ViewportEditorModeStateTracker
        : public ViewportEditorModeStateTrackerInterface
    {
    public:
        //! Registers this object with the AZ::Interface.
        void RegisterInterface();

        //! Unregisters this object with the AZ::Interface.
        void UnregisterInterface();

        // ViewportEditorModeStateTrackerInterface ...
        void EnterMode(const ViewportEditorModeInfo& viewportEditorModeInfo, ViewportEditorMode mode) override;
        void ExitMode(const ViewportEditorModeInfo& viewportEditorModeInfo, ViewportEditorMode mode) override;
        const ViewportEditorModeStateInterface* GetEditorModeState(const ViewportEditorModeInfo& viewportEditorModeInfo) const override;
        size_t GetNumTrackedViewports() const override;
        bool IsViewportStateBeingTracked(const ViewportEditorModeInfo& viewportEditorModeInfo) const override;

    private:
        using ViewportEditorModeStates = AZStd::unordered_map<typename ViewportEditorModeInfo::IdType, ViewportEditorModeState>;
        ViewportEditorModeStates m_viewportEditorModeStates; //!< Editor mode state per viewport.
    };
} // namespace AzToolsFramework
