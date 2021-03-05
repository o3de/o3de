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

// Description : Definition of PickObjectTool, tool used to pick objects.


#ifndef CRYINCLUDE_EDITOR_PICKOBJECTTOOL_H
#define CRYINCLUDE_EDITOR_PICKOBJECTTOOL_H

#if !defined(Q_MOC_RUN)
#include "EditTool.h"
#include "IObjectManager.h"
#endif

#pragma once

//////////////////////////////////////////////////////////////////////////
class CPickObjectTool
    : public CEditTool
    , public IObjectSelectCallback
{
    Q_OBJECT
public:
    CPickObjectTool(IPickObjectCallback* callback, const QMetaObject* targetClass = NULL);

    //! If set to true, pick tool will not stop picking after first pick.
    void SetMultiplePicks(bool bEnable) { m_bMultiPick = bEnable; };

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    virtual void BeginEditParams(IEditor* ie, int flags);
    virtual void EndEditParams() {};

    virtual void Display([[maybe_unused]] DisplayContext& dc) {};
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnKeyUp([[maybe_unused]] CViewport* view, [[maybe_unused]] uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags) { return false; };

    //////////////////////////////////////////////////////////////////////////
    // IObjectSelectCallback
    //////////////////////////////////////////////////////////////////////////
    virtual bool OnSelectObject(CBaseObject* obj);
    virtual bool CanSelectObject(CBaseObject* obj);
    //////////////////////////////////////////////////////////////////////////

    virtual bool IsNeedSpecificBehaviorForSpaceAcce();

protected:
    virtual ~CPickObjectTool();
    // Delete itself.
    void DeleteThis() { delete this; };

private:
    bool IsRelevant(CBaseObject* obj);

    //! Object that requested pick.
    IPickObjectCallback* m_callback;

    //! If target class specified, will pick only objects that belongs to that runtime class.
    const QMetaObject* m_targetClass;

    bool m_bMultiPick;
};


#endif // CRYINCLUDE_EDITOR_PICKOBJECTTOOL_H
