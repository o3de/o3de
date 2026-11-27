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

    //! The "Create Preset" submenu in the viewport and Entity Outliner context menus.
    //!
    //! Gems may attach a submenu or action here for a preset that cannot be expressed as data.
    //! Register menus and actions in the registration hooks and attach them in OnMenuBindingHook;
    //! every handler finishes the registration hooks before any binding hook runs.
    inline constexpr AZStd::string_view EntityPresetsRootMenuIdentifier = "o3de.menu.editor.entityPresets";

    //! Prefix of the generated per-category submenus. These come and go with the presets, so gems
    //! should attach to the root menu instead.
    inline constexpr AZStd::string_view EntityPresetsCategoryMenuIdentifierPrefix = "o3de.menu.editor.entityPresets.";

    //! Prefix of the generated per-preset actions.
    inline constexpr AZStd::string_view EntityPresetActionIdentifierPrefix = "o3de.action.editor.entityPreset.";

    // Actions
    inline constexpr AZStd::string_view EntityPresetsManageActionIdentifier = "o3de.action.editor.entityPresets.manage";

    // Sort keys. The number of categories varies, so gems get a band of their own.

    //! The first category submenu, with one step between each subsequent one.
    inline constexpr int EntityPresetsCategorySortKeyStart = 1000;
    inline constexpr int EntityPresetsCategorySortKeyStep = 100;

    //! Gem entries: after the categories, before "Manage Presets...".
    inline constexpr int EntityPresetsGemSortKeyStart = 100000;

    //! "Manage Presets..." is always last.
    inline constexpr int EntityPresetsManageSortKey = 1000000;
} // namespace EntityPresetsIdentifiers
