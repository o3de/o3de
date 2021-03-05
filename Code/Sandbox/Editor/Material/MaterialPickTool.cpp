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

#include "MaterialPickTool.h"

// Editor
#include "MaterialManager.h"
#include "SurfaceInfoPicker.h"
#include "Viewport.h"


#define RENDER_MESH_TEST_DISTANCE 0.2f

static IClassDesc * s_ToolClass = NULL;

//////////////////////////////////////////////////////////////////////////
CMaterialPickTool::CMaterialPickTool()
{
    m_pClassDesc = s_ToolClass;
    m_statusText = tr("Left Click To Pick Material");
}

//////////////////////////////////////////////////////////////////////////
CMaterialPickTool::~CMaterialPickTool()
{
    SetMaterial(0);
}

//////////////////////////////////////////////////////////////////////////
bool CMaterialPickTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    if (event == eMouseLDown)
    {
        if (m_pMaterial)
        {
            CMaterial* pMtl = GetIEditor()->GetMaterialManager()->FromIMaterial(m_pMaterial);
            if (pMtl)
            {
                GetIEditor()->GetMaterialManager()->SetHighlightedMaterial(0);
                GetIEditor()->OpenMaterialLibrary(pMtl);
                Abort();
                return true;
            }
        }
    }
    else if (event == eMouseMove)
    {
        return OnMouseMove(view, flags, point);
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialPickTool::Display(DisplayContext& dc)
{
    QPoint mousePoint = QCursor::pos();

    dc.view->ScreenToClient(mousePoint);

    Vec3 wp = dc.view->ViewToWorld(mousePoint);

    if (m_pMaterial)
    {
        float color[4] = {1, 1, 1, 1};
        dc.renderer->Draw2dLabel(mousePoint.x() + 12, mousePoint.y ()+ 8, 1.2f, color, false, "%s", m_displayString.toUtf8().data());
    }

    float fScreenScale = dc.view->GetScreenScaleFactor(m_HitInfo.vHitPos) * 0.06f;

    dc.DepthTestOff();
    dc.SetColor(ColorB(0, 0, 255, 255));
    if (!m_HitInfo.vHitNormal.IsZero())
    {
        dc.DrawLine(m_HitInfo.vHitPos, m_HitInfo.vHitPos + m_HitInfo.vHitNormal * fScreenScale);

        Vec3 raySrc, rayDir;
        dc.view->ViewToWorldRay(mousePoint, raySrc, rayDir);

        Matrix34 tm;

        Vec3 zAxis = m_HitInfo.vHitNormal;
        Vec3 xAxis = rayDir.Cross(zAxis);
        if (!xAxis.IsZero())
        {
            xAxis.Normalize();
            Vec3 yAxis = xAxis.Cross(zAxis).GetNormalized();
            tm.SetFromVectors(xAxis, yAxis, zAxis, m_HitInfo.vHitPos);

            dc.PushMatrix(tm);
            dc.DrawCircle(Vec3(0, 0, 0), 0.5f * fScreenScale);
            dc.PopMatrix();
        }
    }
    dc.DepthTestOn();
}

//////////////////////////////////////////////////////////////////////////
bool CMaterialPickTool::OnMouseMove(CViewport* view, [[maybe_unused]] UINT nFlags, const QPoint& point)
{
    view->SetCurrentCursor(STD_CURSOR_HIT, "");

    _smart_ptr<IMaterial> pNearestMaterial(NULL);

    m_Mouse2DPosition = point;

    CSurfaceInfoPicker surfacePicker;
    int nPickObjectGroupFlag = CSurfaceInfoPicker::ePOG_All;
    if (surfacePicker.Pick(point, pNearestMaterial, m_HitInfo, NULL, nPickObjectGroupFlag))
    {
        SetMaterial(pNearestMaterial);
        return true;
    }

    SetMaterial(0);
    return false;
}

const GUID& CMaterialPickTool::GetClassID()
{
    // {FD20F6F2-7B87-4349-A5D4-7533538E357F}
    static const GUID guid = {
        0xfd20f6f2, 0x7b87, 0x4349, { 0xa5, 0xd4, 0x75, 0x33, 0x53, 0x8e, 0x35, 0x7f }
    };
    return guid;
}

//////////////////////////////////////////////////////////////////////////
void CMaterialPickTool::RegisterTool(CRegistrationContext& rc)
{
    rc.pClassFactory->RegisterClass(s_ToolClass = new CQtViewClass<CMaterialPickTool>("EditTool.PickMaterial", "Material", ESYSTEM_CLASS_EDITTOOL));
}

//////////////////////////////////////////////////////////////////////////
void CMaterialPickTool::SetMaterial(_smart_ptr<IMaterial> pMaterial)
{
    if (pMaterial == m_pMaterial)
    {
        return;
    }

    m_pMaterial = pMaterial;
    CMaterial* pCMaterial = GetIEditor()->GetMaterialManager()->FromIMaterial(m_pMaterial);
    GetIEditor()->GetMaterialManager()->SetHighlightedMaterial(pCMaterial);

    m_displayString = "";
    if (pMaterial)
    {
        QString sfType;
        sfType = QStringLiteral("%1 : %2").arg(pMaterial->GetSurfaceType()->GetId()).arg(pMaterial->GetSurfaceType()->GetName());

        m_displayString = "\n";
        m_displayString += pMaterial->GetName();
        m_displayString += "\n";
        m_displayString += sfType;
    }
}

#include <Material/moc_MaterialPickTool.cpp>
