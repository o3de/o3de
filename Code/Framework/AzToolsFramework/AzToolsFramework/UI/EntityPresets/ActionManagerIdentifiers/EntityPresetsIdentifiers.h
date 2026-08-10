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
    inline constexpr AZStd::string_view EntityPresetsRootMenuIdentifier = "o3de.menu.editor.entityPresets";
    inline constexpr AZStd::string_view EntityPresetsTerrainMenuIdentifier = "o3de.menu.editor.entityPresets.terrain";

    //! Category submenus are generated from the preset data, so only their prefix can be named
    //! here - the rest is a slug of the category. See EntityPresetMenu::CategoryMenuIdentifierFor.
    inline constexpr AZStd::string_view EntityPresetsCategoryMenuIdentifierPrefix = "o3de.menu.editor.entityPresets.";

    //! Likewise, one action is generated per preset, identified by category and name.
    //! See EntityPresetMenu::ActionIdentifierFor.
    inline constexpr AZStd::string_view EntityPresetActionIdentifierPrefix = "o3de.action.editor.entityPreset.";

    // Actions
    inline constexpr AZStd::string_view EntityPresetsManageActionIdentifier = "o3de.action.editor.entityPresets.manage";
}
