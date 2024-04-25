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
    // Editor main window top MenuBar
    inline constexpr AZStd::string_view EditorMainWindowMenuBarIdentifier = "o3de.menubar.editor.mainwindow";

    // File menus and sub-menus
    // Some of the sub-menus are also shown elsewhere
    inline constexpr AZStd::string_view FileMenuIdentifier = "o3de.menu.editor.file";
    inline constexpr AZStd::string_view RecentFilesMenuIdentifier = "o3de.menu.editor.file.recent";
    inline constexpr AZStd::string_view EditMenuIdentifier = "o3de.menu.editor.edit";
    inline constexpr AZStd::string_view EditModifyMenuIdentifier = "o3de.menu.editor.edit.modify";
    inline constexpr AZStd::string_view EditModifySnapMenuIdentifier = "o3de.menu.editor.edit.modify.snap";
    inline constexpr AZStd::string_view EditModifyModesMenuIdentifier = "o3de.menu.editor.edit.modify.modes";
    inline constexpr AZStd::string_view EditSettingsMenuIdentifier = "o3de.menu.editor.edit.settings";
    inline constexpr AZStd::string_view GameMenuIdentifier = "o3de.menu.editor.game";
    inline constexpr AZStd::string_view PlayGameMenuIdentifier = "o3de.menu.editor.game.play";
    inline constexpr AZStd::string_view GameAudioMenuIdentifier = "o3de.menu.editor.game.audio";
    inline constexpr AZStd::string_view GameDebuggingMenuIdentifier = "o3de.menu.editor.game.debugging";
    inline constexpr AZStd::string_view ToolBoxMacrosMenuIdentifier = "o3de.menu.editor.toolbox.macros";
    inline constexpr AZStd::string_view ToolsMenuIdentifier = "o3de.menu.editor.tools";
    inline constexpr AZStd::string_view ViewMenuIdentifier = "o3de.menu.editor.view";
    inline constexpr AZStd::string_view LayoutsMenuIdentifier = "o3de.menu.editor.view.layouts";
    inline constexpr AZStd::string_view ViewportMenuIdentifier = "o3de.menu.editor.viewport";
    inline constexpr AZStd::string_view GoToLocationMenuIdentifier = "o3de.menu.editor.goToLocation";
    inline constexpr AZStd::string_view SaveLocationMenuIdentifier = "o3de.menu.editor.saveLocation";
    inline constexpr AZStd::string_view HelpMenuIdentifier = "o3de.menu.editor.help";
    inline constexpr AZStd::string_view HelpDocumentationMenuIdentifier = "o3de.menu.editor.help.documentation";
    inline constexpr AZStd::string_view HelpGameDevResourcesMenuIdentifier = "o3de.menu.editor.help.gamedevresources";

    // Viewport menus and sub-menus
    inline constexpr AZStd::string_view ViewportContextMenuIdentifier = "o3de.menu.editor.viewport.context";
    inline constexpr AZStd::string_view ViewportCameraMenuIdentifier = "o3de.menu.editor.viewport.camera";
    inline constexpr AZStd::string_view ViewportHelpersMenuIdentifier = "o3de.menu.editor.viewport.helpers";
    inline constexpr AZStd::string_view ViewportDebugInfoMenuIdentifier = "o3de.menu.editor.viewport.debugInfo";
    inline constexpr AZStd::string_view ViewportSizeMenuIdentifier = "o3de.menu.editor.viewport.size";
    inline constexpr AZStd::string_view ViewportSizeRatioMenuIdentifier = "o3de.menu.editor.viewport.size.ratio";
    inline constexpr AZStd::string_view ViewportSizeResolutionMenuIdentifier = "o3de.menu.editor.viewport.size.resolution";
    inline constexpr AZStd::string_view ViewportOptionsMenuIdentifier = "o3de.menu.editor.viewport.options";

    // Editor Widget Menus
    inline constexpr AZStd::string_view EntityOutlinerContextMenuIdentifier = "o3de.menu.editor.entityOutliner.context";
    inline constexpr AZStd::string_view InspectorEntityComponentContextMenuIdentifier = "o3de.menu.editor.inspector.component.context";
    inline constexpr AZStd::string_view InspectorEntityPropertyContextMenuIdentifier = "o3de.menu.editor.inspector.property.context";

    // Entity creation menu
    inline constexpr AZStd::string_view EntityCreationMenuIdentifier = "o3de.menu.entity.create";
}
