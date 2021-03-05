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

// Description : Button implementation in the MiniGUI


#ifndef CRYINCLUDE_CRYSYSTEM_MINIGUI_MINIMENU_H
#define CRYINCLUDE_CRYSYSTEM_MINIGUI_MINIMENU_H
#pragma once


#include "MiniGUI.h"
#include "MiniButton.h"

MINIGUI_BEGIN

//////////////////////////////////////////////////////////////////////////
// Root window all other controls derive from
class CMiniMenu
    : public CMiniButton
{
public:
    CMiniMenu();

    //////////////////////////////////////////////////////////////////////////
    // CMiniCtrl interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual EMiniCtrlType GetType() const { return eCtrlType_Menu; }
    virtual void SetRect(const Rect& rc);
    virtual void OnPaint(CDrawContext& dc);
    virtual void OnEvent(float x, float y, EMiniCtrlEvent event);
    virtual void AddSubCtrl(IMiniCtrl* pCtrl);
    virtual void Reset();
    virtual void SaveState();
    virtual void RestoreState();
    //////////////////////////////////////////////////////////////////////////

    //digital selection funcs
    CMiniMenu* UpdateSelection(EMiniCtrlEvent event);

    void Open();
    void Close();

protected:

protected:
    bool m_bVisible;
    bool m_bSubMenu;
    float m_menuWidth;

    int m_selectionIndex;
};

MINIGUI_END

#endif // CRYINCLUDE_CRYSYSTEM_MINIGUI_MINIMENU_H
