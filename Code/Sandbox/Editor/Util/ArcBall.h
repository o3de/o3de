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

#ifndef CRYINCLUDE_EDITOR_UTIL_ARCBALL_H
#define CRYINCLUDE_EDITOR_UTIL_ARCBALL_H
#pragma once
#include <Cry_Math.h>
#include <Cry_Color.h>

#define CrossDist (0.05f)
#define AxisDist  (0.05f)

class SANDBOX_API CArcBall3D
{
public:
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    uint32 RotControl;
    Sphere sphere;
    uint32 Mouse_CutFlag;
    uint32 Mouse_CutFlagStart;
    uint32 AxisSnap;
    Vec3 LineStart3D;
    Vec3 Mouse_CutOnUnitSphere;
    Quat DragRotation;
    Quat ObjectRotation;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    CArcBall3D()
    {
        InitArcBall();
    };

    void InitArcBall()
    {
        RotControl = 0;
        sphere(Vec3(ZERO), 0.25f);
        Mouse_CutFlag = 0;
        Mouse_CutOnUnitSphere(0, 0, 0);
        LineStart3D(0, -1, 0);
        AxisSnap = 0;
        DragRotation.SetIdentity();
        ObjectRotation.SetIdentity();
    }

    //---------------------------------------------------------------
    // ArcControl
    // Returns true if the rotation has changed
    //---------------------------------------------------------------
    bool ArcControl(const Matrix34& reference, const Ray& ray, uint32 mouseleft);
    void ArcRotation();
    void DrawSphere(const Matrix34& reference, const CCamera& cam, struct IRenderAuxGeom* pRenderer);
    static uint32 IntersectSphereLineSegment(const Sphere& s, const Vec3& LineStart, const Vec3& LineEnd, Vec3& I);
};
#endif // CRYINCLUDE_EDITOR_UTIL_ARCBALL_H
