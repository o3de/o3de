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
    // Editor main window top ToolBarArea
    inline constexpr AZStd::string_view MainWindowTopToolBarAreaIdentifier = "o3de.toolbararea.editor.mainwindow.top";

    // Editor ToolBars (shown in the top ToolBar area by default)
    inline constexpr AZStd::string_view ToolsToolBarIdentifier = "o3de.toolbar.editor.tools";
    inline constexpr AZStd::string_view PlayControlsToolBarIdentifier = "o3de.toolbar.editor.playcontrols";

    // Viewport ToolBar
    inline constexpr AZStd::string_view ViewportTopToolBarIdentifier = "o3de.toolbar.viewport.top";
}
