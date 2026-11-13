/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

class QMenu;

//////////////////////////////////////////////////////////////////////////
//  UI EDITOR, FLATTENED GUI
//
//  The UI Editor's menus gave themselves a sheet of one rule, `QMenu::item:disabled { color: rgb(127, 127, 127) }`
//  (UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET, EditorCommon.h), which outranked the application sheet's rule for the same item
//  (Menu.qss:23-26). The legacy GUI sets that sheet. The flattened GUI sets none: its menus are tagged, and the element
//  LyShine.Menu.UiEditorPopup draws the grey (UiEditorFlat.cpp). The value is UiEditorFlat::DisabledText (EditorCommon.h).
//
//  A named namespace, the one EditorCommon.h opens: a unity build merges the anonymous namespaces of a target's files.
//////////////////////////////////////////////////////////////////////////
namespace UiEditorFlat
{
    //! The class tag the flattened GUI reads on a menu (Style::addClass), and the name of the element it gets
    inline constexpr const char* PopupMenuTag = "UiEditorPopup";
    inline constexpr const char* PopupMenuElementName = "LyShine.Menu.UiEditorPopup";

    //! The one line of a menu site whose disabled items show: where the menu is made, before any sub-menu is added to it and before
    //! it is shown. The legacy GUI gets the menu's own sheet; the flattened GUI gets the tag (and the element is registered, once,
    //! the first time it is needed).
    void SetPopupMenuSheet(QMenu* menu);
} // namespace UiEditorFlat
