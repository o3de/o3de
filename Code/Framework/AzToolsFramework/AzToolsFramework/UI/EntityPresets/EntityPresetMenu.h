/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/std/string/string.h>

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        struct Preset;
    }

    //! Puts the entity presets into the viewport and Entity Outliner context menus.
    //!
    //! Each preset is an Action Manager action (so it can take a hotkey), grouped into a submenu
    //! per category under one "Create Preset" submenu.
    namespace EntityPresetMenu
    {
        //! Register an action for every preset. Call from OnActionRegistrationHook.
        AZTF_API void RegisterActions();

        //! Register the category submenus. Call from OnMenuRegistrationHook.
        AZTF_API void RegisterMenus();

        //! Bind the actions into their submenus and hang those off the context menus.
        //! Call from OnMenuBindingHook.
        AZTF_API void BindMenus();

        //! Re-register and re-bind after presets changed. Safe to call at runtime.
        AZTF_API void Refresh();

        //! The action identifier a preset is registered under.
        AZTF_API AZStd::string ActionIdentifierFor(const EntityPresets::Preset& preset);
    } // namespace EntityPresetMenu
} // namespace AzToolsFramework
