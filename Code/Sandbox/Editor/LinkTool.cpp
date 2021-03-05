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

#include "EditorDefs.h"

#include "LinkTool.h"

// Editor
#include "Viewport.h"
#include "Objects/EntityObject.h"
#include "Objects/SelectionGroup.h"
#include <Plugins/ComponentEntityEditorPlugin/Objects/ComponentEntityObject.h>

// AzCore
#include <AzCore/Component/Entity.h>

#ifdef LoadCursor
#undef LoadCursor
#endif

namespace
{
    const float kGeomCacheNodePivotSizeScale = 0.0025f;
}

//////////////////////////////////////////////////////////////////////////
CLinkTool::CLinkTool()
    : m_nodeName(nullptr)
    , m_pGeomCacheRenderNode(nullptr)
{
    m_pChild = NULL;
    SetStatusText("Click on object and drag a link to a new parent");

    m_hLinkCursor = CMFCUtils::LoadCursor(IDC_POINTER_LINK);
    m_hLinkNowCursor = CMFCUtils::LoadCursor(IDC_POINTER_LINKNOW);
    m_hCurrCursor = &m_hLinkCursor;
    AZ::EntitySystemBus::Handler::BusConnect();
}

//////////////////////////////////////////////////////////////////////////
CLinkTool::~CLinkTool()
{
    AZ::EntitySystemBus::Handler::BusDisconnect();
}

//////////////////////////////////////////////////////////////////////////
void CLinkTool::LinkObject(CBaseObject* pChild, CBaseObject* pParent)
{
    if (pChild == NULL)
    {
        return;
    }

    if (ChildIsValid(pParent, pChild))
    {
        CUndo undo("Link Object");

        if (qobject_cast<CEntityObject*>(pChild))
        {
            static_cast<CEntityObject*>(pChild)->SetAttachTarget("");
            static_cast<CEntityObject*>(pChild)->SetAttachType(CEntityObject::eAT_Pivot);
        }

        pParent->AttachChild(pChild, true);

        QString str;
        str = tr("%1 attached to %2").arg(pChild->GetName(), pParent->GetName());
        SetStatusText(str);
    }
    else
    {
        SetStatusText("Error: Cyclic linking or already linked.");
    }
}

//////////////////////////////////////////////////////////////////////////
void CLinkTool::LinkSelectedToParent(CBaseObject* pParent)
{
    if (pParent)
    {
        if (IsRelevant(pParent))
        {
            CSelectionGroup* pSel = GetIEditor()->GetSelection();
            if (!pSel->GetCount())
            {
                return;
            }
            CUndo undo("Link Object(s)");
            for (int i = 0; i < pSel->GetCount(); i++)
            {
                CBaseObject* pChild = pSel->GetObject(i);
                if (pChild == pParent)
                {
                    continue;
                }
                LinkObject(pChild, pParent);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, [[maybe_unused]] int flags)
{
    view->SetCursorString("");

    m_hCurrCursor = &m_hLinkCursor;
    if (event == eMouseLDown)
    {
        HitContext hitInfo;
        view->HitTest(point, hitInfo);
        CBaseObject* obj = hitInfo.object;
        if (obj)
        {
            if (IsRelevant(obj))
            {
                m_StartDrag = obj->GetWorldPos();
                m_pChild = obj;
            }
        }
    }
    else if (event == eMouseLUp)
    {
        HitContext hitInfo;
        view->HitTest(point, hitInfo);
        CBaseObject* obj = hitInfo.object;
        if (obj)
        {
            if (IsRelevant(obj))
            {
                CSelectionGroup* pSelectionGroup = GetIEditor()->GetSelection();
                int nGroupCount = pSelectionGroup->GetCount();
                if (pSelectionGroup && nGroupCount > 1)
                {
                    LinkSelectedToParent(obj);
                }
                if (!pSelectionGroup || nGroupCount <= 1  || !pSelectionGroup->IsContainObject(m_pChild))
                {
                    LinkObject(m_pChild, obj);
                }
            }
        }
        m_pChild = NULL;
    }
    else if (event == eMouseMove)
    {
        m_EndDrag = view->ViewToWorld(point);
        m_nodeName = nullptr;
        m_pGeomCacheRenderNode = nullptr;

        HitContext hitInfo;
        if (view->HitTest(point, hitInfo))
        {
            m_EndDrag = hitInfo.raySrc + hitInfo.rayDir * hitInfo.dist;
        }

        CBaseObject* obj = hitInfo.object;
        if (obj)
        {
            if (IsRelevant(obj))
            {
                QString name = obj->GetName();
                if (hitInfo.name)
                {
                    name += QString("\n  ") + hitInfo.name;
                }

                // Set Cursors.
                view->SetCursorString(name);
                if (m_pChild)
                {
                    if (ChildIsValid(obj, m_pChild))
                    {
                        m_hCurrCursor = &m_hLinkNowCursor;
                    }
                }
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::OnKeyDown([[maybe_unused]] CViewport* view, uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        // Cancel selection.
        GetIEditor()->SetEditTool(nullptr);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CLinkTool::Display(DisplayContext& dc)
{
    if (m_pChild && m_EndDrag != Vec3(ZERO))
    {
        ColorF lineColor = (m_hCurrCursor == &m_hLinkNowCursor) ? ColorF(0, 1, 0) : ColorF(1, 0, 0);
        dc.DrawLine(m_StartDrag, m_EndDrag, lineColor, lineColor);
    }
}

//////////////////////////////////////////////////////////////////////////
void CLinkTool::OnEntityDestruction(const AZ::EntityId& entityId)
{
    if (m_pChild && (m_pChild->GetType() == OBJTYPE_AZENTITY))
    {
        CComponentEntityObject* childComponentEntity = static_cast<CComponentEntityObject*>(m_pChild);
        AZ::EntityId childEntityId = childComponentEntity->GetAssociatedEntityId();
        if(entityId == childEntityId)
        {
            GetIEditor()->SetEditTool(nullptr);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::ChildIsValid(CBaseObject* pParent, CBaseObject* pChild, int nDir)
{
    if (!pParent)
    {
        return false;
    }
    if (!pChild)
    {
        return false;
    }
    if (pParent == pChild)
    {
        return false;
    }

    // Legacy entities and AZ entities shouldn't be linked.
    if ((pParent->GetType() == OBJTYPE_AZENTITY) != (pChild->GetType() == OBJTYPE_AZENTITY))
    {
        return false;
    }

    CBaseObject* pObj;
    if (nDir & 1)
    {
        if (pObj = pChild->GetParent())
        {
            if (!ChildIsValid(pParent, pObj, 1))
            {
                return false;
            }
        }
    }
    if (nDir & 2)
    {
        for (int i = 0; i < pChild->GetChildCount(); i++)
        {
            if (pObj = pChild->GetChild(i))
            {
                if (!ChildIsValid(pParent, pObj, 2))
                {
                    return false;
                }
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CLinkTool::OnSetCursor(CViewport* vp)
{
    vp->SetCursor(*m_hCurrCursor);
    return true;
}

#include <moc_LinkTool.cpp>
