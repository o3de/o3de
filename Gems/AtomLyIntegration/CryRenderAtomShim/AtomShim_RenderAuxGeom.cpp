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

#include "CryRenderOther_precompiled.h"
#include "AtomShim_Renderer.h"
#include "AtomShim_RenderAuxGeom.h"

#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <MathConversion.h>

CAtomShimRenderAuxGeom* CAtomShimRenderAuxGeom::s_pThis = NULL;

namespace
{
    using DrawFunction = AZStd::function<void(const uint32* indices)>;
    void Handle16BitIndices(const vtx_idx* ind, uint32_t numIndices, DrawFunction drawFunc)
    {
        constexpr bool copyIndicesToUint32 = sizeof(vtx_idx) != sizeof(uint32_t); // mobile platforms use 16 bit vtx_idx
        if constexpr(copyIndicesToUint32)
        {
            uint32_t* indices = new uint32_t[numIndices];
            for (int i = 0; i < numIndices; ++i)
            {
                indices[i] = ind[i];
            }
            drawFunc(indices);
            delete[] indices;
        }
        else
        {
            // re-interpret because on mobile vtx_idx is a uint16
            // Additionally the else case should not be taken on mobile
            // because sizeof(uint16) < sizeof(uint32).
            const uint32_t* indices = reinterpret_cast<const uint32_t*>(ind);
            drawFunc(indices);
        }
    }

    AZ::RPI::AuxGeomDraw::DrawStyle LyDrawStyleToAZDrawStyle(bool bSolid, EBoundingBoxDrawStyle bbDrawStyle)
    {
        AZ::RPI::AuxGeomDraw::DrawStyle drawStyle = AZ::RPI::AuxGeomDraw::DrawStyle::Solid;
        if (!bSolid)
        {
            drawStyle = AZ::RPI::AuxGeomDraw::DrawStyle::Line;
        }
        else if (bbDrawStyle == eBBD_Extremes_Color_Encoded)
        {
            drawStyle = AZ::RPI::AuxGeomDraw::DrawStyle::Shaded;    // Not the same but shows a difference
        }
        return drawStyle;
    }

    AZ::Aabb LyAABBToAZAabbWithFixup(const AABB& source)
    {
        AABB fixed;
        fixed.min.x = AZStd::min(source.min.x, source.max.x);
        fixed.min.y = AZStd::min(source.min.y, source.max.y);
        fixed.min.z = AZStd::min(source.min.z, source.max.z);
        fixed.max.x = AZStd::max(source.min.x, source.max.x);
        fixed.max.y = AZStd::max(source.min.y, source.max.y);
        fixed.max.z = AZStd::max(source.min.z, source.max.z);
        return LyAABBToAZAabb(fixed);
    }

}

CAtomShimRenderAuxGeom::CAtomShimRenderAuxGeom(CAtomShimRenderer& renderer)
    : m_renderer(&renderer)
{
}

CAtomShimRenderAuxGeom::~CAtomShimRenderAuxGeom()
{
}

void CAtomShimRenderAuxGeom::BeginFrame()
{
}

void CAtomShimRenderAuxGeom::EndFrame()
{
}

void CAtomShimRenderAuxGeom::SetViewProjOverride(const AZ::Matrix4x4& viewProj)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        m_viewProjOverrideIndex = auxGeom->AddViewProjOverride(viewProj);
    }
}

void CAtomShimRenderAuxGeom::UnsetViewProjOverride()
{
    m_viewProjOverrideIndex = -1;
}

void CAtomShimRenderAuxGeom::SetRenderFlags(const SAuxGeomRenderFlags& renderFlags)
{
    m_cryRenderFlags = renderFlags;
    m_drawArgs.m_depthTest = renderFlags.GetDepthTestFlag() == EAuxGeomPublicRenderflags_DepthTest::e_DepthTestOff ?
        AZ::RPI::AuxGeomDraw::DepthTest::Off : AZ::RPI::AuxGeomDraw::DepthTest::On;
}

SAuxGeomRenderFlags CAtomShimRenderAuxGeom::GetRenderFlags()
{
    return m_cryRenderFlags;
}

void CAtomShimRenderAuxGeom::DrawPoint(const Vec3& v, const ColorB& col, uint8 size /* = 1  */)
{
    DrawPoints(&v, 1, col, size);
}

void CAtomShimRenderAuxGeom::DrawPoints(const Vec3* v, uint32 numPoints, const ColorB* col, uint8 size /* = 1  */)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        AZ::Color* colors = new AZ::Color[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
            colors[i] = LYColorBToAZColor(col[i]);
        }
        AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs(m_drawArgs);
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = numPoints;
        drawArgs.m_colors = colors;
        drawArgs.m_colorCount = numPoints;
        drawArgs.m_size = size;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;

        auxGeom->DrawPoints(drawArgs);

        delete[] points;
    }
}

void CAtomShimRenderAuxGeom::DrawPoints(const Vec3* v, uint32 numPoints, const ColorB& col, uint8 size /* = 1  */)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
        }
        AZ::Color color = LYColorBToAZColor(col);

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = numPoints;
        drawArgs.m_colors = &color;
        drawArgs.m_colorCount = 1;
        drawArgs.m_size = size;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;

        auxGeom->DrawPoints(drawArgs);

        delete[] points;
    }
}

void CAtomShimRenderAuxGeom::DrawLine(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, float thickness /* = 1.0f  */)
{
    const Vec3 verts[2] = {v0, v1};
    const ColorB colors[2] = {colV0, colV1};
    DrawLines(verts, 2, colors, thickness);
}

void CAtomShimRenderAuxGeom::DrawLines(const Vec3* v, uint32 numPoints, const ColorB& col, float thickness /* = 1.0f  */)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
        }

        AZ::Color color = LYColorBToAZColor(col);

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs(m_drawArgs);
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = numPoints;
        drawArgs.m_colors = &color;
        drawArgs.m_colorCount = 1;
        drawArgs.m_size = thickness;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;
        auxGeom->DrawLines(drawArgs);

        delete[] points;
    }
}

void CAtomShimRenderAuxGeom::DrawLines(const Vec3* v, uint32 numPoints, const ColorB* col, float thickness /* = 1.0f  */)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        AZ::Color* colors = new AZ::Color[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
            colors[i] = LYColorBToAZColor(col[i]);
        }

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs(m_drawArgs);
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = numPoints;
        drawArgs.m_colors = colors;
        drawArgs.m_colorCount = numPoints;
        drawArgs.m_size = thickness;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;

        auxGeom->DrawLines(drawArgs);

        delete[] points;
        delete[] colors;
    }
}

void CAtomShimRenderAuxGeom::DrawLines(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB& col, float thickness /* = 1.0f  */)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
        }

        AZ::Color color = LYColorBToAZColor(col);

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicIndexedDrawArguments drawArgs(m_drawArgs);
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = numPoints;
        drawArgs.m_indexCount = numIndices;
        drawArgs.m_colors = &color;
        drawArgs.m_colorCount = 1;
        drawArgs.m_size = thickness;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;

        Handle16BitIndices(
            ind, numIndices,
            [&drawArgs, auxGeom](const uint32_t* indices)
            {
                drawArgs.m_indices = indices;
                auxGeom->DrawLines(drawArgs);
            }
        );

        delete[] points;
    }
}

void CAtomShimRenderAuxGeom::DrawLines(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB* col, float thickness /* = 1.0f  */)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        AZ::Color* colors = new AZ::Color[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
            colors[i] = LYColorBToAZColor(col[i]);
        }

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicIndexedDrawArguments drawArgs(m_drawArgs);
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = numPoints;
        drawArgs.m_indexCount = numIndices;
        drawArgs.m_colors = colors;
        drawArgs.m_colorCount = numPoints;
        drawArgs.m_size = thickness;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;

        Handle16BitIndices(
            ind, numIndices,
            [&drawArgs, auxGeom](const uint32_t* indices)
            {
                drawArgs.m_indices = indices;
                auxGeom->DrawLines(drawArgs);
            }
        );

        delete[] points;
        delete[] colors;
    }
}

void CAtomShimRenderAuxGeom::DrawPolyline(const Vec3* v, uint32 numPoints, bool closed, const ColorB& col, float thickness /* = 1.0f  */)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
        }

        AZ::Color color = LYColorBToAZColor(col);
        AZ::RPI::AuxGeomDraw::PolylineEnd polylineClosed = closed ? AZ::RPI::AuxGeomDraw::PolylineEnd::Closed : AZ::RPI::AuxGeomDraw::PolylineEnd::Open;

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = numPoints;
        drawArgs.m_colors = &color;
        drawArgs.m_colorCount = 1;
        drawArgs.m_size = thickness;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;

        auxGeom->DrawPolylines(drawArgs, polylineClosed);

        delete[] points;
    }
}

void CAtomShimRenderAuxGeom::DrawPolyline(const Vec3* v, uint32 numPoints, bool closed, const ColorB* col, float thickness /* = 1.0f  */)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        AZ::Color* colors = new AZ::Color[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
            colors[i] = LYColorBToAZColor(col[i]);
        }

        AZ::RPI::AuxGeomDraw::PolylineEnd polylineClosed = closed ? AZ::RPI::AuxGeomDraw::PolylineEnd::Closed : AZ::RPI::AuxGeomDraw::PolylineEnd::Open;

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = numPoints;
        drawArgs.m_colors = colors;
        drawArgs.m_colorCount = numPoints;
        drawArgs.m_size = thickness;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;
        auxGeom->DrawPolylines(drawArgs, polylineClosed);

        delete[] points;
        delete[] colors;
    }
}

void CAtomShimRenderAuxGeom::DrawTriangle(const Vec3& v0, const ColorB& colV0, const Vec3& v1, const ColorB& colV1, const Vec3& v2, const ColorB& colV2)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3 points[3] =
        {
            LYVec3ToAZVec3(v0),
            LYVec3ToAZVec3(v1),
            LYVec3ToAZVec3(v2),
        };

        AZ::Color colors[3] =
        {
            LYColorBToAZColor(colV0),
            LYColorBToAZColor(colV1),
            LYColorBToAZColor(colV2),
        };

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = 3;
        drawArgs.m_colors = colors;
        drawArgs.m_colorCount = 3;
        drawArgs.m_opacityType = (colV0.a == 0xFF && colV1.a == 0xFF && colV2.a == 0xFF) ? AZ::RPI::AuxGeomDraw::OpacityType::Opaque : AZ::RPI::AuxGeomDraw::OpacityType::Translucent;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;

        auxGeom->DrawTriangles(drawArgs);
    }
}

void CAtomShimRenderAuxGeom::DrawTriangles(const Vec3* v, uint32 numPoints, const ColorB& col)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
        }
        AZ::Color color = LYColorBToAZColor(col);

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = numPoints;
        drawArgs.m_colors = &color;
        drawArgs.m_colorCount = 1;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;

        auxGeom->DrawTriangles(drawArgs);
        delete[] points;
    }
}

void CAtomShimRenderAuxGeom::DrawTriangles(const Vec3* v, uint32 numPoints, const ColorB* col)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        AZ::Color* colors = new AZ::Color[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
            colors[i] = LYColorBToAZColor(col[i]);
        }

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments drawArgs;
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = 3;
        drawArgs.m_colors = colors;
        drawArgs.m_colorCount = numPoints;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;

        auxGeom->DrawTriangles(drawArgs);
        delete[] points;
        delete[] colors;
    }
}

void CAtomShimRenderAuxGeom::DrawTriangles(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB& col)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
        }
        AZ::Color color = LYColorBToAZColor(col);

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicIndexedDrawArguments drawArgs(m_drawArgs);
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = numPoints;
        drawArgs.m_indexCount = numIndices;
        drawArgs.m_colors = &color;
        drawArgs.m_colorCount = 1;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;

        Handle16BitIndices(
            ind, numIndices,
            [&drawArgs, auxGeom](const uint32_t* indices)
            {
                drawArgs.m_indices = indices;
                auxGeom->DrawTriangles(drawArgs);
            }
        );

        delete[] points;
    }
}

void CAtomShimRenderAuxGeom::DrawTriangles(const Vec3* v, uint32 numPoints, const vtx_idx* ind, uint32 numIndices, const ColorB* col)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Vector3* points = new AZ::Vector3[numPoints];
        AZ::Color* colors = new AZ::Color[numPoints];
        for (int i = 0; i < numPoints; ++i)
        {
            points[i] = LYVec3ToAZVec3(v[i]);
            colors[i] = LYColorBToAZColor(col[i]);
        }

        AZ::RPI::AuxGeomDraw::AuxGeomDynamicIndexedDrawArguments drawArgs(m_drawArgs);
        drawArgs.m_verts = points;
        drawArgs.m_vertCount = numPoints;
        drawArgs.m_indexCount = numIndices;
        drawArgs.m_colors = colors;
        drawArgs.m_colorCount = numPoints;
        drawArgs.m_viewProjectionOverrideIndex = m_viewProjOverrideIndex;

        Handle16BitIndices(
            ind, numIndices,
            [&drawArgs, auxGeom](const uint32_t* indices)
            {
                drawArgs.m_indices = indices;
                auxGeom->DrawTriangles(drawArgs);
            }
        );

        delete[] points;
        delete[] colors;
    }
}

void CAtomShimRenderAuxGeom::DrawQuad(float width, float height, const Matrix34& matWorld, const ColorB& col, bool drawShaded)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::RPI::AuxGeomDraw::DrawStyle drawStyle = drawShaded ? AZ::RPI::AuxGeomDraw::DrawStyle::Shaded : AZ::RPI::AuxGeomDraw::DrawStyle::Solid;
        AZ::Transform transform = LYTransformToAZTransform(matWorld);
        auxGeom->DrawQuad(width, height, transform, LYColorBToAZColor(col), drawStyle, m_drawArgs.m_depthTest);
    }
}

void CAtomShimRenderAuxGeom::DrawAABB(const AABB& aabb, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        auxGeom->DrawAabb(LyAABBToAZAabbWithFixup(aabb), LYColorBToAZColor(col), LyDrawStyleToAZDrawStyle(bSolid, bbDrawStyle), m_drawArgs.m_depthTest);
    }
}

void CAtomShimRenderAuxGeom::DrawAABBs(const AABB* aabb, uint32 aabbCount, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        for (int i = 0; i < aabbCount; ++aabbCount)
        {
            auxGeom->DrawAabb(
                LyAABBToAZAabbWithFixup(aabb[i]),
                LYColorBToAZColor(col),
                LyDrawStyleToAZDrawStyle(bSolid, bbDrawStyle),
                m_drawArgs.m_depthTest,
                AZ::RPI::AuxGeomDraw::DepthWrite::On,
                AZ::RPI::AuxGeomDraw::FaceCullMode::Back,
                m_viewProjOverrideIndex);
        }
    }
}

void CAtomShimRenderAuxGeom::DrawAABB(const AABB& aabb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Matrix3x4 transform = LYTransformToAZMatrix3x4(matWorld);
        auxGeom->DrawAabb(
            LyAABBToAZAabbWithFixup(aabb),
            transform,
            LYColorBToAZColor(col),
            LyDrawStyleToAZDrawStyle(bSolid, bbDrawStyle),
            m_drawArgs.m_depthTest,
            AZ::RPI::AuxGeomDraw::DepthWrite::On,
            AZ::RPI::AuxGeomDraw::FaceCullMode::Back,
            m_viewProjOverrideIndex);
    }
}

void CAtomShimRenderAuxGeom::DrawOBB(const OBB& obb, const Vec3& pos, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        auxGeom->DrawObb(LyOBBtoAZObb(obb), LYVec3ToAZVec3(pos), LYColorBToAZColor(col), LyDrawStyleToAZDrawStyle(bSolid, bbDrawStyle), m_drawArgs.m_depthTest);
    }
}

void CAtomShimRenderAuxGeom::DrawOBB(const OBB& obb, const Matrix34& matWorld, bool bSolid, const ColorB& col, const EBoundingBoxDrawStyle& bbDrawStyle)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::Matrix3x4 transform = LYTransformToAZMatrix3x4(matWorld);
        auxGeom->DrawObb(LyOBBtoAZObb(obb), transform, LYColorBToAZColor(col), LyDrawStyleToAZDrawStyle(bSolid, bbDrawStyle), m_drawArgs.m_depthTest);
    }
}

void CAtomShimRenderAuxGeom::DrawSphere(const Vec3& pos, float radius, const ColorB& col, bool drawShaded)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::RPI::AuxGeomDraw::DrawStyle drawStyle = drawShaded ? AZ::RPI::AuxGeomDraw::DrawStyle::Shaded : AZ::RPI::AuxGeomDraw::DrawStyle::Solid;
        auxGeom->DrawSphere(LYVec3ToAZVec3(pos), radius, LYColorBToAZColor(col), drawStyle, m_drawArgs.m_depthTest);
    }
}

void CAtomShimRenderAuxGeom::DrawDisk(const Vec3& pos, const Vec3& dir, float radius, const ColorB& col, bool drawShaded)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::RPI::AuxGeomDraw::DrawStyle drawStyle = drawShaded ? AZ::RPI::AuxGeomDraw::DrawStyle::Shaded : AZ::RPI::AuxGeomDraw::DrawStyle::Solid;
        auxGeom->DrawDisk(LYVec3ToAZVec3(pos), LYVec3ToAZVec3(dir), radius, LYColorBToAZColor(col), drawStyle, m_drawArgs.m_depthTest);
    }
}

void CAtomShimRenderAuxGeom::DrawCone(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::RPI::AuxGeomDraw::DrawStyle drawStyle = drawShaded ? AZ::RPI::AuxGeomDraw::DrawStyle::Shaded : AZ::RPI::AuxGeomDraw::DrawStyle::Solid;
        auxGeom->DrawCone(LYVec3ToAZVec3(pos), LYVec3ToAZVec3(dir), radius, height, LYColorBToAZColor(col), drawStyle, m_drawArgs.m_depthTest);
    }
}

void CAtomShimRenderAuxGeom::DrawCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, const ColorB& col, bool drawShaded)
{
    auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
    if (auto auxGeom = AZ::RPI::AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
    {
        AZ::RPI::AuxGeomDraw::DrawStyle drawStyle = drawShaded ? AZ::RPI::AuxGeomDraw::DrawStyle::Shaded : AZ::RPI::AuxGeomDraw::DrawStyle::Solid;
        auxGeom->DrawCylinder(LYVec3ToAZVec3(pos), LYVec3ToAZVec3(dir), radius, height, LYColorBToAZColor(col), drawStyle, m_drawArgs.m_depthTest);
    }
}

void CAtomShimRenderAuxGeom::DrawBone(const Vec3& p, const Vec3& c,  ColorB col)
{
    Vec3 vBoneVec = c - p;
    float fBoneLength = vBoneVec.GetLength();

    if (fBoneLength < 1e-4)
    {
        return;
    }

    Matrix33 m33 = Matrix33::CreateRotationV0V1(Vec3(1, 0, 0), vBoneVec / fBoneLength);
    Matrix34 m34 = Matrix34(m33, p);

    f32 t       =   min(0.01f, fBoneLength * 0.05f);

    //bone points in x-direction
    Vec3 s  =   Vec3(ZERO);
    Vec3 m0 =   Vec3(t, +t, +t);
    Vec3 m1 =   Vec3(t, -t, +t);
    Vec3 m2 =   Vec3(t, -t, -t);
    Vec3 m3 =   Vec3(t, +t, -t);
    Vec3 e  =   Vec3(fBoneLength, 0, 0);

    Vec3 VBuffer[6];
    ColorB CBuffer[6];

    VBuffer[0]  =   m34 * s;
    CBuffer[0]  =   RGBA8(0xff, 0x1f, 0x1f, 0x00);  //start of bone (joint)

    VBuffer[1]  =   m34 * m0;
    CBuffer[1]  =   col;
    VBuffer[2]  =   m34 * m1;
    CBuffer[2]  =   col;
    VBuffer[3]  =   m34 * m2;
    CBuffer[3]  =   col;
    VBuffer[4]  =   m34 * m3;
    CBuffer[4]  =   col;

    VBuffer[5]  =   m34 * e;
    CBuffer[5]  =   RGBA8(0x07, 0x0f, 0x1f, 0x00); //end of bone


    DrawLine(VBuffer[0], CBuffer[0], VBuffer[1], CBuffer[1]);
    DrawLine(VBuffer[0], CBuffer[0], VBuffer[2], CBuffer[2]);
    DrawLine(VBuffer[0], CBuffer[0], VBuffer[3], CBuffer[3]);
    DrawLine(VBuffer[0], CBuffer[0], VBuffer[4], CBuffer[4]);

    DrawLine(VBuffer[1], CBuffer[1], VBuffer[2], CBuffer[2]);
    DrawLine(VBuffer[2], CBuffer[2], VBuffer[3], CBuffer[3]);
    DrawLine(VBuffer[3], CBuffer[3], VBuffer[4], CBuffer[4]);
    DrawLine(VBuffer[4], CBuffer[4], VBuffer[1], CBuffer[1]);

    DrawLine(VBuffer[5], CBuffer[5], VBuffer[1], CBuffer[1]);
    DrawLine(VBuffer[5], CBuffer[5], VBuffer[2], CBuffer[2]);
    DrawLine(VBuffer[5], CBuffer[5], VBuffer[3], CBuffer[3]);
    DrawLine(VBuffer[5], CBuffer[5], VBuffer[4], CBuffer[4]);
}

void CAtomShimRenderAuxGeom::RenderText([[maybe_unused]] Vec3 pos, [[maybe_unused]] SDrawTextInfo& ti, [[maybe_unused]] const char* format, [[maybe_unused]] va_list args)
{
    if (format && !gEnv->IsDedicated())
    {
        char str[512];

        vsnprintf_s(str, sizeof(str), sizeof(str) - 1, format, args);
        str[sizeof(str) - 1] = '\0';
        gEnv->pRenderer->DrawTextQueued(pos, ti, str);
    }
}

