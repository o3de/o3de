/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>

namespace EditorIdentifiers
{
    inline constexpr AZStd::string_view AngleSnappingStateChangedUpdaterIdentifier = "o3de.updater.onAngleSnappingStateChanged";
    inline constexpr AZStd::string_view ComponentModeChangedUpdaterIdentifier = "o3de.updater.onComponentModeChanged";
    inline constexpr AZStd::string_view ComponentSelectionChangedUpdaterIdentifier = "o3de.updater.onComponentSelectionChanged";
    inline constexpr AZStd::string_view ContainerEntityStatesChangedUpdaterIdentifier = "o3de.updater.onContainerEntityStatesChanged";
    inline constexpr AZStd::string_view DrawHelpersStateChangedUpdaterIdentifier = "o3de.updater.onViewportDrawHelpersStateChanged";
    inline constexpr AZStd::string_view EntityPickingModeChangedUpdaterIdentifier = "o3de.updater.onEntityPickingModeChanged";
    inline constexpr AZStd::string_view EntitySelectionChangedUpdaterIdentifier = "o3de.updater.onEntitySelectionChanged";
    inline constexpr AZStd::string_view GameModeStateChangedUpdaterIdentifier = "o3de.updater.onGameModeStateChanged";
    inline constexpr AZStd::string_view GridShowingChangedUpdaterIdentifier = "o3de.updater.onGridShowingChanged";
    inline constexpr AZStd::string_view GridSnappingStateChangedUpdaterIdentifier = "o3de.updater.onGridSnappingStateChanged";
    inline constexpr AZStd::string_view IconsStateChangedUpdaterIdentifier = "o3de.updater.onViewportIconsStateChanged";
    inline constexpr AZStd::string_view LevelLoadedUpdaterIdentifier = "o3de.updater.onLevelLoaded";
    inline constexpr AZStd::string_view RecentFilesChangedUpdaterIdentifier = "o3de.updater.onRecentFilesChanged";
    inline constexpr AZStd::string_view VertexSelectionChangedUpdaterIdentifier = "o3de.updater.onVertexSelectionChanged";
    inline constexpr AZStd::string_view UndoRedoUpdaterIdentifier = "o3de.updater.onUndoRedo";
    inline constexpr AZStd::string_view ViewportDisplayInfoStateChangedUpdaterIdentifier = "o3de.updater.onViewportDisplayInfoStateChanged";
}
