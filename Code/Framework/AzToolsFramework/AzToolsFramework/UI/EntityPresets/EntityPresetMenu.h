/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/utils.h>

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        struct Preset;
    }

    //! Puts the entity presets into the editor's right-click menus.
    //!
    //! Each preset becomes a registered action, grouped into a submenu per category, with the
    //! categories collected under one "Create Preset" submenu hung off the viewport and Entity
    //! Outliner context menus.
    //!
    //! Going through the Action Manager rather than building a QMenu directly means the presets
    //! behave like every other editor command: they can be given hotkeys through the Hotkey
    //! Editor, they appear in the editor's own shortcut settings, and their enabled state is
    //! managed rather than hand-maintained.
    namespace EntityPresetMenu
    {
        //! Register an action for every preset. Call from OnActionRegistrationHook.
        AZTF_API void RegisterActions();

        //! Register the category submenus. Call from OnMenuRegistrationHook.
        AZTF_API void RegisterMenus();

        //! Bind the actions into their submenus and hang those off the context menus.
        //! Call from OnMenuBindingHook.
        AZTF_API void BindMenus();

        //! Re-register and re-bind after the user has edited their presets.
        //!
        //! Safe to call at runtime: AddActionToMenu prompts a menu refresh, so a preset added in
        //! the panel shows up on the next right click rather than after a restart.
        AZTF_API void Refresh();

        //! The hand-written Terrain actions, as (display name, action identifier) pairs.
        //!
        //! Exposed for the same reason as the preset identifiers below: they live only in context
        //! menus, so anything enumerating what this gem offers has to be told about them rather
        //! than finding them by walking the menu bar.
        AZTF_API AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>> TerrainActions();

        //! The action identifier a preset is registered under.
        //!
        //! Exposed so callers outside the menu - the Python command palette, via the reflected bus
        //! - can trigger a preset through the Action Manager rather than reimplementing creation.
        AZTF_API AZStd::string ActionIdentifierFor(const EntityPresets::Preset& preset);
    } // namespace EntityPresetMenu
} // namespace AzToolsFramework
