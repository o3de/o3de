/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "AxisHelper.h"

#include "Objects/DisplayContext.h"
#include "Include/IDisplayViewport.h"
#include "Include/HitContext.h"
#include "Util/Math.h"
#include "IObjectManager.h"
#include <Cry_Geo.h>

const float kPlaneScale = 0.3f;
const float kBoldLine3D = 4.f;
const float kBoldLine2D = 2.f;
const float kSelectionBallScale = 0.05f;
const float kRotateCircleRadiusScale = 0.2f;

//////////////////////////////////////////////////////////////////////////
CAxisHelper::CAxisHelper()
    : m_matrix(IDENTITY)
{
    m_nModeFlags = MOVE_MODE;
    m_currentMode = MOVE_MODE;
    m_highlightMode = 1;
    m_highlightAxis = 0;
    m_bHighlightRotationCircle = false;

    m_bNeedX = true;
    m_bNeedY = true;
    m_bNeedZ = true;
}

//////////////////////////////////////////////////////////////////////////
void CAxisHelper::SetMode(int nModeFlags)
{
    m_nModeFlags = nModeFlags;
}

//////////////////////////////////////////////////////////////////////////
void CAxisHelper::Prepare(const Matrix34& worldTM, const SGizmoParameters& setup, IDisplayViewport* view)
{
    m_fScreenScale = view->GetScreenScaleFactor(worldTM.GetTranslation());
    m_size = setup.axisGizmoScale * m_fScreenScale;

    m_bNeedX = true;
    m_bNeedY = true;
    m_bNeedZ = true;

    IDisplayViewport::EAxis axis = IDisplayViewport::AXIS_NONE;
    bool b2D = false;

    view->GetPerpendicularAxis(&axis, &b2D);
    switch (axis)
    {
    case static_cast<IDisplayViewport::EAxis>(AXIS_X):
        m_bNeedX = false;
        break;
    case static_cast<IDisplayViewport::EAxis>(AXIS_Y):
        m_bNeedY = false;
        break;
    case static_cast<IDisplayViewport::EAxis>(AXIS_Z):
        m_bNeedZ = false;
        break;
    }

    m_matrix = worldTM;
    RefCoordSys refCoordSys = setup.referenceCoordSys;
    if (b2D && refCoordSys == COORDS_VIEW)
    {
        m_matrix = view->GetViewTM();
        m_matrix.SetTranslation(worldTM.GetTranslation());
    }
    m_matrix.OrthonormalizeFast();
}

//////////////////////////////////////////////////////////////////////////
void CAxisHelper::DrawDome(const Matrix34& worldTM, const SGizmoParameters& setup, DisplayContext& dc, AABB& objectBox)
{
    Prepare(worldTM, setup, dc.view);
    Vec3 x(m_size, 0, 0);
    Vec3 y(0, m_size, 0);
    Vec3 z(0, 0, m_size);


    int prevRState = dc.GetState();

    if (!(dc.flags & DISPLAY_2D))
    {
        dc.DepthTestOff();
    }

    dc.PushMatrix(m_matrix);
    dc.SetDrawInFrontMode(true);

    float radius = objectBox.GetRadius();
    dc.DrawCircle(Vec3(0, 0, 0), radius);
    dc.DrawCircle(Vec3(0, 0, 0), radius / 3.0f);

    dc.DrawArc(Vec3(0, 0, 0), radius, 0.0f, 180.0f, 1, Vec3(0, -1, 0));
    dc.DrawArc(Vec3(0, 0, 0), radius, 90.0f, 180.0f, 1, Vec3(1, 0, 0));


    dc.PopMatrix();
    if (!(dc.flags & DISPLAY_2D))
    {
        dc.DepthTestOn();
    }

    dc.SetState(prevRState);
}


//////////////////////////////////////////////////////////////////////////
void CAxisHelper::DrawAxis(const Matrix34& worldTM, const SGizmoParameters& setup, DisplayContext& dc)
{
    Prepare(worldTM, setup, dc.view);

    Vec3 x(m_size, 0, 0);
    Vec3 y(0, m_size, 0);
    Vec3 z(0, 0, m_size);

    int prevRState = dc.GetState();

    if (!(dc.flags & DISPLAY_2D))
    {
        dc.DepthTestOff();
    }

    dc.PushMatrix(m_matrix);
    dc.SetDrawInFrontMode(true);

    Vec3 colSelected(1, 1, 0);
    Vec3 axisColor(1, 1, 1);
    Vec3 disabledColor(0.75, 0.75, 0.75);

    Matrix34 worldTMWithoutScale(worldTM);
    worldTMWithoutScale.OrthonormalizeFast();
    float textSize = 1.4f;

    if (!(m_nModeFlags & ROTATE_CIRCLE_MODE))
    {
        dc.SetColor(setup.enabled ? axisColor : disabledColor);
        if (m_bNeedX && setup.axisGizmoText)
        {
            dc.DrawTextLabel(worldTMWithoutScale.TransformPoint(x), textSize, "x");
        }
        if (m_bNeedY && setup.axisGizmoText)
        {
            dc.DrawTextLabel(worldTMWithoutScale.TransformPoint(y), textSize, "y");
        }
        if (m_bNeedZ && setup.axisGizmoText)
        {
            dc.DrawTextLabel(worldTMWithoutScale.TransformPoint(z), textSize, "z");
        }
    }

    int axis = setup.axisConstraint;
    if (m_highlightAxis)
    {
        axis = m_highlightAxis;
    }

    float lineWidth = kBoldLine3D;
    if (dc.flags & DISPLAY_2D)
    {
        lineWidth = kBoldLine2D;
    }

    float linew[3];
    linew[0] = linew[1] = linew[2] = 0;
    Vec3 colX, colY, colZ;
    if (setup.enabled)
    {
        colX = Vec3(1, 0, 0);
        colY = Vec3(0, 1, 0);
        colZ = Vec3(0, 0, 1);
    }
    else
    {
        colX = colY = colZ = disabledColor;
    }
    Vec3 colXArrow = colX, colYArrow = colY, colZArrow = colZ;
    if (!(m_nModeFlags & ROTATE_CIRCLE_MODE))
    {
        if (axis)
        {
            if (axis == AXIS_X || axis == AXIS_XY || axis == AXIS_XZ || axis == AXIS_XYZ)
            {
                colX = colSelected;
                dc.SetColor(colSelected);
                if (m_bNeedX && setup.axisGizmoText)
                {
                    dc.DrawTextLabel(worldTMWithoutScale.TransformPoint(x), textSize, "x");
                }
                linew[0] = lineWidth;
            }
            if (axis == AXIS_Y || axis == AXIS_XY || axis == AXIS_YZ || axis == AXIS_XYZ)
            {
                colY = colSelected;
                dc.SetColor(colSelected);
                if (m_bNeedY && setup.axisGizmoText)
                {
                    dc.DrawTextLabel(worldTMWithoutScale.TransformPoint(y), textSize, "y");
                }
                linew[1] = lineWidth;
            }
            if (axis == AXIS_Z || axis == AXIS_XZ || axis == AXIS_YZ || axis == AXIS_XYZ)
            {
                colZ = colSelected;
                dc.SetColor(colSelected);
                if (m_bNeedZ && setup.axisGizmoText)
                {
                    dc.DrawTextLabel(worldTMWithoutScale.TransformPoint(z), textSize, "z");
                }
                linew[2] = lineWidth;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Calc sizes.
    //////////////////////////////////////////////////////////////////////////
    float headOfs[8];
    float headScl[8];
    headOfs[MOVE_MODE] = 0;
    headScl[MOVE_MODE] = 0.07f * m_fScreenScale;
    headOfs[ROTATE_MODE] = 0;
    headScl[ROTATE_MODE] = 0.01f * m_fScreenScale;
    headOfs[SCALE_MODE] = 0;
    headScl[SCALE_MODE] = 0.01f * m_fScreenScale;
    if (m_nModeFlags == (MOVE_MODE | ROTATE_MODE | SCALE_MODE))
    {
        headOfs[ROTATE_MODE] += 0.05f * m_fScreenScale;
        headOfs[SCALE_MODE] += 0.1f * m_fScreenScale;
    }
    //////////////////////////////////////////////////////////////////////////

    if (!(m_nModeFlags & ROTATE_CIRCLE_MODE))
    {
        if (m_bNeedX)
        {
            dc.SetColor(colX);
            dc.SetLineWidth(linew[0]);
            dc.DrawLine(Vec3(0, 0, 0), x + Vec3(headOfs[SCALE_MODE], 0, 0));
        }
        if (m_bNeedY)
        {
            dc.SetColor(colY);
            dc.SetLineWidth(linew[1]);
            dc.DrawLine(Vec3(0, 0, 0), y + Vec3(0, headOfs[SCALE_MODE], 0));
        }
        if (m_bNeedZ)
        {
            dc.SetColor(colZ);
            dc.SetLineWidth(linew[2]);
            dc.DrawLine(Vec3(0, 0, 0), z + Vec3(0, 0, headOfs[SCALE_MODE]));
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Draw Move Arrows.
    //////////////////////////////////////////////////////////////////////////
    if (m_nModeFlags & MOVE_MODE)
    {
        if (m_bNeedX)
        {
            if (m_highlightMode == 1)
            {
                dc.SetColor(colX);
            }
            else
            {
                dc.SetColor(colXArrow);
            }
            dc.DrawArrow(x - x * 0.1f, x, headScl[MOVE_MODE]);
        }
        if (m_bNeedY)
        {
            if (m_highlightMode == 1)
            {
                dc.SetColor(colY);
            }
            else
            {
                dc.SetColor(colYArrow);
            }
            dc.DrawArrow(y - y * 0.1f, y, headScl[MOVE_MODE]);
        }
        if (m_bNeedZ)
        {
            if (m_highlightMode == 1)
            {
                dc.SetColor(colZ);
            }
            else
            {
                dc.SetColor(colZArrow);
            }
            dc.DrawArrow(z - z * 0.1f, z, headScl[MOVE_MODE]);
        }
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Draw Scale Boxes.
    //////////////////////////////////////////////////////////////////////////
    if (m_nModeFlags & SCALE_MODE)
    {
        dc.SetColor(QColor(128, 128, 0));
        Vec3 boxsz = Vec3(headScl[SCALE_MODE], headScl[SCALE_MODE], headScl[SCALE_MODE]);
        dc.DrawSolidBox(-boxsz, boxsz);
        if (m_bNeedX)
        {
            if (m_highlightMode == 3)
            {
                dc.SetColor(colX);
            }
            else
            {
                dc.SetColor(colXArrow);
            }
            dc.DrawSolidBox(x + boxsz + Vec3(headOfs[SCALE_MODE], 0, 0), x - boxsz + Vec3(headOfs[SCALE_MODE], 0, 0));
        }
        if (m_bNeedY)
        {
            if (m_highlightMode == 3)
            {
                dc.SetColor(colY);
            }
            else
            {
                dc.SetColor(colYArrow);
            }
            dc.DrawSolidBox(y + boxsz + Vec3(0, headOfs[SCALE_MODE], 0), y - boxsz + Vec3(0, headOfs[SCALE_MODE], 0));
        }
        if (m_bNeedZ)
        {
            if (m_highlightMode == 3)
            {
                dc.SetColor(colZ);
            }
            else
            {
                dc.SetColor(colZArrow);
            }
            dc.DrawSolidBox(z + boxsz + Vec3(0, 0, headOfs[SCALE_MODE]), z - boxsz + Vec3(0, 0, headOfs[SCALE_MODE]));
        }
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Draw Rotate Circle
    //////////////////////////////////////////////////////////////////////////
    if (m_nModeFlags & ROTATE_CIRCLE_MODE)
    {
        if (m_bHighlightRotationCircle)
        {
            dc.SetLineWidth(5);
            dc.SetColor(colSelected);
        }
        else
        {
            dc.SetLineWidth(0);
            dc.SetColor(axisColor);
        }
        dc.DrawCircle(Vec3(0, 0, 0), kRotateCircleRadiusScale * m_fScreenScale, 0);
    }

    dc.SetLineWidth(0);

    // If only in move mode.
    if (m_nModeFlags == MOVE_MODE)
    {
        //////////////////////////////////////////////////////////////////////////
        // Draw axis planes.
        //////////////////////////////////////////////////////////////////////////
        Vec3 colXY[2];
        Vec3 colXZ[2];
        Vec3 colYZ[2];

        if (setup.enabled)
        {
            colX = Vec3(1, 0, 0);
            colY = Vec3(0, 1, 0);
            colZ = Vec3(0, 0, 1);
        }
        else
        {
            colX = colY = colZ = disabledColor;
        }
        colXY[0] = colX;
        colXY[1] = colY;
        colXZ[0] = colX;
        colXZ[1] = colZ;
        colYZ[0] = colY;
        colYZ[1] = colZ;

        linew[0] = linew[1] = linew[2] = 0;
        if (axis)
        {
            if (axis == AXIS_XY)
            {
                colXY[0] = colSelected;
                colXY[1] = colSelected;
                linew[0] = lineWidth;
            }
            else if (axis == AXIS_XZ)
            {
                colXZ[0] = colSelected;
                colXZ[1] = colSelected;
                linew[1] = lineWidth;
            }
            else if (axis == AXIS_YZ)
            {
                colYZ[0] = colSelected;
                colYZ[1] = colSelected;
                linew[2] = lineWidth;
            }
        }

        if (!(dc.flags & DISPLAY_2D))
        {
            if (axis == AXIS_TERRAIN)
            {
                dc.SetColor(colSelected, 0.4f);
            }
            else
            {
                dc.SetColor(QColor(128, 32, 32), 0.4f);
            }
            dc.DrawBall(Vec3(0.0f),  m_size * kSelectionBallScale);
        }

        dc.SetColor(QColor(255, 255, 0), 0.5f);

        float sz = m_size * kPlaneScale;
        Vec3 p1(sz, sz, 0);
        Vec3 p2(sz, 0, sz);
        Vec3 p3(0, sz, sz);

        float colAlpha = 1.0f;
        x *= kPlaneScale;
        y *= kPlaneScale;
        z *= kPlaneScale;

        // XY
        if (m_bNeedX && m_bNeedY)
        {
            dc.SetLineWidth(linew[0]);
            dc.SetColor(colXY[0], colAlpha);
            dc.DrawLine(p1, p1 - x);
            dc.SetColor(colXY[1], colAlpha);
            dc.DrawLine(p1, p1 - y);
        }

        // XZ
        if (m_bNeedX && m_bNeedZ)
        {
            dc.SetLineWidth(linew[1]);
            dc.SetColor(colXZ[0], colAlpha);
            dc.DrawLine(p2, p2 - x);
            dc.SetColor(colXZ[1], colAlpha);
            dc.DrawLine(p2, p2 - z);
        }

        // YZ
        if (m_bNeedY && m_bNeedZ)
        {
            dc.SetLineWidth(linew[2]);
            dc.SetColor(colYZ[0], colAlpha);
            dc.DrawLine(p3, p3 - y);
            dc.SetColor(colYZ[1], colAlpha);
            dc.DrawLine(p3, p3 - z);
        }

        dc.SetLineWidth(0);

        colAlpha = 0.25f;

        if (axis == AXIS_XY && m_bNeedX && m_bNeedY)
        {
            dc.CullOff();
            dc.SetColor(colSelected, colAlpha);
            dc.DrawQuad(p1, p1 - x, p1 - x - y, p1 - y);
            dc.CullOn();
        }
        else if (axis == AXIS_XZ && m_bNeedX && m_bNeedZ)
        {
            dc.CullOff();
            dc.SetColor(colSelected, colAlpha);
            dc.DrawQuad(p2, p2 - x, p2 - x - z, p2 - z);
            dc.CullOn();
        }
        else if (axis == AXIS_YZ && m_bNeedY && m_bNeedZ)
        {
            dc.CullOff();
            dc.SetColor(colSelected, colAlpha);
            dc.DrawQuad(p3, p3 - y, p3 - y - z, p3 - z);
            dc.CullOn();
        }
    }

    dc.PopMatrix();
    if (!(dc.flags & DISPLAY_2D))
    {
        dc.DepthTestOn();
    }

    dc.SetState(prevRState);
}

//////////////////////////////////////////////////////////////////////////
bool CAxisHelper::HitTestForRotationCircle(const Matrix34& worldTM, IDisplayViewport* view, const QPoint& pos, float fHitWidth, Vec3* pOutHitPos, Vec3* pOutHitNormal)
{
    Ray ray;
    view->ViewToWorldRay(pos, ray.origin, ray.direction);

    const float fCircleRadius = kRotateCircleRadiusScale * m_fScreenScale;

    std::vector<Vec3> vList;
    float step = 30.0f / 180 * gf_PI;
    for (float angle = 0; angle < 360.0f / 180 * gf_PI + step; angle += step)
    {
        Vec3 p(0, fCircleRadius * sin(angle), fCircleRadius * cos(angle));
        vList.push_back(worldTM.TransformPoint(p));
    }

    Vec3 vDir0 = (vList[0] - vList[1]).GetNormalized();
    Vec3 vDir1 = (vList[2] - vList[1]).GetNormalized();
    Vec3 vNormal = vDir1.Cross(vDir0).GetNormalized();

    if (pOutHitNormal)
    {
        *pOutHitNormal = vNormal;
    }

    Vec3 vShortestHitPos;
    float shortestDist = 3e11f;

    for (int i = 0, iCount(static_cast<int>(vList.size())); i < iCount; ++i)
    {
        const Vec3& v0 = vList[i];
        const Vec3& v1 = vList[(i + 1) % iCount];
        Vec3 vRight = (v1 - v0).Cross(vNormal).GetNormalized() * m_fScreenScale * fHitWidth;

        Vec3 p_v0 = v0 + vRight;
        Vec3 p_v1 = v1 + vRight;
        Vec3 p_v2 = v1 - vRight;
        Vec3 p_v3 = v0 - vRight;

        Vec3 hitPos;
        if (Intersect::Ray_Triangle(ray, p_v0, p_v1, p_v2, hitPos) || Intersect::Ray_Triangle(ray, p_v0, p_v2, p_v3, hitPos) ||
            Intersect::Ray_Triangle(ray, p_v0, p_v2, p_v1, hitPos) || Intersect::Ray_Triangle(ray, p_v0, p_v3, p_v2, hitPos))
        {
            Vec3 hitPosToEdge;
            if (PointToLineDistance(v0, v1, hitPos, hitPosToEdge))
            {
                if (!pOutHitPos)
                {
                    return true;
                }
                float hitDistance = hitPosToEdge.GetDistance(hitPos);
                if (hitDistance < shortestDist)
                {
                    vShortestHitPos = hitPosToEdge;
                    shortestDist = hitDistance;
                }
            }
        }
    }

    if (shortestDist < 3e10f)
    {
        if (pOutHitPos)
        {
            *pOutHitPos = vShortestHitPos;
        }
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool CAxisHelper::HitTest(const Matrix34& worldTM, const SGizmoParameters& setup, HitContext& hc)
{
    if (hc.distanceTolerance != 0)
    {
        return 0;
    }

    Prepare(worldTM, setup, hc.view);

    m_bHighlightRotationCircle = false;
    hc.manipulatorMode = 0;

    if (m_nModeFlags == ROTATE_CIRCLE_MODE)
    {
        if (HitTestForRotationCircle(m_matrix, hc.view, hc.point2d, 0.05f))
        {
            m_bHighlightRotationCircle = true;
            hc.manipulatorMode = 4;
            return true;
        }
        return false;
    }

    Vec3 x(m_size, 0, 0);
    Vec3 y(0, m_size, 0);
    Vec3 z(0, 0, m_size);

    Vec3 pos = m_matrix.GetTranslation();

    Vec3 intPoint;
    hc.view->ViewToWorldRay(hc.point2d, hc.raySrc, hc.rayDir);
    Ray ray(hc.raySrc, hc.rayDir);
    Sphere sphere(pos, m_size + 0.1f * m_fScreenScale);
    if (!Intersect::Ray_SphereFirst(ray, sphere, intPoint))
    {
        m_highlightAxis = 0;
        return false;
    }

    x = m_matrix.TransformVector(x);
    y = m_matrix.TransformVector(y);
    z = m_matrix.TransformVector(z);

    float sz = m_size * kPlaneScale;
    Vec3 p1(sz, sz, 0);
    Vec3 p2(sz, 0, sz);
    Vec3 p3(0, sz, sz);

    p1 = m_matrix.TransformPoint(p1);
    p2 = m_matrix.TransformPoint(p2);
    p3 = m_matrix.TransformPoint(p3);

    Vec3 planeX = x * kPlaneScale;
    Vec3 planeY = y * kPlaneScale;
    Vec3 planeZ = z * kPlaneScale;

    int axis = 0;

    Sphere testSphere(pos, m_size * kSelectionBallScale);
    if (!hc.b2DViewport && m_nModeFlags == MOVE_MODE && Intersect::Ray_SphereFirst(ray, testSphere, intPoint))
    {
        axis = AXIS_TERRAIN;
    }
    else
    {
        // Start the closestAxis value at the maximum selection radius based on the user's settings
        float closestAxis = aznumeric_cast<float>(GetIEditor()->GetObjectManager()->GetAxisHelperHitRadius());

        float distanceToAxis = hc.view->GetDistanceToLine(pos, pos + x, hc.point2d);
        if(distanceToAxis < closestAxis)
        {
            axis = AXIS_X;
            closestAxis = distanceToAxis;
        }
        distanceToAxis = hc.view->GetDistanceToLine(pos, pos + y, hc.point2d);
        if(distanceToAxis < closestAxis)
        {
            axis = AXIS_Y;
            closestAxis = distanceToAxis;
        }
        distanceToAxis = hc.view->GetDistanceToLine(pos, pos + z, hc.point2d);
        if(distanceToAxis < closestAxis)
        {
            axis = AXIS_Z;
            closestAxis = distanceToAxis;
        }
        if (m_nModeFlags == MOVE_MODE)
        {
            // If only in move mode.
            distanceToAxis = hc.view->GetDistanceToLine(p1, p1 - planeX, hc.point2d);
            if(distanceToAxis < closestAxis)
            {
                axis = AXIS_XY;
                closestAxis = distanceToAxis;
            }
            distanceToAxis = hc.view->GetDistanceToLine(p1, p1 - planeY, hc.point2d);
            if(distanceToAxis < closestAxis)
            {
                axis = AXIS_XY;
                closestAxis = distanceToAxis;
            }
            distanceToAxis = hc.view->GetDistanceToLine(p2, p2 - planeX, hc.point2d);
            if(distanceToAxis < closestAxis)
            {
                axis = AXIS_XZ;
                closestAxis = distanceToAxis;
            }
            distanceToAxis = hc.view->GetDistanceToLine(p2, p2 - planeZ, hc.point2d);
            if(distanceToAxis < closestAxis)
            {
                axis = AXIS_XZ;
                closestAxis = distanceToAxis;
            }
            distanceToAxis = hc.view->GetDistanceToLine(p3, p3 - planeY, hc.point2d);
            if(distanceToAxis < closestAxis)
            {
                axis = AXIS_YZ;
                closestAxis = distanceToAxis;
            }
            distanceToAxis = hc.view->GetDistanceToLine(p3, p3 - planeZ, hc.point2d);
            if(distanceToAxis < closestAxis)
            {
                axis = AXIS_YZ;
                closestAxis = distanceToAxis;
            }
            if (axis != 0)
            {
                hc.manipulatorMode = 1;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Calc sizes.
    //////////////////////////////////////////////////////////////////////////
    float headOfs[8];
    
    headOfs[MOVE_MODE] = 0;
    headOfs[ROTATE_MODE] = 0;
    headOfs[SCALE_MODE] = 0;
    
    if (m_nModeFlags == (MOVE_MODE | ROTATE_MODE | SCALE_MODE))
    {
        headOfs[ROTATE_MODE] += 0.05f * m_fScreenScale;
        headOfs[SCALE_MODE] += 0.1f * m_fScreenScale;
    }
    //////////////////////////////////////////////////////////////////////////
    if (axis == 0 && (m_nModeFlags & ROTATE_MODE))
    {
        Vec3 x2(m_size, 0, 0);
        Vec3 y2(0, m_size, 0);
        Vec3 z2(0, 0, m_size);
        int hitr = 10;
        if (m_bNeedX)
        {
            if (GetDistance2D(hc.view, hc.point2d, m_matrix.TransformPoint(x2 + Vec3(headOfs[ROTATE_MODE], 0, 0))) < hitr)
            {
                axis = AXIS_X;
                hc.manipulatorMode = 2;
            }
        }
        if (m_bNeedY)
        {
            if (GetDistance2D(hc.view, hc.point2d, m_matrix.TransformPoint(y2 + Vec3(0, headOfs[ROTATE_MODE], 0))) < hitr)
            {
                axis = AXIS_Y;
                hc.manipulatorMode = 2;
            }
        }
        if (m_bNeedZ)
        {
            if (GetDistance2D(hc.view, hc.point2d, m_matrix.TransformPoint(z2 + Vec3(0, 0, headOfs[ROTATE_MODE]))) < hitr)
            {
                axis = AXIS_Z;
                hc.manipulatorMode = 2;
            }
        }
    }
    if (m_nModeFlags & SCALE_MODE)
    {
        Vec3 x2(m_size, 0, 0);
        Vec3 y2(0, m_size, 0);
        Vec3 z2(0, 0, m_size);
        int hitr = 14;
        if (GetDistance2D(hc.view, hc.point2d, m_matrix.GetTranslation()) < hitr + 2)
        {
            axis = AXIS_XYZ;
            hc.manipulatorMode = 3;
        }
        if (axis == 0)
        {
            if (m_bNeedX)
            {
                if (GetDistance2D(hc.view, hc.point2d, m_matrix.TransformPoint(x2 + Vec3(headOfs[SCALE_MODE], 0, 0))) < hitr)
                {
                    axis = AXIS_X;
                    hc.manipulatorMode = 3;
                }
            }
            if (m_bNeedY)
            {
                if (GetDistance2D(hc.view, hc.point2d, m_matrix.TransformPoint(y2 + Vec3(0, headOfs[SCALE_MODE], 0))) < hitr)
                {
                    axis = AXIS_Y;
                    hc.manipulatorMode = 3;
                }
            }
            if (m_bNeedZ)
            {
                if (GetDistance2D(hc.view, hc.point2d, m_matrix.TransformPoint(z2 + Vec3(0, 0, headOfs[SCALE_MODE]))) < hitr)
                {
                    axis = AXIS_Z;
                    hc.manipulatorMode = 3;
                }
            }
        }
    }

    if (axis != 0)
    {
        if (hc.manipulatorMode == 0)
        {
            if (m_nModeFlags & MOVE_MODE)
            {
                hc.manipulatorMode = 1;
            }
            else if (m_nModeFlags & ROTATE_MODE)
            {
                hc.manipulatorMode = 2;
            }
            else if (m_nModeFlags & SCALE_MODE)
            {
                hc.manipulatorMode = 3;
            }
        }
        hc.axis = axis;
        hc.dist = 0;
        m_highlightMode = hc.manipulatorMode;
    }

    m_highlightAxis = axis;

    return axis != 0;
}

//////////////////////////////////////////////////////////////////////////
float CAxisHelper::GetDistance2D(IDisplayViewport* view, const QPoint& p, const Vec3& wp)
{
    QPoint vp = view->WorldToView(wp);
    float dx = static_cast<float>(p.x() - vp.x());
    float dy = static_cast<float>(p.y() - vp.y());
    return sqrtf(dx * dx + dy * dy);
}

