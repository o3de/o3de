/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>

namespace EntityPresetsIdentifiers
{
    // Menus

    //! The "Create Preset" submenu, hung off the viewport and Entity Outliner context menus.
    //!
    //! This identifier is a public contract. A gem may hang its own submenu or action here for a
    //! preset that cannot be expressed as data - one that touches the level entity, builds a
    //! hierarchy, or wires entities to one another. An ordinary preset needs no code and should
    //! ship as Presets/*.entitypresets.json inside the gem instead; it will be picked up
    //! automatically and needs nothing from this header.
    //!
    //! No handler-ordering arrangement is required. ActionManagerSystemComponent broadcasts each
    //! registration hook to *every* handler before moving on to the next, so this menu - registered
    //! during OnMenuRegistrationHook - already exists by the time any handler receives
    //! OnMenuBindingHook. A gem should register its own menu and actions in the registration hooks
    //! and only call AddSubMenuToMenu against this identifier in the binding hook.
    inline constexpr AZStd::string_view EntityPresetsRootMenuIdentifier = "o3de.menu.editor.entityPresets";

    //! Category submenus are generated from the preset data, so only their prefix can be named
    //! here - the rest is a slug of the category. See EntityPresetMenu::CategoryMenuIdentifierFor.
    //!
    //! Deliberately not part of the contract above: a category menu exists only while some preset
    //! declares that category, so a gem that hung an action inside one would find it come and go
    //! with unrelated preset edits. Gems attach to the root menu instead.
    inline constexpr AZStd::string_view EntityPresetsCategoryMenuIdentifierPrefix = "o3de.menu.editor.entityPresets.";

    //! Likewise, one action is generated per preset, identified by category and name.
    //! See EntityPresetMenu::ActionIdentifierFor.
    inline constexpr AZStd::string_view EntityPresetActionIdentifierPrefix = "o3de.action.editor.entityPreset.";

    // Actions
    inline constexpr AZStd::string_view EntityPresetsManageActionIdentifier = "o3de.action.editor.entityPresets.manage";

    // Sort keys
    //
    // Published as bands rather than as single values because the data-driven part of the menu
    // grows and shrinks: the number of category submenus depends on what presets are loaded, so a
    // gem cannot compute a safe key from outside. Pick a key inside the gem band and it will land
    // below every category and above nothing else that matters.

    //! The first category submenu, with one step between each subsequent one.
    inline constexpr int EntityPresetsCategorySortKeyStart = 1000;
    inline constexpr int EntityPresetsCategorySortKeyStep = 100;

    //! Where gem-contributed entries go: after the categories, before "Manage Presets...".
    //! The gap above the category band leaves room for roughly 990 categories, which is far more
    //! than the menu would stay usable with.
    inline constexpr int EntityPresetsGemSortKeyStart = 100000;

    //! "Manage Presets..." is always last - it manages the list rather than creating anything.
    inline constexpr int EntityPresetsManageSortKey = 1000000;
} // namespace EntityPresetsIdentifiers
