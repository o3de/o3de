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

#ifndef CRYINCLUDE_CRYSYSTEM_MINIGUI_MINIINFOBOX_H
#define CRYINCLUDE_CRYSYSTEM_MINIGUI_MINIINFOBOX_H
#pragma once


#include "MiniGUI.h"

MINIGUI_BEGIN

//////////////////////////////////////////////////////////////////////////
// Root window all other controls derive from
class CMiniInfoBox
    : public CMiniCtrl
    , public IMiniInfoBox
{
public:
    CMiniInfoBox();

    //////////////////////////////////////////////////////////////////////////
    // CMiniCtrl interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual EMiniCtrlType GetType() const { return eCtrlType_InfoBox; }
    virtual void OnPaint(CDrawContext& dc);
    virtual void OnEvent(float x, float y, EMiniCtrlEvent event);
    virtual void Reset();
    virtual void SaveState();
    virtual void RestoreState();
    virtual void AutoResize();
    //////////////////////////////////////////////////////////////////////////

    virtual void SetTextIndent(float x) { m_fTextIndent = x; }
    virtual void SetTextSize(float sz) { m_fTextSize = sz; }
    virtual void ClearEntries();
    virtual void AddEntry(const char* str, ColorB col, float textSize);

    //////////////////////////////////////////////////////////////////////////
    // IMiniTable interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual bool IsHidden() { return CheckFlag(eCtrl_Hidden); }
    virtual void Hide(bool stat) { SetVisible(!stat); }

public:

    //////////////////////////////////////////////////////////////////////////

    static const int MAX_TEXT_LENGTH = 64;

    struct SInfoEntry
    {
        char text[MAX_TEXT_LENGTH];
        ColorB color;
        float textSize;
    };
    //////////////////////////////////////////////////////////////////////////

protected:

    std::vector<SInfoEntry> m_entries;
    float m_fTextIndent;
};

MINIGUI_END

#endif // CRYINCLUDE_CRYSYSTEM_MINIGUI_MINIINFOBOX_H
