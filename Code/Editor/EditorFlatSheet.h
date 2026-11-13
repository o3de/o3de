/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SandboxAPI.h>

namespace EditorFlatSheet
{
    //! Flattened GUI only (AzQtComponents::StyleManager::stylesheetsDisabled): the elements that carry what Editor.qss gave the
    //! widgets of the Editor. CryEdit.cpp registers this function for the sheet (StyleManager::registerFlatSheet) before it sets
    //! the sheet on the main window, and the flattened GUI runs it when the sheet is first set; legacy mode never does.
    SANDBOX_API void RegisterElements();

    //! Flattened GUI only: the elements that carry what EditorPreferencesDialog.qss gave the widgets of the Editor Preferences
    //! dialog, the sheet the dialog sets on itself (EditorPreferencesDialog.cpp:99). CryEdit.cpp registers this function for that
    //! sheet next to Editor.qss's; the flattened GUI runs it when the dialog first sets the sheet, legacy mode never does.
    SANDBOX_API void RegisterPreferencesDialogElements();

    //! Flattened GUI only: the elements that carry what the sheets of the startup logo dialog gave its labels (the splash screen and the
    //! About box, StartupLogoDialog.cpp). Unlike the registrars above this one belongs to no sheet file: the splash is made before the
    //! main window sets Editor.qss, so the dialog calls this itself, before its form is built. Once for the process; legacy mode never
    //! registers.
    SANDBOX_API void RegisterStartupLogoElements();

    //! The class tag of the startup logo dialog in its About mode, as the flattened GUI reads it (Style::addClass on the dialog): the
    //! legacy GUI sets a sheet per mode, and the link rule is the About box's alone. The dialog tags itself in that mode, once it has
    //! named its labels, and polishes the labels below it again (the form polished them while it was built, before the names and the
    //! tag were there; the sheet of the legacy GUI did that when it was set, QStyleSheetStyle::repolish).
    inline constexpr const char* StartupLogoAboutTag = "StartupLogoAbout";
} // namespace EditorFlatSheet
