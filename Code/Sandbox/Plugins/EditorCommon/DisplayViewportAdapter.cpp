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

#include "EditorCommon_precompiled.h"
//Cry
#include <Cry_Camera.h>

//Editor
#include <Util/EditorUtils.h>
#include <Util/Math.h>

//Local
#include "QViewport.h"
#include "DisplayViewportAdapter.h"

CDisplayViewportAdapter::CDisplayViewportAdapter(QViewport* viewport)
    : m_viewport(viewport)
{
    m_screenMatrix.SetIdentity();
}

void CDisplayViewportAdapter::Update()
{
}

const Matrix34& CDisplayViewportAdapter::GetScreenTM() const
{
    return m_screenMatrix;
}

float CDisplayViewportAdapter::GetScreenScaleFactor(const Vec3& position) const
{
    float dist = m_viewport->Camera()->GetPosition().GetDistance(position);
    if (dist < m_viewport->Camera()->GetNearPlane())
    {
        dist = m_viewport->Camera()->GetNearPlane();
    }
    return dist;
}

float CDisplayViewportAdapter::GetScreenScaleFactor([[maybe_unused]] const CCamera& camera, [[maybe_unused]] const Vec3& object_position)
{
    return 1;
}

bool CDisplayViewportAdapter::HitTestLine(const Vec3& lineP1, const Vec3& lineP2, const QPoint& hitpoint, int pixelRadius, float* pToCameraDistance) const
{
    float dist = GetDistanceToLine(lineP1, lineP2, hitpoint);
    if (dist <= pixelRadius)
    {
        if (pToCameraDistance)
        {
            Vec3 raySrc, rayDir;
            ViewToWorldRay(hitpoint, raySrc, rayDir);
            Vec3 rayTrg = raySrc + rayDir * 10000.0f;

            Vec3 pa, pb;
            float mua, mub;
            LineLineIntersect(lineP1, lineP2, raySrc, rayTrg, pa, pb, mua, mub);
            *pToCameraDistance = mub;
        }

        return true;
    }

    return false;
}

float CDisplayViewportAdapter::GetDistanceToLine(const Vec3& lineP1, const Vec3& lineP2, const QPoint& point) const
{
    QPoint p1 = WorldToView(lineP1);
    QPoint p2 = WorldToView(lineP2);

    return PointToLineDistance2D(
        Vec3(float(p1.x()), float(p1.y()), 0), 
        Vec3(float(p2.x()), float(p2.y()), 0), 
        Vec3(float(point.x()), float(point.y()), 0));
}

CBaseObjectsCache* CDisplayViewportAdapter::GetVisibleObjectsCache()
{
    return 0;
}

bool CDisplayViewportAdapter::IsBoundsVisible([[maybe_unused]] const AABB& box) const
{
    return false;
}

void CDisplayViewportAdapter::GetPerpendicularAxis(EAxis* axis, bool* is2D) const
{
    *axis = AXIS_NONE;
    *is2D = false;
}

const Matrix34& CDisplayViewportAdapter::GetViewTM() const
{
    m_viewMatrix = m_viewport->Camera()->GetViewMatrix();
    return m_viewMatrix;
}

QPoint CDisplayViewportAdapter::WorldToView(const Vec3& worldPoint) const
{
    return m_viewport->ProjectToScreen(worldPoint);
}

QPoint CDisplayViewportAdapter::WorldToViewParticleEditor(const Vec3& worldPoint, [[maybe_unused]] int width, [[maybe_unused]] int height) const
{
    return m_viewport->ProjectToScreen(worldPoint);
}

Vec3 CDisplayViewportAdapter::WorldToView3D([[maybe_unused]] const Vec3& worldPoint, [[maybe_unused]] int flags) const
{
    return Vec3(0.0f, 0.0f, 0.0f);
}

Vec3 CDisplayViewportAdapter::ViewToWorld([[maybe_unused]] const QPoint& vp, [[maybe_unused]] bool* collideWithTerrain, [[maybe_unused]] bool onlyTerrain, [[maybe_unused]] bool bSkipVegetation, [[maybe_unused]] bool bTestRenderMesh, [[maybe_unused]] bool* collideWithObject) const
{
    return Vec3(0.0f, 0.0f, 0.0f);
}

void CDisplayViewportAdapter::ViewToWorldRay(const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const
{
    Ray ray;
    // this can fail for number of reasons
    if (!m_viewport->ScreenToWorldRay(&ray, vp.x(), vp.y()))
    {
        // return some "safe" default that will not cause FPE
        raySrc = m_viewport->Camera()->GetPosition();
        rayDir = m_viewport->Camera()->GetViewdir();

        // the interface should be changed to accommodate for error
        return;
    }
    raySrc = ray.origin;
    rayDir = ray.direction;
}

float CDisplayViewportAdapter::GetGridStep() const
{
    return 1.0f;
}

float CDisplayViewportAdapter::GetAspectRatio() const
{
    int w, h;
    GetDimensions(&w, &h);
    if (h != 0)
    {
        return float(w) / h;
    }
    else
    {
        return 1.0f;
    }
}

const Plane* CDisplayViewportAdapter::GetConstructionPlane() const
{
    return 0;
}

void CDisplayViewportAdapter::ScreenToClient([[maybe_unused]] QPoint& pt) const
{
}

void CDisplayViewportAdapter::GetDimensions(int* width, int* height) const
{
    if (width)
    {
        *width = m_viewport->Width();
    }
    if (height)
    {
        *height = m_viewport->Height();
    }
}

void CDisplayViewportAdapter::setRay([[maybe_unused]] QPoint& vp, [[maybe_unused]] Vec3& raySrc, [[maybe_unused]] Vec3& rayDir)
{
}

void CDisplayViewportAdapter::setHitcontext([[maybe_unused]] QPoint& vp, [[maybe_unused]] Vec3& raySrc, [[maybe_unused]] Vec3& rayDir)
{
}
