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

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERATOMSHIM_ATOMSHIMRENDERAUXGEOM_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERATOMSHIM_ATOMSHIMRENDERAUXGEOM_H
#pragma once
#include "../Common/RenderAuxGeom.h"
#include <AzCore/Math/Matrix4x4.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>

class CAtomShimRenderer;
class ICrySizer;

class CAtomShimRenderAuxGeom
    : public IRenderAuxGeom
{
public:
    // interface
    virtual void SetRenderFlags(const SAuxGeomRenderFlags& renderFlags);
    virtual SAuxGeomRenderFlags GetRenderFlags();

    virtual void Flush() {}
    virtual void Commit([[maybe_unused]] uint frames = 0) {}
    virtual void Process() {}

    virtual void DrawPoint(const Vec3& v, const ColorB& col, uint8 size = 1);
    virtual void DrawPoints(const Vec3* v, uint32 numPoints, const ColorB& col, uint8 size = 1);
    virtual void DrawPoints(const Vec3* v, uint32 numPoints, const ColorB* col, uint8 size = 1);

    virtual void DrawLine(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, float thickness = 1.0f);
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const ColorB& col, float thickness = 1.0f);
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const ColorB* col, float thickness = 1.0f);
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB& col, float thickness = 1.0f);
    virtual void DrawLines(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB* col, float thickness = 1.0f);
    virtual void DrawPolyline(const Vec3* v, uint32 numPoints, bool closed, const ColorB& col, float thickness = 1.0f);
    virtual void DrawPolyline(const Vec3* v, uint32 numPoints, bool closed, const ColorB* col, float thickness = 1.0f);

    virtual void DrawTriangle(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, const Vec3& v2, const ColorB& colV2);
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const ColorB& col);
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const ColorB* col);
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB& col);
    virtual void DrawTriangles(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB* col);

    virtual void DrawQuad(float width, float height, const Matrix34& matWorld, const ColorB& col, bool drawShaded = true);

    virtual void DrawAABB(const AABB& aabb, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle);
    virtual void DrawAABBs(const AABB* aabb, uint32 aabbCount, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle);
    virtual void DrawAABB(const AABB& aabb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle);

    virtual void DrawOBB(const OBB& obb, const Vec3& pos, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle);
    virtual void DrawOBB(const OBB& obb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle);

    virtual void DrawSphere(const Vec3& pos, float radius, const ColorB& col, bool drawShaded = true);
    virtual void DrawDisk(const Vec3& pos, const Vec3& dir, float radius, const ColorB& col, bool drawShaded = true);
    virtual void DrawCone(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded = true);
    virtual void DrawCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded = true);

    virtual void DrawBone(const Vec3& rParent, const Vec3& rBone, ColorB col);

    virtual void RenderText([[maybe_unused]] Vec3 pos, [[maybe_unused]] SDrawTextInfo& ti, [[maybe_unused]] const char* format, [[maybe_unused]] va_list args);

public:
    static CAtomShimRenderAuxGeom* Create(CAtomShimRenderer& renderer)
    {
        if (s_pThis == NULL)
        {
            s_pThis = new CAtomShimRenderAuxGeom(renderer);
        }
        return s_pThis;
    }

public:
    ~CAtomShimRenderAuxGeom();

    void BeginFrame();
    void EndFrame();

    void SetViewProjOverride(const AZ::Matrix4x4& viewProj);
    void UnsetViewProjOverride();

private:
    CAtomShimRenderAuxGeom(CAtomShimRenderer& renderer);

    int32_t m_viewProjOverrideIndex = -1;
    AZ::RPI::AuxGeomDraw::AuxGeomDynamicIndexedDrawArguments m_drawArgs;

    CAtomShimRenderer* m_renderer;

    static CAtomShimRenderAuxGeom* s_pThis;

    SAuxGeomRenderFlags m_cryRenderFlags;
};

#endif // NULL_RENDER_AUX_GEOM_H
