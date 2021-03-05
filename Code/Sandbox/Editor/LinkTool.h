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

// Description : Definition of CLinkTool, tool used to link objects.


#ifndef CRYINCLUDE_EDITOR_LINKTOOL_H
#define CRYINCLUDE_EDITOR_LINKTOOL_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Component/EntityBus.h>
#include "EditTool.h"
#include "Include/IObjectManager.h"
#endif

class CEntityObject;

//////////////////////////////////////////////////////////////////////////
class CLinkTool
    : public CEditTool
    , public IObjectSelectCallback
    , private AZ::EntitySystemBus::Handler
{
    Q_OBJECT
public:
    Q_INVOKABLE CLinkTool(); // IPickObjectCallback *callback,CRuntimeClass *targetClass=NULL );

    // Ovverides from CEditTool
    bool MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);

    virtual void BeginEditParams([[maybe_unused]] IEditor* ie, [[maybe_unused]] int flags) {};
    virtual void EndEditParams() {};

    virtual void Display(DisplayContext& dc);
    virtual bool OnKeyDown(CViewport* view, uint32 nChar, uint32 nRepCnt, uint32 nFlags);
    virtual bool OnKeyUp([[maybe_unused]] CViewport* view, [[maybe_unused]] uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags) { return false; };

    virtual bool OnSelectObject([[maybe_unused]] CBaseObject* obj) {return false; }
    virtual bool CanSelectObject([[maybe_unused]] CBaseObject* obj) { return true; };

    virtual bool OnSetCursor(CViewport* vp);

    void LinkSelectedToParent(CBaseObject* pParent);

protected:
    virtual ~CLinkTool();
    // Delete itself.
    void DeleteThis() { delete this; };

private:
    bool IsRelevant([[maybe_unused]] CBaseObject* obj) { return true; }
    bool ChildIsValid(CBaseObject* pParent, CBaseObject* pChild, int nDir = 3);
    void LinkObject(CBaseObject* pChild, CBaseObject* pParent);
    void LinkToNode(CEntityObject* pChild, CEntityObject* pParent, const char* nodeName);

    // AZ::EntitySystemBus::Handler
    void OnEntityDestruction(const AZ::EntityId& entityId) override;


    CBaseObject* m_pChild;
    Vec3 m_StartDrag;
    Vec3 m_EndDrag;

    QCursor m_hLinkCursor;
    QCursor m_hLinkNowCursor;
    QCursor* m_hCurrCursor;

    const char* m_nodeName;
    IGeomCacheRenderNode* m_pGeomCacheRenderNode;
};


#endif // CRYINCLUDE_EDITOR_LINKTOOL_H
