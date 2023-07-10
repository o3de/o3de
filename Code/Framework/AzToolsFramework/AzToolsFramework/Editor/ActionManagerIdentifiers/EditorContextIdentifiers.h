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
    // Main Window
    inline constexpr AZStd::string_view MainWindowActionContextIdentifier = "o3de.context.editor.mainwindow";

    // Main Window Widgets
    inline constexpr AZStd::string_view EditorAssetBrowserActionContextIdentifier = "o3de.context.editor.assetbrowser";
    inline constexpr AZStd::string_view EditorConsoleActionContextIdentifier = "o3de.context.editor.console";
    inline constexpr AZStd::string_view EditorEntityPropertyEditorActionContextIdentifier = "o3de.context.editor.entitypropertyeditor";
}
