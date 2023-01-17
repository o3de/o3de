/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>

namespace EditorActionUpdater
{
    static constexpr AZStd::string_view AngleSnappingStateChangedUpdaterIdentifier = "o3de.updater.onAngleSnappingStateChanged";
    static constexpr AZStd::string_view DrawHelpersStateChangedUpdaterIdentifier = "o3de.updater.onViewportDrawHelpersStateChanged";
    static constexpr AZStd::string_view ComponentModeChangedUpdaterIdentifier = "o3de.updater.onComponentModeChanged";
    static constexpr AZStd::string_view EntitySelectionChangedUpdaterIdentifier = "o3de.updater.onEntitySelectionChanged";
    static constexpr AZStd::string_view GameModeStateChangedUpdaterIdentifier = "o3de.updater.onGameModeStateChanged";
    static constexpr AZStd::string_view GridShowingChangedUpdaterIdentifier = "o3de.updater.onGridShowingChanged";
    static constexpr AZStd::string_view GridSnappingStateChangedUpdaterIdentifier = "o3de.updater.onGridSnappingStateChanged";
    static constexpr AZStd::string_view IconsStateChangedUpdaterIdentifier = "o3de.updater.onViewportIconsStateChanged";
    static constexpr AZStd::string_view OnlyShowHelpersForSelectedEntitiesIdentifier ="o3de.updater.onOnlyShowHelpersForSelectedEntitiesChanged";
    static constexpr AZStd::string_view LevelLoadedUpdaterIdentifier = "o3de.updater.onLevelLoaded";
    static constexpr AZStd::string_view RecentFilesChangedUpdaterIdentifier = "o3de.updater.onRecentFilesChanged";
    static constexpr AZStd::string_view UndoRedoUpdaterIdentifier = "o3de.updater.onUndoRedo";
    static constexpr AZStd::string_view ViewportDisplayInfoStateChangedUpdaterIdentifier = "o3de.updater.onViewportDisplayInfoStateChanged";
}
