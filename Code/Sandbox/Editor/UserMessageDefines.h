/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_USERMESSAGEDEFINES_H
#define CRYINCLUDE_EDITOR_USERMESSAGEDEFINES_H
#pragma once


enum ESandboxUserMessages
{
    // InPlaceComboBox
    WM_USER_ON_SELECTION_CANCEL = WM_USER + 1,
    WM_USER_ON_SELECTION_OK,
    WM_USER_ON_NEW_SELECTION,
    WM_USER_ON_EDITCHANGE,
    WM_USER_ON_OPENDROPDOWN,
    WM_USER_ON_EDITKEYDOWN,
    WM_USER_ON_EDITCLICK,
    // ACListWnd
    ENAC_UPDATE,
    // EditWithButton
    WM_USER_EDITWITHBUTTON_CLICKED,
    // FillSliderCtrl
    WMU_FS_CHANGED,
    WMU_FS_LBUTTONDOWN,
    WMU_FS_LBUTTONUP,
    FLM_EDITTEXTCHANGED,
    FLM_FILTERTEXTCHANGED,
    // NumberCtrlEdit
    WMU_LBUTTONDOWN,
    WMU_LBUTTONUP,
    WM_ONWINDOWFOCUSCHANGES,
    // SelectObjectDialog
    IDT_TIMER_0,
    IDT_TIMER_1,
    // LensFlareEditor
    WM_FLAREEDITOR_UPDATETREECONTROL,
    // EquipPackDialog
    UM_EQUIPLIST_CHECKSTATECHANGE,
    // MaterialSender/MatEditMainDlg
    WM_MATEDITPICK,
    // GridMapWindow
    WM_USER_ON_DBL_CLICK,
    // LMCompDialog
    WM_UPDATE_LIGHTMAP_GENERATION_PROGRESS,
    WM_UPDATE_LIGHTMAP_GENERATION_MEMUSAGE,
    WM_UPDATE_LIGHTMAP_GENERATION_MEMUSAGE_STATIC,
    WM_UPDATE_GLM_NAME_EDIT,
    // Viewport
    WM_VIEWPORT_ON_TITLE_CHANGE,
    // VisualLogControls
    UWM_BUTTON_CLICKED,
};
#endif // CRYINCLUDE_EDITOR_USERMESSAGEDEFINES_H
