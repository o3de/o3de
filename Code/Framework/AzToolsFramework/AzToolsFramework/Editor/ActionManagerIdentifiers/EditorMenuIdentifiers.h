/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>

namespace EditorMenu
{
    static constexpr AZStd::string_view EditorMainWindowMenuBarIdentifier = "o3de.menubar.editor.mainwindow";

    static constexpr AZStd::string_view FileMenuIdentifier = "o3de.menu.editor.file";
    static constexpr AZStd::string_view RecentFilesMenuIdentifier = "o3de.menu.editor.file.recent";
    static constexpr AZStd::string_view EditMenuIdentifier = "o3de.menu.editor.edit";
    static constexpr AZStd::string_view EditModifyMenuIdentifier = "o3de.menu.editor.edit.modify";
    static constexpr AZStd::string_view EditModifySnapMenuIdentifier = "o3de.menu.editor.edit.modify.snap";
    static constexpr AZStd::string_view EditModifyModesMenuIdentifier = "o3de.menu.editor.edit.modify.modes";
    static constexpr AZStd::string_view EditSettingsMenuIdentifier = "o3de.menu.editor.edit.settings";
    static constexpr AZStd::string_view GameMenuIdentifier = "o3de.menu.editor.game";
    static constexpr AZStd::string_view PlayGameMenuIdentifier = "o3de.menu.editor.game.play";
    static constexpr AZStd::string_view GameAudioMenuIdentifier = "o3de.menu.editor.game.audio";
    static constexpr AZStd::string_view GameDebuggingMenuIdentifier = "o3de.menu.editor.game.debugging";
    static constexpr AZStd::string_view ToolBoxMacrosMenuIdentifier = "o3de.menu.editor.toolbox.macros";
    static constexpr AZStd::string_view ToolsMenuIdentifier = "o3de.menu.editor.tools";
    static constexpr AZStd::string_view ViewMenuIdentifier = "o3de.menu.editor.view";
    static constexpr AZStd::string_view LayoutsMenuIdentifier = "o3de.menu.editor.view.layouts";
    static constexpr AZStd::string_view ViewportMenuIdentifier = "o3de.menu.editor.viewport";
    static constexpr AZStd::string_view GoToLocationMenuIdentifier = "o3de.menu.editor.goToLocation";
    static constexpr AZStd::string_view SaveLocationMenuIdentifier = "o3de.menu.editor.saveLocation";
    static constexpr AZStd::string_view HelpMenuIdentifier = "o3de.menu.editor.help";
    static constexpr AZStd::string_view HelpDocumentationMenuIdentifier = "o3de.menu.editor.help.documentation";
    static constexpr AZStd::string_view HelpGameDevResourcesMenuIdentifier = "o3de.menu.editor.help.gamedevresources";
}
