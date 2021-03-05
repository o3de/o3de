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

// Description : Table implementation in the MiniGUI


#ifndef CRYINCLUDE_CRYSYSTEM_MINIGUI_MINITABLE_H
#define CRYINCLUDE_CRYSYSTEM_MINIGUI_MINITABLE_H
#pragma once


#include "MiniGUI.h"

MINIGUI_BEGIN

class CMiniTable
    : public CMiniCtrl
    , public IMiniTable
    , public AzFramework::InputChannelEventListener
{
public:
    CMiniTable();

    //////////////////////////////////////////////////////////////////////////
    // CMiniCtrl interface implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual EMiniCtrlType GetType() const { return eCtrlType_Table; }
    virtual void OnPaint(CDrawContext& dc);
    virtual void OnEvent(float x, float y, EMiniCtrlEvent event);
    virtual void Reset();
    virtual void SaveState();
    virtual void RestoreState();
    virtual void AutoResize();
    virtual void SetVisible(bool state);

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // IMiniTable interface implementation.
    //////////////////////////////////////////////////////////////////////////

    //Add a new column to the table, return column index
    virtual int AddColumn(const char* name);

    //Remove all columns an associated data
    virtual void RemoveColumns();

    //Add data to specified column (add onto the end), return row index
    virtual int AddData(int columnIndex, ColorB col, const char* format, ...);

    //Clear all data from table
    virtual void ClearTable();

    virtual bool IsHidden() { return CheckFlag(eCtrl_Hidden); }

    virtual void Hide(bool stat);

    // AzFramework::InputChannelEventListener
    bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;
    AZ::s32 GetPriority() const override { return AzFramework::InputChannelEventListener::GetPriorityUI(); }

    static const int MAX_TEXT_LENGTH = 64;

protected:

    struct SCell
    {
        char text[MAX_TEXT_LENGTH];
        ColorB col;
    };

    struct SColumn
    {
        char name[MAX_TEXT_LENGTH];
        float width;
        std::vector<SCell> cells;
    };

    std::vector<SColumn> m_columns;
    int m_pageSize;
    int m_pageNum;
};

MINIGUI_END

#endif // CRYINCLUDE_CRYSYSTEM_MINIGUI_MINITABLE_H
