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

#include "AxisHelperExtended.h"

#include "CryPhysicsDeprecation.h"

// Editor
#include "Include/IDisplayViewport.h"
#include "SurfaceInfoPicker.h"
#include "Objects/DisplayContext.h"
#include "Objects/SelectionGroup.h"
#include "Util/fastlib.h"


//////////////////////////////////////////////////////////////////////////
// Helper Extended Axis object.
//////////////////////////////////////////////////////////////////////////
CAxisHelperExtended::CAxisHelperExtended()
    : m_matrix(IDENTITY)
    , m_vPos(ZERO)
    , m_pCurObject(0)
    , m_dwLastUpdateTime(0)
    , m_fMaxDist(100.0f)
{
}

//////////////////////////////////////////////////////////////////////////
void CAxisHelperExtended::DrawAxes(DisplayContext& dc, const Matrix34& matrix, bool bUsePhysicalProxy)
{
    const DWORD dwUpdateTime = 2000; // 2 sec
    CSelectionGroup* pSel = GetIEditor()->GetSelection();
    int numSels = pSel->GetCount();

    if (numSels == 0)
    {
        return;
    }

    CBaseObject* pCurObject = pSel->GetObject(numSels - 1); // get just last object for simple check

    // Add current selection to the elements to be skipped
    CRY_PHYSICS_REPLACEMENT_ASSERT();

    Vec3 x = Vec3(1, 0, 0);
    Vec3 y = Vec3(0, 1, 0);
    Vec3 z = Vec3(0, 0, 1);

    Vec3 colR = Vec3(1, 0, 0);
    Vec3 colG = Vec3(0, 1, 0);
    Vec3 colB = Vec3(0, 0.8f, 1);

    m_vPos = matrix.GetTranslation();

    Vec3 vDirX = (matrix * x - m_vPos);
    Vec3 vDirY = (matrix * y - m_vPos);
    Vec3 vDirZ = (matrix * z - m_vPos);
    vDirX.Normalize();
    vDirY.Normalize();
    vDirZ.Normalize();

    if (
        m_pCurObject != pCurObject ||
        GetTickCount() - m_dwLastUpdateTime > dwUpdateTime ||
        !Matrix34::IsEquivalent(m_matrix, matrix)
        )
    {
        Vec3 outTmp;
        AABB aabb(m_vPos, m_fMaxDist);
        m_objects.clear();
        CBaseObjectsArray allObjects;
        GetIEditor()->GetObjectManager()->GetObjects(allObjects);

        for (size_t i = 0, n = allObjects.size(); i < n; ++i)
        {
            CBaseObject* pObject = allObjects[i];
            if (pObject->IsSelected() || !pObject->GetEngineNode())
            {
                continue;
            }

            AABB aabbObj;
            pObject->GetBoundBox(aabbObj);

            if (!Intersect::Ray_AABB(m_vPos, vDirX, aabbObj, outTmp)
                && !Intersect::Ray_AABB(m_vPos, vDirY, aabbObj, outTmp)
                && !Intersect::Ray_AABB(m_vPos, vDirZ, aabbObj, outTmp)
                && !Intersect::Ray_AABB(m_vPos, -vDirX, aabbObj, outTmp)
                && !Intersect::Ray_AABB(m_vPos, -vDirY, aabbObj, outTmp)
                && !Intersect::Ray_AABB(m_vPos, -vDirZ, aabbObj, outTmp))
            {
                continue;
            }

            if (aabb.IsIntersectBox(aabbObj))
            {
                m_objects.push_back(pObject);
            }
        }
        m_dwLastUpdateTime = GetTickCount();
    }
    m_pCurObject = pCurObject;
    m_matrix = matrix;

    DrawAxis(dc, vDirX, z, colR, bUsePhysicalProxy);
    DrawAxis(dc, vDirY, z, colG, bUsePhysicalProxy);
    DrawAxis(dc, vDirZ, x, colB, bUsePhysicalProxy);
    DrawAxis(dc, -vDirX, z, colR, bUsePhysicalProxy);
    DrawAxis(dc, -vDirY, z, colG, bUsePhysicalProxy);
    DrawAxis(dc, -vDirZ, x, colB, bUsePhysicalProxy);
}


//////////////////////////////////////////////////////////////////////////
void CAxisHelperExtended::DrawAxis(DisplayContext& dc, const Vec3& vDir, const Vec3& vUpAxis, const Vec3& col, bool bUsePhysicalProxy)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    const float fBallSize = 0.005f;
    const float fTextSize = 1.4f;

    float fDist = m_fMaxDist + 1.0f;
    size_t objectsSize = m_objects.size();
    for (size_t i = 0; i < objectsSize; ++i)
    {
        CBaseObject* pObject = m_objects[i];
        if (pObject->CheckFlags(OBJFLAG_DELETED))
        {
            continue;
        }

        SRayHitInfo hitInfo;
        if (pObject->IntersectRayMesh(m_vPos, vDir, hitInfo))
        {
            Vec3 newP = pObject->GetWorldTM().TransformPoint(hitInfo.vHitPos);
            float fNewDist = (newP - m_vPos).GetLength();
            if (fDist > fNewDist)
            {
                fDist = fNewDist;
            }
        }
    }

    if (bUsePhysicalProxy)
    {
        CRY_PHYSICS_REPLACEMENT_ASSERT();
    }
    else
    {
        CSurfaceInfoPicker picker;
        m_objectsForPicker.assign(m_objects.begin(), m_objects.end());
        picker.SetObjects(&m_objectsForPicker);
        SRayHitInfo hitInfo;
        if (picker.Pick(m_vPos, fDist * vDir, hitInfo, NULL, CSurfaceInfoPicker::ePOG_All))
        {
            fDist = hitInfo.fDistance;
        }
        picker.SetObjects(0);
    }

    if (fDist < m_fMaxDist)
    {
        Vec3 p = m_vPos + vDir * fDist;

        dc.SetColor(col);
        dc.DrawLine(m_vPos, p);
        float fScreenScale = dc.view->GetScreenScaleFactor(p);
        dc.DrawBall(p, fBallSize * fScreenScale);
        QString label;
        label = QString::number(fDist, 'f', 2);
        dc.DrawTextOn2DBox((p + m_vPos) * 0.5f, label.toUtf8().data(), fTextSize, col, ColorF(0.0f, 0.0f, 0.0f, 0.7f));

        Vec3 vUp = (m_matrix * vUpAxis - m_vPos);
        vUp.Normalize();

        Vec3 u = vUp;
        Vec3 v = vDir ^ vUp;
        float fPlaneSize = fDist / 4.0f;
        float alphaMax = 1.0f, alphaMin = 0.0f;
        ColorF colAlphaMin = ColorF(col.x, col.y, col.z, alphaMin);

        float fStepSize = dc.view->GetGridStep();
        const int MIN_STEP_COUNT = 5;
        const int MAX_STEP_COUNT = 20;
        int nSteps = std::min(std::max(FloatToIntRet(fPlaneSize / fStepSize), MIN_STEP_COUNT), MAX_STEP_COUNT);
        float fGridSize = nSteps * fStepSize;

        for (int i = -nSteps; i <= nSteps; ++i)
        {
            Vec3 stepV = v * (fStepSize * i);
            Vec3 stepU = u * (fStepSize * i);
            ColorF colCurAlpha = ColorF(col.x, col.y, col.z, alphaMax - fabsf(float(i) / float(nSteps)) * (alphaMax - alphaMin));
            // Draw u lines.
            dc.DrawLine(p + stepV, p + u * fGridSize + stepV, colCurAlpha, colAlphaMin);
            dc.DrawLine(p + stepV, p - u * fGridSize + stepV, colCurAlpha, colAlphaMin);
            // Draw v lines.
            dc.DrawLine(p + stepU, p + v * fGridSize + stepU, colCurAlpha, colAlphaMin);
            dc.DrawLine(p + stepU, p - v * fGridSize + stepU, colCurAlpha, colAlphaMin);
        }
    }
}
