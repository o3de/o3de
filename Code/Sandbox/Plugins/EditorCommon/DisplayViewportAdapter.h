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
#pragma once

//Cry
#include <Cry_Math.h>
#include "Cry_Matrix34.h"
#include "Cry_Vector3.h"

//Editor
#include "Include/IDisplayViewport.h"

//Local
#include "EditorCommonAPI.h"

class EDITOR_COMMON_API QViewport;

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class EDITOR_COMMON_API CDisplayViewportAdapter
    : public ::IDisplayViewport
{
public:
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    CDisplayViewportAdapter(QViewport* viewport);

    void Update() override;
    const Matrix34& GetScreenTM() const override;
    float GetScreenScaleFactor(const Vec3& position) const override;
    float GetScreenScaleFactor(const CCamera& camera, const Vec3& object_position) override;
    bool HitTestLine(const Vec3& lineP1, const Vec3& lineP2, const QPoint& hitpoint, int pixelRadius, float* pToCameraDistance = 0) const override;
    float GetDistanceToLine(const Vec3& lineP1, const Vec3& lineP2, const QPoint& point) const override;
    CBaseObjectsCache* GetVisibleObjectsCache() override;
    bool IsBoundsVisible(const AABB& box) const override;
    void GetPerpendicularAxis(EAxis* axis, bool* is2D) const override;
    const Matrix34& GetViewTM() const override;
    QPoint WorldToView(const Vec3& worldPoint) const override;
    QPoint WorldToViewParticleEditor(const Vec3& worldPoint, int width, int height) const override;
    Vec3 WorldToView3D(const Vec3& worldPoint, int flags = 0) const override;
    Vec3 ViewToWorld(const QPoint& vp, bool* collideWithTerrain = nullptr, bool onlyTerrain = false, bool bSkipVegetation = false, bool bTestRenderMesh = false, bool* collideWithObject = nullptr) const override;
    void ViewToWorldRay(const QPoint& vp, Vec3& raySrc, Vec3& rayDir) const override;
    float GetGridStep() const override;
    float GetAspectRatio() const override;
    const Plane* GetConstructionPlane() const override;
    void ScreenToClient(QPoint& pt) const override;
    void GetDimensions(int* width, int* height) const override;
    void setRay(QPoint& vp, Vec3& raySrc, Vec3& rayDir) override;
    void setHitcontext(QPoint& vp, Vec3& raySrc, Vec3& rayDir) override;

private:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    mutable Matrix34 m_viewMatrix;
    Matrix34 m_screenMatrix;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    QViewport* m_viewport;
};
