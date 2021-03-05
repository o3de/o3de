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

#include "VoxelAligningTool.h"

// Editor
#include "SurfaceInfoPicker.h"
#include "Objects/SelectionGroup.h"

//////////////////////////////////////////////////////////////////////////
CVoxelAligningTool::CVoxelAligningTool()
{
    m_curObj = 0;
    m_PreviewMode = ePM_Idle;

    CSelectionGroup* sel = GetIEditor()->GetSelection();
    if (!sel->IsEmpty())
    {
        m_curObj = sel->GetObject(0);
        m_CurObjTMBeforePreviewMode = m_curObj->GetWorldTM();
        m_q = m_curObj->GetRotation();
    }
}

//////////////////////////////////////////////////////////////////////////
CVoxelAligningTool::~CVoxelAligningTool()
{
}

//////////////////////////////////////////////////////////////////////////
void CVoxelAligningTool::Display([[maybe_unused]] DisplayContext& dc)
{
}

//////////////////////////////////////////////////////////////////////////
bool CVoxelAligningTool::MouseCallback([[maybe_unused]] CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    // Get contrl key status.
    bool bCtrlClick = (flags & MK_CONTROL);
    bool bShiftClick = (flags & MK_SHIFT);
    bool bOnlyCtrlClick = bCtrlClick && !bShiftClick;

    CSelectionGroup* sel = GetIEditor()->GetSelection();
    if (sel->IsEmpty() || m_curObj != sel->GetObject(0))
    {
        GetIEditor()->SetEditTool(0);
        return true;
    }

    if (event == eMouseMove)
    {
        if (m_PreviewMode == ePM_Idle)
        {
            if (bOnlyCtrlClick)
            {
                if (m_curObj)
                {
                    m_CurObjTMBeforePreviewMode = m_curObj->GetWorldTM();
                }
                m_PreviewMode = ePM_Previewing;
                GetIEditor()->BeginUndo();
            }
        }
        else if (!bOnlyCtrlClick)
        {
            if (m_curObj)
            {
                m_curObj->SetWorldTM(m_CurObjTMBeforePreviewMode);
                //m_curObj->SetRotation(m_extraRot);
            }
            m_PreviewMode = ePM_Idle;
            GetIEditor()->CancelUndo();
        }

        if (m_PreviewMode == ePM_Previewing && bOnlyCtrlClick)
        {   // Preview align to normal
            ApplyPickedTM2CurObj(point);
        }
    }

    if (event == eMouseLDown && m_PreviewMode == ePM_Previewing)
    {
        m_CurObjTMBeforePreviewMode = m_curObj->GetWorldTM();
        GetIEditor()->AcceptUndo("Surface Normal Aligning");
        GetIEditor()->SetEditTool(NULL);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CVoxelAligningTool::ApplyPickedTM2CurObj(const QPoint& point, [[maybe_unused]] bool bPickOnlyTerrain)
{
    int nPickFlag = CSurfaceInfoPicker::ePOG_All;
    SRayHitInfo hitInfo;
    CSurfaceInfoPicker::CExcludedObjects excludeObjects;
    if (m_curObj)
    {
        excludeObjects.Add(m_curObj);
    }
    CSurfaceInfoPicker surfacePicker;
    if (surfacePicker.Pick(point, hitInfo, &excludeObjects, nPickFlag))
    {
        m_curObj->SetPos(hitInfo.vHitPos, eObjectUpdateFlags_UserInput);
        ApplyRotation(hitInfo.vHitNormal);
    }
}

//////////////////////////////////////////////////////////////////////////
void CVoxelAligningTool::ApplyRotation(Vec3& normal)
{
    Vec3 zaxis = m_q * Vec3(0, 0, 1);
    zaxis.Normalize();
    Quat nq;
    nq.SetRotationV0V1(zaxis, normal);
    m_curObj->SetRotation(nq * m_q, eObjectUpdateFlags_UserInput);
}

//////////////////////////////////////////////////////////////////////////
void CVoxelAligningTool::BeginEditParams([[maybe_unused]] IEditor* ie, [[maybe_unused]] int flags)
{
}

//////////////////////////////////////////////////////////////////////////
void CVoxelAligningTool::EndEditParams()
{
}

//////////////////////////////////////////////////////////////////////////
bool CVoxelAligningTool::OnKeyDown([[maybe_unused]] CViewport* view, uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        GetIEditor()->SetEditTool(0);
    }
    return false;
}

#include <moc_VoxelAligningTool.cpp>
