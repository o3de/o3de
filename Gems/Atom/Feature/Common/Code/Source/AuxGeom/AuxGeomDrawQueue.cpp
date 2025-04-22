/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "AuxGeomDrawQueue.h"

#include <Atom/RPI.Public/Scene.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/std/functional.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            AZ::u32 PackColor(AZ::Color color)
            {
                // We use the format RHI::Format::R8G8B8A8_UNORM
                return (color.GetA8() << 24) | (color.GetB8() << 16) | (color.GetG8() << 8) | color.GetR8();
            }

            bool IsOpaque(AZ::Color color)
            {
                return color.GetA8() == 0xFF;
            }
        }

        const uint32_t VerticesPerPoint = 1;
        const uint32_t VerticesPerLine = 2;
        const uint32_t VerticesPerTriangle = 3;

        int32_t AuxGeomDrawQueue::AddViewProjOverride(const AZ::Matrix4x4& viewProj)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_buffersWriteLock);
            AuxGeomBufferData& buffer = m_buffers[m_currentBufferIndex];

            //the override matrix is pushed an array that persists until the frame is over, so that the matrix can be looked up later
            buffer.m_viewProjOverrides.push_back(viewProj);
            return aznumeric_cast<int32_t>(buffer.m_viewProjOverrides.size()) - 1;
        }

        int32_t AuxGeomDrawQueue::GetOrAdd2DViewProjOverride()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_buffersWriteLock);
            AuxGeomBufferData& buffer = m_buffers[m_currentBufferIndex];

            if (buffer.m_2DViewProjOverrideIndex == -1)
            {
                // matrix to convert 2d normalized screen coordinates (0.0-1.0 window lower-left based coordinates) to post projection space.
                static float s_Matrix4x4Floats[16] = {
                    2.0f,  0.0f, 0.0f, -1.0f,
                    0.0f, -2.0f, 0.0f, 1.0f,
                    0.0f,  0.0f, 1.0f, 0.0f,
                    0.0f,  0.0f, 0.0f, 1.0f};
                static Matrix4x4 s_proj2D = Matrix4x4::CreateFromRowMajorFloat16(s_Matrix4x4Floats);

                buffer.m_2DViewProjOverrideIndex = AddViewProjOverride(s_proj2D);
            }
            return buffer.m_2DViewProjOverrideIndex;
        }

        void AuxGeomDrawQueue::SetPointSize(float pointSize)
        {
            m_pointSize = pointSize;
        }

        float AuxGeomDrawQueue::GetPointSize()
        {
            return m_pointSize;
        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // dynamic draw functions

        void AuxGeomDrawQueue::DrawPoints(const AuxGeomDynamicDrawArguments& args)
        {
            AZ_Assert(args.m_colorCount == 1 || args.m_colorCount == args.m_vertCount, "DrawPolylines call with zero color entries");
            AZStd::function<AZ::u32(uint32_t)> packedColorFunction;
            bool isOpaque = true;
            if ( args.m_colorCount == 1 )
            {
                AZ::u32 packedColor = PackColor(args.m_colors[0]);
                packedColorFunction = [packedColor](uint32_t) { return packedColor; };
                isOpaque = IsOpaque(args.m_colors[0]);
            }
            else
            {
                packedColorFunction = [&args](uint32_t index) { return PackColor(args.m_colors[index]); };
                isOpaque = args.m_opacityType == OpacityType::Opaque;
            }
            DrawPrimitiveCommon(
                PrimitiveType_PointList, 
                VerticesPerPoint,
                args.m_vertCount, 
                args.m_verts,
                packedColorFunction,
                isOpaque,
                ConvertRPIDepthTestFlag(args.m_depthTest),
                ConvertRPIDepthWriteFlag(args.m_depthWrite), 
                FaceCull_None,
                args.m_size,
                args.m_viewProjectionOverrideIndex);
        }

        void AuxGeomDrawQueue::DrawLines(const AuxGeomDynamicDrawArguments& args)
        {
            AZ_Assert(args.m_vertCount >= 2, "DrawLines call with insufficient vertices");
            AZ_Assert(args.m_colorCount == 1 || args.m_colorCount == args.m_vertCount, "DrawLines call with zero color entries");
            AZStd::function<AZ::u32(uint32_t)> packedColorFunction;
            bool isOpaque = true;
            if ( args.m_colorCount == 1 )
            {
                AZ::u32 packedColor = PackColor(args.m_colors[0]);
                packedColorFunction = [packedColor](uint32_t) { return packedColor; };
                isOpaque = IsOpaque(args.m_colors[0]);
            }
            else
            {
                packedColorFunction = [&args](uint32_t index) { return PackColor(args.m_colors[index]); };
                isOpaque = args.m_opacityType == OpacityType::Opaque;
            }
            DrawPrimitiveCommon(
                PrimitiveType_LineList, 
                VerticesPerLine,
                args.m_vertCount, 
                args.m_verts,
                packedColorFunction,
                isOpaque,
                ConvertRPIDepthTestFlag(args.m_depthTest),
                ConvertRPIDepthWriteFlag(args.m_depthWrite), 
                FaceCull_None,
                args.m_size,
                args.m_viewProjectionOverrideIndex);
        }

        void AuxGeomDrawQueue::DrawLines(const AuxGeomDynamicIndexedDrawArguments& args)
        {
            AZ_Assert(args.m_vertCount >= 2, "DrawLines call with insufficient vertices");
            AZ_Assert(args.m_colorCount == 1 || args.m_colorCount == args.m_vertCount, "DrawLines call with zero color entries");
            AZStd::function<AZ::u32(uint32_t)> packedColorFunction;
            bool isOpaque = true;
            if ( args.m_colorCount == 1 )
            {
                AZ::u32 packedColor = PackColor(args.m_colors[0]);
                packedColorFunction = [packedColor](uint32_t) { return packedColor; };
                isOpaque = IsOpaque(args.m_colors[0]);
            }
            else
            {
                packedColorFunction = [&args](uint32_t index) { return PackColor(args.m_colors[index]); };
                isOpaque = args.m_opacityType == OpacityType::Opaque;
            }
            DrawPrimitiveWithSharedVerticesCommon(
                PrimitiveType_LineList, 
                VerticesPerLine,
                args.m_vertCount, 
                args.m_indexCount, 
                args.m_verts,
                packedColorFunction,
                [&args](uint32_t index) { return args.m_indices[index]; },
                isOpaque,
                ConvertRPIDepthTestFlag(args.m_depthTest),
                ConvertRPIDepthWriteFlag(args.m_depthWrite), 
                FaceCull_None,
                args.m_size,
                args.m_viewProjectionOverrideIndex);
        }

        void AuxGeomDrawQueue::DrawPolylines(const AuxGeomDynamicDrawArguments& args, PolylineEnd end)
        {
            AZ_Assert(args.m_vertCount >= 2, "DrawPolylines call with insufficient vertices");
            AZ_Assert(args.m_colorCount == 1 || args.m_colorCount == args.m_vertCount, "DrawPolylines call with zero color entries");
            AZStd::function<AZ::u32(uint32_t)> packedColorFunction;
            bool isOpaque = true;
            uint32_t indexCount = (end == PolylineEnd::Closed) ? args.m_vertCount * 2 : (args.m_vertCount - 1) * 2;
            if ( args.m_colorCount == 1 )
            {
                AZ::u32 packedColor = PackColor(args.m_colors[0]);
                packedColorFunction = [packedColor](uint32_t) { return packedColor; };
                isOpaque = IsOpaque(args.m_colors[0]);
            }
            else
            {
                packedColorFunction = [&args](uint32_t index) { return PackColor(args.m_colors[index]); };
                isOpaque = args.m_opacityType == OpacityType::Opaque;
            }

            DrawPrimitiveWithSharedVerticesCommon(
                PrimitiveType_LineList, 
                VerticesPerLine,
                args.m_vertCount, 
                indexCount, 
                args.m_verts,
                packedColorFunction,
                [&args](uint32_t index) { return ((index / 2) + (index % 2)) % args.m_vertCount; },
                isOpaque,
                ConvertRPIDepthTestFlag(args.m_depthTest),
                ConvertRPIDepthWriteFlag(args.m_depthWrite), 
                FaceCull_None,
                args.m_size,
                args.m_viewProjectionOverrideIndex);

        }

        void AuxGeomDrawQueue::DrawTriangles(const AuxGeomDynamicDrawArguments& args, FaceCullMode faceCull)
        {
            AZ_Assert(args.m_vertCount >= 3, "DrawTriangles call with insufficient vertices");
            AZ_Assert(args.m_colorCount == 1 || args.m_colorCount == args.m_vertCount, "DrawTriangles call with zero color entries");
            AZStd::function<AZ::u32(uint32_t)> packedColorFunction;
            bool isOpaque = true;
            if ( args.m_colorCount == 1 )
            {
                AZ::u32 packedColor = PackColor(args.m_colors[0]);
                packedColorFunction = [packedColor](uint32_t) { return packedColor; };
                isOpaque = IsOpaque(args.m_colors[0]);
            }
            else
            {
                packedColorFunction = [&args](uint32_t index) { return PackColor(args.m_colors[index]); };
                isOpaque = args.m_opacityType == OpacityType::Opaque;
            }

            DrawPrimitiveCommon(
                PrimitiveType_TriangleList, 
                VerticesPerTriangle,
                args.m_vertCount, 
                args.m_verts,
                packedColorFunction,
                isOpaque,
                ConvertRPIDepthTestFlag(args.m_depthTest),
                ConvertRPIDepthWriteFlag(args.m_depthWrite),
                ConvertRPIFaceCullFlag(faceCull),
                args.m_size,
                args.m_viewProjectionOverrideIndex);
        }

        void AuxGeomDrawQueue::DrawTriangles(const AuxGeomDynamicIndexedDrawArguments& args, FaceCullMode faceCull)
        {
            AZ_Assert(args.m_vertCount >= 3, "DrawTriangles call with insufficient vertices");
            AZ_Assert(args.m_colorCount == 1 || args.m_colorCount == args.m_vertCount, "DrawTriangles call with zero color entries");
            AZStd::function<AZ::u32(uint32_t)> packedColorFunction;
            bool isOpaque = true;
            if ( args.m_colorCount == 1 )
            {
                AZ::u32 packedColor = PackColor(args.m_colors[0]);
                packedColorFunction = [packedColor](uint32_t) { return packedColor; };
                isOpaque = IsOpaque(args.m_colors[0]);
            }
            else
            {
                packedColorFunction = [&args](uint32_t index) { return PackColor(args.m_colors[index]); };
                isOpaque = args.m_opacityType == OpacityType::Opaque;
            }
            DrawPrimitiveWithSharedVerticesCommon(
                PrimitiveType_TriangleList, 
                VerticesPerTriangle,
                args.m_vertCount, 
                args.m_indexCount, 
                args.m_verts,
                packedColorFunction,
                [&args](uint32_t index) { return args.m_indices[index]; },
                isOpaque,
                ConvertRPIDepthTestFlag(args.m_depthTest),
                ConvertRPIDepthWriteFlag(args.m_depthWrite),
                ConvertRPIFaceCullFlag(faceCull),
                args.m_size,
                args.m_viewProjectionOverrideIndex);
        }

        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Fixed shape draw functions

        void AuxGeomDrawQueue::DrawQuad(
            float width, 
            float height, 
            const AZ::Matrix3x4& transform, 
            const AZ::Color& color, 
            DrawStyle style, 
            DepthTest depthTest, 
            DepthWrite depthWrite, 
            FaceCullMode faceCull, 
            int32_t viewProjOverrideIndex) 
        {
            if (width <= 0.0f && height <= 0.0f)
            {
                return;
            }

            AZ::Matrix3x4 noScaleTransform = transform;
            AZ::Vector3 scale = noScaleTransform.ExtractScale();

            ShapeBufferEntry shape;
            shape.m_shapeType = ShapeType_Quad;
            shape.m_depthRead  = ConvertRPIDepthTestFlag(depthTest);
            shape.m_depthWrite = ConvertRPIDepthWriteFlag(depthWrite);
            shape.m_faceCullMode = ConvertRPIFaceCullFlag(faceCull);
            shape.m_color = color;
            shape.m_rotationMatrix = Matrix3x3::CreateFromMatrix3x4(noScaleTransform);
            shape.m_position = transform.GetTranslation();
            shape.m_scale = scale * Vector3(width, 1.0f, height);
            shape.m_pointSize = m_pointSize;
            shape.m_viewProjOverrideIndex = viewProjOverrideIndex;

            AddShape(style, shape);
        }

        Matrix3x3 CreateMatrix3x3FromDirection(const AZ::Vector3& direction)
        {
            Vector3 unitDirection(direction.GetNormalized());
            Vector3 unitOrthogonal(direction.GetOrthogonalVector().GetNormalized());
            Vector3 unitCross(unitOrthogonal.Cross(unitDirection));
            return Matrix3x3::CreateFromColumns(unitOrthogonal, unitDirection, unitCross);
        }

        void AuxGeomDrawQueue::DrawSphere(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex)
        {
            DrawSphereCommon(center, direction, radius, color, style, depthTest, depthWrite, faceCull, viewProjOverrideIndex, false);
        }

        void AuxGeomDrawQueue::DrawSphere(const AZ::Vector3& center, float radius, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex)
        {
            DrawSphereCommon(center, AZ::Vector3::CreateAxisZ(), radius, color, style, depthTest, depthWrite, faceCull, viewProjOverrideIndex, false);
        }

        void AuxGeomDrawQueue::DrawHemisphere(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex)
        {
            DrawSphereCommon(center, direction, radius, color, style, depthTest, depthWrite, faceCull, viewProjOverrideIndex, true);
        }

        void AuxGeomDrawQueue::DrawSphereCommon(
            const AZ::Vector3& center,
            const AZ::Vector3& direction,
            float radius, 
            const AZ::Color& color, 
            DrawStyle style, 
            DepthTest depthTest, 
            DepthWrite depthWrite, 
            FaceCullMode faceCull, 
            int32_t viewProjOverrideIndex,
            bool isHemisphere) 
        {
            if (radius <= 0.0f)
            {
                return;
            }

            ShapeBufferEntry shape;
            shape.m_shapeType = isHemisphere ? ShapeType_Hemisphere : ShapeType_Sphere;
            shape.m_depthRead  = ConvertRPIDepthTestFlag(depthTest);
            shape.m_depthWrite = ConvertRPIDepthWriteFlag(depthWrite);
            shape.m_faceCullMode = ConvertRPIFaceCullFlag(faceCull);
            shape.m_color = color;
            shape.m_rotationMatrix = CreateMatrix3x3FromDirection(direction);
            shape.m_position = center;
            shape.m_scale = AZ::Vector3(radius, radius, radius);
            shape.m_pointSize = m_pointSize;
            shape.m_viewProjOverrideIndex = viewProjOverrideIndex;

            AddShape(style, shape);
        }

        void AuxGeomDrawQueue::DrawDisk(
            const AZ::Vector3& center, 
            const AZ::Vector3& direction, 
            float radius, 
            const AZ::Color& color, 
            DrawStyle style, 
            DepthTest depthTest, 
            DepthWrite depthWrite, 
            FaceCullMode faceCull, 
            int32_t viewProjOverrideIndex) 
        {
            ShapeBufferEntry shape;
            shape.m_shapeType = ShapeType_Disk;
            shape.m_depthRead  = ConvertRPIDepthTestFlag(depthTest);
            shape.m_depthWrite = ConvertRPIDepthWriteFlag(depthWrite);
            shape.m_faceCullMode = ConvertRPIFaceCullFlag(faceCull);
            shape.m_color = color;

            // The disk mesh is created with the top of the disk pointing along the positive Y axis. This creates a
            // rotation so that the top of the disk will point along the given direction vector.
            shape.m_rotationMatrix = CreateMatrix3x3FromDirection(direction);
            shape.m_position = center;
            shape.m_scale = AZ::Vector3(radius, 1.0f, radius);
            shape.m_pointSize = m_pointSize;
            shape.m_viewProjOverrideIndex = viewProjOverrideIndex;

            AddShape(style, shape);
        }

        void AuxGeomDrawQueue::DrawCone(
            const AZ::Vector3& center, 
            const AZ::Vector3& direction, 
            float radius, 
            float height, 
            const AZ::Color& color, 
            DrawStyle style, 
            DepthTest depthTest, 
            DepthWrite depthWrite, 
            FaceCullMode faceCull, 
            int32_t viewProjOverrideIndex) 
        {
            if (radius <= 0.0f || height <= 0.0f)
            {
                return;
            }

            ShapeBufferEntry shape;
            shape.m_shapeType = ShapeType_Cone;
            shape.m_depthRead  = ConvertRPIDepthTestFlag(depthTest);
            shape.m_depthWrite = ConvertRPIDepthWriteFlag(depthWrite);
            shape.m_faceCullMode = ConvertRPIFaceCullFlag(faceCull);
            shape.m_color = color;

            shape.m_rotationMatrix = CreateMatrix3x3FromDirection(direction);
            shape.m_position = center;
            shape.m_scale = AZ::Vector3(radius, height, radius);
            shape.m_pointSize = m_pointSize;
            shape.m_viewProjOverrideIndex = viewProjOverrideIndex;

            AddShape(style, shape);
        }

        void AuxGeomDrawQueue::DrawCylinder(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, float height, const AZ::Color& color,
            DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex)
        {
            DrawCylinderCommon(center, direction, radius, height, color, style, depthTest, depthWrite, faceCull, viewProjOverrideIndex, true);
        }

        void AuxGeomDrawQueue::DrawCylinderNoEnds(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, float height, const AZ::Color& color,
            DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex)
        {
            DrawCylinderCommon(center, direction, radius, height, color, style, depthTest, depthWrite, faceCull, viewProjOverrideIndex, false);
        }

        void AuxGeomDrawQueue::DrawCylinderCommon(
            const AZ::Vector3& center,
            const AZ::Vector3& direction,
            float radius,
            float height,
            const AZ::Color& color,
            DrawStyle style,
            DepthTest depthTest,
            DepthWrite depthWrite,
            FaceCullMode faceCull,
            int32_t viewProjOverrideIndex,
            bool drawEnds)
        {
            if (radius <= 0.0f || height <= 0.0f)
            {
                return;
            }

            ShapeBufferEntry shape;
            shape.m_shapeType = drawEnds ? ShapeType_Cylinder : ShapeType_CylinderNoEnds;
            shape.m_depthRead = ConvertRPIDepthTestFlag(depthTest);
            shape.m_depthWrite = ConvertRPIDepthWriteFlag(depthWrite);
            shape.m_faceCullMode = ConvertRPIFaceCullFlag(faceCull);
            shape.m_color = color;

            // The cylinder mesh is created with the top end cap of the cylinder facing along the positive Y axis. This creates a
            // rotation so that the top face of the cylinder will face along the given direction vector.
            shape.m_rotationMatrix = CreateMatrix3x3FromDirection(direction);
            shape.m_position = center;
            shape.m_scale = AZ::Vector3(radius, height, radius);
            shape.m_pointSize = m_pointSize;
            shape.m_viewProjOverrideIndex = viewProjOverrideIndex;

            AddShape(style, shape);
        }

        void AuxGeomDrawQueue::DrawAabb(
            const AZ::Aabb& aabb, 
            const AZ::Color& color, 
            DrawStyle style, 
            DepthTest depthTest, 
            DepthWrite depthWrite, 
            FaceCullMode faceCull, 
            int32_t viewProjOverrideIndex) 
        {
            BoxBufferEntry box;
            box.m_color = color;
            box.m_depthRead  = ConvertRPIDepthTestFlag(depthTest);
            box.m_depthWrite = ConvertRPIDepthWriteFlag(depthWrite);
            box.m_faceCullMode = ConvertRPIFaceCullFlag(faceCull);
            box.m_position = aabb.GetCenter();
            box.m_scale = aabb.GetExtents();
            box.m_rotationMatrix = Matrix3x3::CreateIdentity();
            box.m_pointSize = m_pointSize;
            box.m_viewProjOverrideIndex = viewProjOverrideIndex;

            AddBox(style, box);
        }

        void AuxGeomDrawQueue::DrawAabb(
            const AZ::Aabb& aabb, 
            const AZ::Matrix3x4& matrix3x4,
            const AZ::Color& color, 
            DrawStyle style, 
            DepthTest depthTest, 
            DepthWrite depthWrite, 
            FaceCullMode faceCull, 
            int32_t viewProjOverrideIndex) 
        {
            AZ::Vector3 center = aabb.GetCenter();
            AZ::Vector3 extents = aabb.GetExtents();
            AZ::Matrix3x4 localMatrix3x4 = matrix3x4;

            BoxBufferEntry box;
            box.m_depthRead  = ConvertRPIDepthTestFlag(depthTest);
            box.m_depthWrite = ConvertRPIDepthWriteFlag(depthWrite);
            box.m_faceCullMode = ConvertRPIFaceCullFlag(faceCull);
            box.m_color = color;
            box.m_scale = localMatrix3x4.ExtractScale() * extents;
            box.m_position = matrix3x4 * center;
            box.m_rotationMatrix = Matrix3x3::CreateFromMatrix3x4(localMatrix3x4);
            box.m_pointSize = m_pointSize;
            box.m_viewProjOverrideIndex = viewProjOverrideIndex;

            AddBox(style, box);
        }

        void AuxGeomDrawQueue::DrawObb(
            const AZ::Obb& obb, 
            const AZ::Vector3& position, 
            const AZ::Color& color, 
            DrawStyle style, 
            DepthTest depthTest, 
            DepthWrite depthWrite, 
            FaceCullMode faceCull, 
            int32_t viewProjOverrideIndex) 
        {
            AZ::Vector3 center = obb.GetPosition();
            AZ::Vector3 extents(obb.GetHalfLengthX() * 2.0f, obb.GetHalfLengthY() * 2.0f, obb.GetHalfLengthZ() * 2.0f);

            BoxBufferEntry box;
            box.m_depthRead  = ConvertRPIDepthTestFlag(depthTest);
            box.m_depthWrite = ConvertRPIDepthWriteFlag(depthWrite);
            box.m_faceCullMode = ConvertRPIFaceCullFlag(faceCull);
            box.m_color = color;
            box.m_scale = extents;
            box.m_position = position + center;
            box.m_rotationMatrix = Matrix3x3::CreateFromColumns(obb.GetAxisX(), obb.GetAxisY(), obb.GetAxisZ());
            box.m_pointSize = m_pointSize;
            box.m_viewProjOverrideIndex = viewProjOverrideIndex;

            AddBox(style, box);
        }

        void AuxGeomDrawQueue::DrawObb(
            const AZ::Obb& obb, 
            const AZ::Matrix3x4& matrix3x4,
            const AZ::Color& color, 
            DrawStyle style, 
            DepthTest depthTest, 
            DepthWrite depthWrite, 
            FaceCullMode faceCull, 
            int32_t viewProjOverrideIndex) 
        {
            AZ::Vector3 center = obb.GetPosition();
            AZ::Vector3 extents(obb.GetHalfLengthX() * 2.0f, obb.GetHalfLengthY() * 2.0f, obb.GetHalfLengthZ() * 2.0f);
            AZ::Matrix3x4 localMatrix3x4 = matrix3x4;

            BoxBufferEntry box;
            box.m_depthRead  = ConvertRPIDepthTestFlag(depthTest);
            box.m_depthWrite = ConvertRPIDepthWriteFlag(depthWrite);
            box.m_faceCullMode = ConvertRPIFaceCullFlag(faceCull);
            box.m_color = color;
            box.m_scale = localMatrix3x4.ExtractScale() * extents;
            box.m_position = localMatrix3x4.GetTranslation() + center;
            box.m_rotationMatrix = Matrix3x3::CreateFromMatrix3x4(localMatrix3x4) * Matrix3x3::CreateFromColumns(obb.GetAxisX(), obb.GetAxisY(), obb.GetAxisZ());
            box.m_pointSize = m_pointSize;
            box.m_viewProjOverrideIndex = viewProjOverrideIndex;

            AddBox(style, box);
        }

        void AuxGeomDrawQueue::DrawFrustum(
            const Frustum& frustum,
            const Color& color,
            bool drawNormals,
            DrawStyle style,
            DepthTest depthTest,
            DepthWrite depthWrite,
            FaceCullMode faceCull,
            int32_t viewProjOverrideIndex)
        {

            Frustum::CornerVertexArray corners;
            bool validFrustum = frustum.GetCorners(corners);

            if (validFrustum)
            {
                // This helps cut down on clutter below. Replace with a using-enum-declaration when c++20 support is required.
                enum CornerIndices
                {
                    NearTopLeft = Frustum::CornerIndices::NearTopLeft,
                    NearTopRight = Frustum::CornerIndices::NearTopRight,
                    NearBottomLeft = Frustum::CornerIndices::NearBottomLeft,
                    NearBottomRight = Frustum::CornerIndices::NearBottomRight,
                    FarTopLeft = Frustum::CornerIndices::FarTopLeft,
                    FarTopRight = Frustum::CornerIndices::FarTopRight,
                    FarBottomLeft = Frustum::CornerIndices::FarBottomLeft,
                    FarBottomRight = Frustum::CornerIndices::FarBottomRight,
                };

                RPI::AuxGeomDraw::AuxGeomDynamicIndexedDrawArguments drawArgs;
                drawArgs.m_verts = corners.data();
                drawArgs.m_vertCount = 8;
                drawArgs.m_colors = &color;
                drawArgs.m_colorCount = 1;
                drawArgs.m_depthTest = depthTest;
                drawArgs.m_depthWrite = depthWrite;
                drawArgs.m_viewProjectionOverrideIndex = viewProjOverrideIndex;

                if (style == DrawStyle::Point)
                {
                    DrawPoints(drawArgs);
                }
                else
                {
                    // Always draw lines if draw style isn't Point.
                    uint32_t lineIndices[24]{
                        //near plane
                        NearTopLeft, NearTopRight,
                        NearTopRight, NearBottomRight,
                        NearBottomRight, NearBottomLeft,
                        NearBottomLeft, NearTopLeft,

                        //Far plane
                        FarTopLeft, FarTopRight,
                        FarTopRight, FarBottomRight,
                        FarBottomRight, FarBottomLeft,
                        FarBottomLeft, FarTopLeft,

                        //Near-to-Far connecting lines
                        NearTopLeft, FarTopLeft,
                        NearTopRight, FarTopRight,
                        NearBottomLeft, FarBottomLeft,
                        NearBottomRight, FarBottomRight,
                    };

                    drawArgs.m_indices = lineIndices;
                    drawArgs.m_indexCount = 24;
                    DrawLines(drawArgs);

                    if (style == DrawStyle::Solid || style == DrawStyle::Shaded)
                    {
                        // DrawTriangles doesn't support shaded drawing, so we can't support it here either.
                        AZ_WarningOnce("AuxGeomDrawQueue", style != DrawStyle::Shaded, "Cannot draw frustum with Shaded DrawStyle, using Solid instead.");

                        uint32_t triangleIndices[36]{
                            //near
                            NearBottomLeft, NearTopLeft, NearTopRight,
                            NearBottomLeft, NearTopRight, NearBottomRight,

                            //far
                            FarBottomRight, FarTopRight, FarTopLeft,
                            FarBottomRight, FarTopLeft, FarBottomLeft,

                            //left
                            NearTopLeft, NearBottomLeft, FarBottomLeft,
                            NearTopLeft, FarBottomLeft, FarTopLeft,

                            //right
                            NearBottomRight, NearTopRight, FarTopRight,
                            NearBottomRight, FarTopRight, FarBottomRight,

                            //bottom
                            FarBottomLeft, NearBottomLeft, NearBottomRight,
                            FarBottomLeft, NearBottomRight, FarBottomRight,

                            //top
                            NearTopLeft, FarTopLeft, FarTopRight,
                            NearTopLeft, FarTopRight, NearTopRight,
                        };

                        Color transparentColor(color.GetR(), color.GetG(), color.GetB(), color.GetA() * 0.3f);
                        drawArgs.m_indices = triangleIndices;
                        drawArgs.m_indexCount = 36;
                        drawArgs.m_colors = &transparentColor;
                        DrawTriangles(drawArgs, faceCull);
                    }
                }

                if (drawNormals)
                {
                    Vector3 planeNormals[] =
                    {
                        //near
                        0.25f * (corners[NearBottomLeft] + corners[NearBottomRight] + corners[NearTopLeft] + corners[NearTopRight]),
                        0.25f * (corners[NearBottomLeft] + corners[NearBottomRight] + corners[NearTopLeft] + corners[NearTopRight]) + frustum.GetPlane(Frustum::PlaneId::Near).GetNormal(),

                        //far
                        0.25f * (corners[FarBottomLeft] + corners[FarBottomRight] + corners[FarTopLeft] + corners[FarTopRight]),
                        0.25f * (corners[FarBottomLeft] + corners[FarBottomRight] + corners[FarTopLeft] + corners[FarTopRight]) + frustum.GetPlane(Frustum::PlaneId::Far).GetNormal(),

                        //left
                        0.5f * (corners[NearBottomLeft] + corners[NearTopLeft]),
                        0.5f * (corners[NearBottomLeft] + corners[NearTopLeft]) + frustum.GetPlane(Frustum::PlaneId::Left).GetNormal(),

                        //right
                        0.5f * (corners[NearBottomRight] + corners[NearTopRight]),
                        0.5f * (corners[NearBottomRight] + corners[NearTopRight]) + frustum.GetPlane(Frustum::PlaneId::Right).GetNormal(),

                        //bottom
                        0.5f * (corners[NearBottomLeft] + corners[NearBottomRight]),
                        0.5f * (corners[NearBottomLeft] + corners[NearBottomRight]) + frustum.GetPlane(Frustum::PlaneId::Bottom).GetNormal(),

                        //top
                        0.5f * (corners[NearTopLeft] + corners[NearTopRight]),
                        0.5f * (corners[NearTopLeft] + corners[NearTopRight]) + frustum.GetPlane(Frustum::PlaneId::Top).GetNormal(),
                    };

                    Color planeNormalColors[] =
                    {
                        Colors::Red, Colors::Red,       //near
                        Colors::Green, Colors::Green,   //far
                        Colors::Blue, Colors::Blue,     //left
                        Colors::Orange, Colors::Orange, //right
                        Colors::Pink, Colors::Pink,     //bottom
                        Colors::MediumPurple, Colors::MediumPurple, //top
                    };

                    RPI::AuxGeomDraw::AuxGeomDynamicDrawArguments planeNormalLineArgs;
                    planeNormalLineArgs.m_verts = planeNormals;
                    planeNormalLineArgs.m_vertCount = 12;
                    planeNormalLineArgs.m_colors = planeNormalColors;
                    planeNormalLineArgs.m_colorCount = planeNormalLineArgs.m_vertCount;
                    planeNormalLineArgs.m_depthTest = depthTest;
                    DrawLines(planeNormalLineArgs);
                }
            }
            else
            {
                AZ_Assert(false, "invalid frustum, cannot draw");
            }
        }

        AuxGeomBufferData* AuxGeomDrawQueue::Commit()
        {
            AZ_PROFILE_SCOPE(AzRender, "AuxGeomDrawQueue: Commit");
            // get a mutually exclusive lock and then switch to the next buffer, returning a pointer to the current buffer (before the switch)

            // grab the lock
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_buffersWriteLock);

            // get a pointer to the buffer we have been filling
            AuxGeomBufferData* filledBufferData = &m_buffers[m_currentBufferIndex];

            // switch the buffer for future requests to the other buffer
            m_currentBufferIndex = (m_currentBufferIndex + 1) % NumBuffers;

            // Clear the buffers that new requests will get added to
            ClearCurrentBufferData();

            return filledBufferData;
        }

        void AuxGeomDrawQueue::ClearCurrentBufferData()
        {
            AZ_PROFILE_SCOPE(AzRender, "AuxGeomDrawQueue: ClearCurrentBufferData");
            // no need for mutex here, this function is only called from a function holding a lock
            AuxGeomBufferData& data = m_buffers[m_currentBufferIndex];

            DynamicPrimitiveData& primitives = data.m_primitiveData;
            primitives.m_primitiveBuffer.clear();
            primitives.m_vertexBuffer.clear();
            primitives.m_indexBuffer.clear();

            for (int drawStyle = 0; drawStyle < DrawStyle_Count; ++drawStyle)
            {
                data.m_opaqueShapes[drawStyle].clear();
                data.m_translucentShapes[drawStyle].clear();

                data.m_opaqueBoxes[drawStyle].clear();
                data.m_translucentBoxes[drawStyle].clear();
            }

            data.m_viewProjOverrides.clear();
            data.m_2DViewProjOverrideIndex = -1;
        }

        bool AuxGeomDrawQueue::ShouldBatchDraw(
            DynamicPrimitiveData& primBuffer, 
            AuxGeomPrimitiveType primType, 
            AuxGeomBlendMode blendMode, 
            AuxGeomDepthReadType depthRead, 
            AuxGeomDepthWriteType depthWrite, 
            AuxGeomFaceCullMode faceCull, 
            u8 width,
            int32_t viewProjOverrideIndex)
        {
            if (!primBuffer.m_primitiveBuffer.size())
            {
                return false;
            }
            auto& primitive = primBuffer.m_primitiveBuffer.back();
            if (    primitive.m_primitiveType == primType && 
                    blendMode == BlendMode_Off &&
                    primitive.m_blendMode == BlendMode_Off &&
                    primitive.m_depthReadType == depthRead && 
                    primitive.m_depthWriteType == depthWrite && 
                    primitive.m_faceCullMode == faceCull &&
                    primitive.m_width == width &&
                    primitive.m_viewProjOverrideIndex == viewProjOverrideIndex)
            {
                return true;
            }
            return false;
        }

        void AuxGeomDrawQueue::DrawPrimitiveCommon(
            AuxGeomPrimitiveType primitiveType,
            [[maybe_unused]] uint32_t verticesPerPrimitiveType,
            uint32_t vertexCount,
            const AZ::Vector3* points,
            AZStd::function<AZ::u32(uint32_t)> packedColorFunction,
            bool isOpaque,
            AuxGeomDepthReadType depthRead,
            AuxGeomDepthWriteType depthWrite,
            AuxGeomFaceCullMode faceCull,
            AZ::u8 width,
            int32_t viewProjOverrideIndex)
        {
            // grab a mutex lock for the rest of this function so that a commit cannot happen during it and
            // other threads can't add geometry during it
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_buffersWriteLock);
            AuxGeomBufferData& buffer = m_buffers[m_currentBufferIndex];

            // We have a separate PrimitiveBufferEntry for each AuxGeomDraw call
            DynamicPrimitiveData& primBuffer = buffer.m_primitiveData;

            AuxGeomIndex vertexOffset = aznumeric_cast<AuxGeomIndex>(primBuffer.m_vertexBuffer.size());
            AuxGeomIndex indexOffset = aznumeric_cast<AuxGeomIndex>(primBuffer.m_indexBuffer.size());
            const size_t vertexCountTotal = aznumeric_cast<size_t>(vertexOffset) + vertexCount;

            if (vertexCountTotal > MaxDynamicVertexCount)
            {
                AZ_WarningOnce("AuxGeom", false, "Draw function ignored, would exceed maximum allowed index of %d", MaxDynamicVertexCount);
                return;
            }

            AZ::Vector3 center(0.0f, 0.0f, 0.0f);
            for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                AZ::u32 packedColor = packedColorFunction(vertexIndex);
                const AZ::Vector3& vertex = points[vertexIndex];
                primBuffer.m_vertexBuffer.push_back(AuxGeomDynamicVertex(vertex, packedColor));
                primBuffer.m_indexBuffer.push_back(vertexOffset + vertexIndex);

                center += vertex;
            }
            center /= static_cast<float>(vertexCount);

            AuxGeomBlendMode blendMode = isOpaque ? BlendMode_Off : BlendMode_Alpha;
            if (ShouldBatchDraw(primBuffer, primitiveType, blendMode, depthRead, depthWrite, faceCull, width, viewProjOverrideIndex))
            {
                auto& primitive = primBuffer.m_primitiveBuffer.back();
                primitive.m_indexCount += vertexCount;
            }
            else
            {
                auto& primitive = primBuffer.m_primitiveBuffer.emplace_back();
                primitive.m_primitiveType = primitiveType;
                primitive.m_depthReadType = depthRead;
                primitive.m_depthWriteType = depthWrite;
                primitive.m_blendMode = blendMode;
                primitive.m_faceCullMode = faceCull;
                primitive.m_width = width;
                primitive.m_indexOffset = indexOffset;
                primitive.m_indexCount = vertexCount;
                primitive.m_center = center;
                primitive.m_viewProjOverrideIndex = viewProjOverrideIndex;
            }
        }

        void AuxGeomDrawQueue::DrawPrimitiveWithSharedVerticesCommon(
            AuxGeomPrimitiveType primitiveType,
            [[maybe_unused]] uint32_t verticesPerPrimitiveType,
            uint32_t vertexCount,
            uint32_t indexCount,
            const AZ::Vector3* points,
            AZStd::function<AZ::u32(uint32_t)> packedColorFunction,
            AZStd::function<AuxGeomIndex(uint32_t)> indexFunction,
            bool isOpaque,
            AuxGeomDepthReadType depthRead,
            AuxGeomDepthWriteType depthWrite,
            AuxGeomFaceCullMode faceCull,
            AZ::u8 width,
            int32_t viewProjOverrideIndex)
        {
            AZ_Assert(indexCount >= verticesPerPrimitiveType && (indexCount % verticesPerPrimitiveType == 0),
                "Index count must be at least %d and must be a multiple of %d",
                verticesPerPrimitiveType, verticesPerPrimitiveType);

            // grab a mutex lock for the rest of this function so that a commit cannot happen during it and
            // other threads can't add geometry during it
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_buffersWriteLock);
            AuxGeomBufferData& buffer = m_buffers[m_currentBufferIndex];

            // We have a separate PrimitiveBufferEntry for each AuxGeomDraw call
            DynamicPrimitiveData& primBuffer = buffer.m_primitiveData;

            AuxGeomIndex vertexOffset = aznumeric_cast<AuxGeomIndex>(primBuffer.m_vertexBuffer.size());
            AuxGeomIndex indexOffset = aznumeric_cast<AuxGeomIndex>(primBuffer.m_indexBuffer.size());
            const size_t vertexCountTotal = aznumeric_cast<size_t>(vertexOffset) + vertexCount;

            if (vertexCountTotal > MaxDynamicVertexCount)
            {
                AZ_WarningOnce("AuxGeom", false, "Draw function ignored, would exceed maximum allowed index of %d", MaxDynamicVertexCount);
                return;
            }

            AZ::Vector3 center(0.0f, 0.0f, 0.0f);
            for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                AZ::u32 packedColor = packedColorFunction(vertexIndex);
                const AZ::Vector3& vertex = points[vertexIndex];
                primBuffer.m_vertexBuffer.push_back(AuxGeomDynamicVertex(vertex, packedColor));

                center += vertex;
            }
            center /= aznumeric_cast<float>(vertexCount);

            for (uint32_t index = 0; index < indexCount; ++index)
            {
                primBuffer.m_indexBuffer.push_back(vertexOffset + indexFunction(index));
            }

            AuxGeomBlendMode blendMode = isOpaque ? BlendMode_Off : BlendMode_Alpha;
            if (ShouldBatchDraw(primBuffer, primitiveType, blendMode, depthRead, depthWrite, faceCull, width, viewProjOverrideIndex))
            {
                auto& primitive = primBuffer.m_primitiveBuffer.back();
                primitive.m_indexCount += indexCount;
            }
            else
            {
                auto& primitive = primBuffer.m_primitiveBuffer.emplace_back();
                primitive.m_primitiveType = primitiveType;
                primitive.m_depthReadType = depthRead;
                primitive.m_depthWriteType = depthWrite;
                primitive.m_blendMode = blendMode;
                primitive.m_faceCullMode = faceCull;
                primitive.m_width = width;
                primitive.m_indexOffset = indexOffset;
                primitive.m_indexCount = indexCount;
                primitive.m_center = center;
                primitive.m_viewProjOverrideIndex = viewProjOverrideIndex;
            }
        }

        void AuxGeomDrawQueue::AddShape(DrawStyle style, const ShapeBufferEntry& shape)
        {
            AuxGeomDrawStyle drawStyle = ConvertRPIDrawStyle(style);

            // grab a mutex lock for the rest of this function so that a commit cannot happen during it and
            // other threads can't add geometry during it
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_buffersWriteLock);
            AuxGeomBufferData& buffer = m_buffers[m_currentBufferIndex];

            if (IsOpaque(shape.m_color))
            {
                buffer.m_opaqueShapes[drawStyle].push_back(shape);
            }
            else
            {
                buffer.m_translucentShapes[drawStyle].push_back(shape);
            }
        }

        void AuxGeomDrawQueue::AddBox(DrawStyle style, BoxBufferEntry& box)
        {
            AuxGeomDrawStyle drawStyle = ConvertRPIDrawStyle(style);

            // grab a mutex lock for the rest of this function so that a commit cannot happen during it and
            // other threads can't add geometry during it
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_buffersWriteLock);
            AuxGeomBufferData& buffer = m_buffers[m_currentBufferIndex];

            if (IsOpaque(box.m_color))
            {
                buffer.m_opaqueBoxes[drawStyle].push_back(box);
            }
            else
            {
                buffer.m_translucentBoxes[drawStyle].push_back(box);
            }
        }

    } // namespace Render
} // namespace AZ
