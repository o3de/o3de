/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzToolsFramework
{
    inline constexpr const char* DefaultActionContextModeIdentifier = "default";

    //! Action Visibility enum.
    //! Determines whether an action should be displayed in a Menu or ToolBar.
    //! The action will be greyed out if shown while disabled by an enabled state callback.
    enum class ActionVisibility
    {
        AlwaysShow = 0,     //!< Action is always shown even in Action Context Modes where it is not available.
        OnlyInActiveMode,   //!< Action is only shown in Action Context Modes it was added to.
        HideWhenDisabled    //!< Action is only shown when enabled, and hidden in all other cases.
    };

    inline bool IsActionVisible(ActionVisibility actionVisibility, bool actionIsActiveInCurrentMode, bool actionIsEnabled)
    {
        return  (actionVisibility == ActionVisibility::AlwaysShow) ||
                (actionVisibility == ActionVisibility::OnlyInActiveMode && actionIsActiveInCurrentMode) ||
                (actionVisibility == ActionVisibility::HideWhenDisabled && actionIsEnabled);
    }

} // namespace AzToolsFramework
