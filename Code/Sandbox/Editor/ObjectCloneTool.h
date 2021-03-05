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

// Description : Definition of ObjectCloneTool, edit tool for cloning of objects..


#ifndef CRYINCLUDE_EDITOR_OBJECTCLONETOOL_H
#define CRYINCLUDE_EDITOR_OBJECTCLONETOOL_H

#pragma once

#include "EditTool.h"

class CBaseObject;

namespace AzToolsFramework
{
    namespace UndoSystem
    {
        class URSequencePoint;
    }
}

/*!
 *  CObjectCloneTool, When created duplicate current selection, and manages cloned selection.
 *
 */

class CObjectCloneTool
    : public CEditTool
{
    Q_OBJECT
public:
    Q_INVOKABLE CObjectCloneTool();

    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams();

    virtual void Display(DisplayContext& dc);
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnKeyUp([[maybe_unused]] CViewport* view, [[maybe_unused]] uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags) { return false; };
    //////////////////////////////////////////////////////////////////////////

    void Accept(bool resetPosition = false);
    void Abort();

protected:
    virtual ~CObjectCloneTool();
    // Delete itself.
    void DeleteThis() { delete this; };

private:
    void CloneSelection();
    void SetConstrPlane(CViewport* view, const QPoint& point);

    CSelectionGroup* m_selection;
    Vec3 m_origin;
    bool m_bSetConstrPlane;
    //bool m_bSetCapture;

    void EndUndoBatch();

    AzToolsFramework::UndoSystem::URSequencePoint* m_currentUndoBatch;
};


#endif // CRYINCLUDE_EDITOR_OBJECTCLONETOOL_H
