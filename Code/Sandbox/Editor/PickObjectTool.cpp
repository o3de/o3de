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

#include "PickObjectTool.h"

// Editor
#include "Viewport.h"
#include "Include/HitContext.h"
#include "Objects/BaseObject.h"


//////////////////////////////////////////////////////////////////////////
CPickObjectTool::CPickObjectTool(IPickObjectCallback* callback, const QMetaObject* targetClass)
{
    assert(callback != 0);
    m_callback = callback;
    m_targetClass = targetClass;
    m_bMultiPick = false;
}

//////////////////////////////////////////////////////////////////////////
CPickObjectTool::~CPickObjectTool()
{
    GetIEditor()->GetObjectManager()->SetSelectCallback(0);
    //m_prevSelectCallback = 0;
    if (m_callback)
    {
        m_callback->OnCancelPick();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPickObjectTool::BeginEditParams([[maybe_unused]] IEditor* ie, [[maybe_unused]] int flags)
{
    QString str = "Pick object";
    if (m_targetClass)
    {
        str = tr("Pick %1 object").arg(m_targetClass->className());
    }
    SetStatusText(str);

    //m_prevSelectCallback =
    GetIEditor()->GetObjectManager()->SetSelectCallback(this);
}

//////////////////////////////////////////////////////////////////////////
bool CPickObjectTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, [[maybe_unused]] int flags)
{
    if (event == eMouseLDown)
    {
        HitContext hitInfo;
        view->HitTest(point, hitInfo);
        CBaseObject* obj = hitInfo.object;
        if (obj)
        {
            if (IsRelevant(obj))
            {
                if (m_callback)
                {
                    // Can pick this one.
                    m_callback->OnPick(obj);
                }
                if (!m_bMultiPick)
                {
                    m_callback = 0;
                    GetIEditor()->SetEditTool(0);
                }
            }
        }
    }
    else if (event == eMouseMove)
    {
        HitContext hitInfo;
        view->HitTest(point, hitInfo);
        CBaseObject* obj = hitInfo.object;
        if (obj)
        {
            if (IsRelevant(obj))
            {
                // Set Cursors.
                view->SetCurrentCursor(STD_CURSOR_HIT, obj->GetName());
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
bool CPickObjectTool::OnSelectObject(CBaseObject* obj)
{
    if (IsRelevant(obj))
    {
        // Can pick this one.
        if (m_callback)
        {
            m_callback->OnPick(obj);
            m_callback = 0;
        }
        if (!m_bMultiPick)
        {
            GetIEditor()->SetEditTool(0);
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPickObjectTool::CanSelectObject(CBaseObject* obj)
{
    return IsRelevant(obj);
}

//////////////////////////////////////////////////////////////////////////
bool CPickObjectTool::OnKeyDown([[maybe_unused]] CViewport* view, uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        // Cancel selection.
        GetIEditor()->SetEditTool(0);
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPickObjectTool::IsRelevant(CBaseObject* obj)
{
    assert(obj != 0);
    if (obj == NULL)
    {
        return false;
    }
    if (!m_callback)
    {
        return false;
    }

    if (!m_targetClass)
    {
        return m_callback->OnPickFilter(obj);
    }
    else
    {
        if (obj->metaObject() == m_targetClass || m_targetClass->cast(obj))
        {
            return m_callback->OnPickFilter(obj);
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CPickObjectTool::IsNeedSpecificBehaviorForSpaceAcce()
{
    if (m_callback && m_callback->IsNeedSpecificBehaviorForSpaceAcce())
    {
        return true;
    }
    return false;
}

#include <moc_PickObjectTool.cpp>
