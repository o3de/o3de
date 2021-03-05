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

#include "ObjectCloneTool.h"

// Editor
#include "MainWindow.h"
#include "Viewport.h"
#include "ViewManager.h"
#include "Include/IObjectManager.h"
#include "Objects/SelectionGroup.h"
#include "Settings.h"

//////////////////////////////////////////////////////////////////////////
// Class description.
//////////////////////////////////////////////////////////////////////////
class CObjectCloneTool_ClassDesc
    : public CRefCountClassDesc
{
    virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_EDITTOOL; }
    virtual REFGUID ClassID()
    {
        // {6A73E865-71DF-4ED0-ABA2-457E66119B35}
        static const GUID guid = {
            0x6a73e865, 0x71df, 0x4ed0,{ 0xab, 0xa2, 0x45, 0x7e, 0x66, 0x11, 0x9b, 0x35 }
        };
        return guid;
    }
    virtual QString ClassName() { return "EditTool.Clone"; };
    virtual QString Category() { return "EditTool"; };
};
CObjectCloneTool_ClassDesc g_cloneClassDesc;

//////////////////////////////////////////////////////////////////////////
CObjectCloneTool::CObjectCloneTool()
    : m_currentUndoBatch(nullptr)
{
    m_pClassDesc = &g_cloneClassDesc;
    m_bSetConstrPlane = true;

    GetIEditor()->SuperBeginUndo();

    GetIEditor()->BeginUndo();
    m_selection = nullptr;
    if (!GetIEditor()->GetSelection()->IsEmpty())
    {
        QWaitCursor wait;
        CloneSelection();
        m_selection = GetIEditor()->GetSelection();
        m_origin = m_selection->GetCenter();
    }
    GetIEditor()->AcceptUndo("Clone");
    GetIEditor()->BeginUndo();

    if (!gSettings.deepSelectionSettings.bStickDuplicate)
    {
        SetStatusText("Clone object at the same location");
    }
    else
    {
        SetStatusText("Left click to clone object");
    }
}

//////////////////////////////////////////////////////////////////////////
CObjectCloneTool::~CObjectCloneTool()
{
    EndUndoBatch();

    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->SuperCancelUndo();
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectCloneTool::CloneSelection()
{
    // Allow component application to intercept cloning behavior.
    // This is to allow support for "smart" cloning of prefabs, and other contextual features.
    AZ_Assert(!m_currentUndoBatch, "CloneSelection undo batch already created.");
    EBUS_EVENT_RESULT(m_currentUndoBatch, AzToolsFramework::ToolsApplicationRequests::Bus, BeginUndoBatch, "Clone Selection");
    bool handled = false;
    EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, CloneSelection, handled);
    if (handled)
    {
        GetIEditor()->GetObjectManager()->CheckAndFixSelection();
        return;
    }

    // This is the legacy case. We're not cloning AZ entities, so abandon the AZ undo batch.
    EndUndoBatch();

    CSelectionGroup selObjects;
    CSelectionGroup sel;

    CSelectionGroup* currSelection = GetIEditor()->GetSelection();

    currSelection->Clone(selObjects);

    GetIEditor()->ClearSelection();
    for (int i = 0; i < selObjects.GetCount(); i++)
    {
        if (selObjects.GetObject(i))
        {
            GetIEditor()->SelectObject(selObjects.GetObject(i));
        }
    }
    MainWindow::instance()->setFocus();
}

//////////////////////////////////////////////////////////////////////////
void CObjectCloneTool::SetConstrPlane(CViewport* view, [[maybe_unused]] const QPoint& point)
{
    Matrix34 originTM;
    originTM.SetIdentity();
    CSelectionGroup* selection = GetIEditor()->GetSelection();
    if (selection->GetCount() == 1)
    {
        originTM = selection->GetObject(0)->GetWorldTM();
    }
    else if (selection->GetCount() > 1)
    {
        originTM = selection->GetObject(0)->GetWorldTM();
        Vec3 center = view->SnapToGrid(originTM.GetTranslation());
        originTM.SetTranslation(center);
    }
    view->SetConstructionMatrix(COORDS_LOCAL, originTM);
}

//static Vec3 gP1,gP2;
//////////////////////////////////////////////////////////////////////////
void CObjectCloneTool::Display([[maybe_unused]] DisplayContext& dc)
{
    //dc.SetColor( 1,1,0,1 );
    //dc.DrawBall( gP1,1.1f );
    //dc.DrawBall( gP2,1.1f );
}

//////////////////////////////////////////////////////////////////////////
bool CObjectCloneTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (m_selection)
    {
        // Set construction plane origin to selection origin.
        if (m_bSetConstrPlane)
        {
            SetConstrPlane(view, point);
            m_bSetConstrPlane = false;
        }

        if (event == eMouseLDown)
        {
            // Accept group.
            Accept();
            GetIEditor()->GetSelection()->FinishChanges();
            return true;
        }
        if (event == eMouseMove)
        {
            // Move selection.
            CSelectionGroup* selection = GetIEditor()->GetSelection();
            if (selection != m_selection)
            {
                Abort();
            }
            else if (!selection->IsEmpty())
            {
                GetIEditor()->RestoreUndo();

                Vec3 v;
                bool followTerrain = false;

                CSelectionGroup* pSelection = GetIEditor()->GetSelection();
                Vec3 selectionCenter = view->SnapToGrid(pSelection->GetCenter());

                int axis = GetIEditor()->GetAxisConstrains();
                if (axis == AXIS_TERRAIN)
                {
                    bool hitTerrain;
                    v = view->ViewToWorld(point, &hitTerrain) - selectionCenter;
                    if (axis == AXIS_TERRAIN)
                    {
                        v = view->SnapToGrid(v);
                        if (hitTerrain)
                        {
                            followTerrain = true;
                            v.z = 0;
                        }
                    }
                }
                else
                {
                    Vec3 p1 = selectionCenter;
                    Vec3 p2 = view->MapViewToCP(point);
                    if (p2.IsZero())
                    {
                        return true;
                    }

                    v = view->GetCPVector(p1, p2);
                    // Snap v offset to grid if its enabled.
                    view->SnapToGrid(v);
                }

                CSelectionGroup::EMoveSelectionFlag selectionFlag = CSelectionGroup::eMS_None;
                if (followTerrain)
                {
                    selectionFlag = CSelectionGroup::eMS_FollowTerrain;
                }

                // Disable undo recording for these move commands as the only operation we need
                // to undo is the creation of the new object. Undo commands are queued so it's
                // possible that the object creation could be undone before attempting to undo
                // these move operations causing undesired behavior.
                bool wasRecording = CUndo::IsRecording();
                if (wasRecording)
                {
                    GetIEditor()->SuspendUndo();
                }

                GetIEditor()->GetSelection()->Move(v, selectionFlag, GetIEditor()->GetReferenceCoordSys(), point);
                
                if (wasRecording)
                {
                    GetIEditor()->ResumeUndo();
                }
            }
        }
        if (event == eMouseWheel)
        {
            CSelectionGroup* selection = GetIEditor()->GetSelection();
            if (selection != m_selection)
            {
                Abort();
            }
            else if (!selection->IsEmpty())
            {
                double angle = 1;

                if (view->GetViewManager()->GetGrid()->IsAngleSnapEnabled())
                {
                    angle = view->GetViewManager()->GetGrid()->GetAngleSnap();
                }

                for (int i = 0; i < selection->GetCount(); ++i)
                {
                    CBaseObject* pObj = selection->GetFilteredObject(i);
                    Quat rot = pObj->GetRotation();
                    rot.SetRotationXYZ(Ang3(0, 0, rot.GetRotZ() + DEG2RAD(flags > 0 ? angle * (-1) : angle)));
                    pObj->SetRotation(rot);
                }
                GetIEditor()->AcceptUndo("Rotate Selection");
            }
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CObjectCloneTool::Abort()
{
    EndUndoBatch();

    // Abort
    GetIEditor()->SetEditTool(0);
}

//////////////////////////////////////////////////////////////////////////
void CObjectCloneTool::Accept(bool resetPosition)
{
    // Close the az undo batch so it can add the appropriate objects to the cry undo stack
    EndUndoBatch();

    if (resetPosition)
    {
        GetIEditor()->GetSelection()->MoveTo(m_origin, CSelectionGroup::eMS_None, GetIEditor()->GetReferenceCoordSys());
    }

    if (GetIEditor()->IsUndoRecording())
    {
        GetIEditor()->SuperAcceptUndo("Clone");
    }

    GetIEditor()->SetEditTool(0);
}

//////////////////////////////////////////////////////////////////////////
void CObjectCloneTool::EndUndoBatch()
{
    if (m_currentUndoBatch)
    {
        AzToolsFramework::UndoSystem::URSequencePoint* undoBatch = nullptr;
        EBUS_EVENT_RESULT(undoBatch, AzToolsFramework::ToolsApplicationRequests::Bus, GetCurrentUndoBatch);
        AZ_Error("ObjectCloneTool", undoBatch == m_currentUndoBatch, "Undo batch is not in sync.");
        if (undoBatch == m_currentUndoBatch)
        {
            EBUS_EVENT(AzToolsFramework::ToolsApplicationRequests::Bus, EndUndoBatch);
        }
        m_currentUndoBatch = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////
void CObjectCloneTool::BeginEditParams([[maybe_unused]] IEditor* ie, [[maybe_unused]] int flags)
{
}

//////////////////////////////////////////////////////////////////////////
void CObjectCloneTool::EndEditParams()
{
}

//////////////////////////////////////////////////////////////////////////
bool CObjectCloneTool::OnKeyDown([[maybe_unused]] CViewport* view, uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        Abort();
    }
    return false;
}

#include <moc_ObjectCloneTool.cpp>
