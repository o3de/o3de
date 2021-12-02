/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Transform.h>

#include <Atom/RPI.Public/Base.h>

#include "AuxGeomBase.h"

namespace AZ
{
    namespace RPI
    {
        class Scene;
        class View;
    }

    namespace Render
    {

        /**
         * Class that stores up AuxGeom draw requests for one RPI scene.
         * This acts somewhat like a render proxy in that it stores data that is consumed by the feature processor.
         */
        class AuxGeomDrawQueue final
            : public RPI::AuxGeomDraw
        {
        public: // functions

            AZ_CLASS_ALLOCATOR(AuxGeomDrawQueue, AZ::SystemAllocator, 0);

            AuxGeomDrawQueue() = default;
            ~AuxGeomDrawQueue() override = default;

            // RPI::AuxGeomDraw
            int32_t AddViewProjOverride(const AZ::Matrix4x4& viewProj) override;
            int32_t GetOrAdd2DViewProjOverride() override;

            void SetPointSize(float pointSize) override;
            float GetPointSize() override;

            // dynamic polygon draws
            void DrawPoints(const AuxGeomDynamicDrawArguments& args) override;
            void DrawLines(const AuxGeomDynamicDrawArguments& args) override;
            void DrawLines(const AuxGeomDynamicIndexedDrawArguments& args) override;
            void DrawPolylines(const AuxGeomDynamicDrawArguments& args, PolylineEnd end = PolylineEnd::Open) override;
            void DrawTriangles(const AuxGeomDynamicDrawArguments& args, FaceCullMode faceCull = FaceCullMode::None) override;
            void DrawTriangles(const AuxGeomDynamicIndexedDrawArguments& args, FaceCullMode faceCull = FaceCullMode::None) override;

            // Fixed shape draws
            void DrawQuad(float width, float height, const AZ::Matrix3x4& transform, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;
            void DrawSphere(const AZ::Vector3& center, float radius, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;
            void DrawSphere(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;
            void DrawHemisphere(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;
            void DrawDisk(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;
            void DrawCone(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, float height, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;
            void DrawCylinder(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, float height, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;
            void DrawCylinderNoEnds(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, float height, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;
            void DrawAabb(const AZ::Aabb& aabb, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;
            void DrawAabb(const AZ::Aabb& aabb, const AZ::Matrix3x4& transform, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;
            void DrawObb(const AZ::Obb& obb, const AZ::Vector3& position, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;
            void DrawObb(const AZ::Obb& obb, const AZ::Matrix3x4& transform, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex) override;

            //! Switch clients of AuxGeom to using a different buffer and return the filled buffer for processing
            AuxGeomBufferData* Commit();

        private: // functions

            void DrawCylinderCommon(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, float height, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex, bool drawEnds);
            void DrawSphereCommon(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, const AZ::Color& color, DrawStyle style, DepthTest depthTest, DepthWrite depthWrite, FaceCullMode faceCull, int32_t viewProjOverrideIndex, bool isHemisphere);

            //! Clear the current buffers
            void ClearCurrentBufferData();

            bool ShouldBatchDraw(
                DynamicPrimitiveData& primBuffer, 
                AuxGeomPrimitiveType primType, 
                AuxGeomBlendMode blendMode, 
                AuxGeomDepthReadType depthRead, 
                AuxGeomDepthWriteType depthWrite, 
                AuxGeomFaceCullMode faceCull, 
                u8 width,
                int32_t viewProjOverrideIndex);


            void DrawPrimitiveCommon(
                AuxGeomPrimitiveType primitiveType,
                uint32_t verticesPerPrimitiveType,
                uint32_t vertexCount,
                const AZ::Vector3* points,
                AZStd::function<AZ::u32(uint32_t)> packedColorFunction,
                bool isOpaque,
                AuxGeomDepthReadType depthRead,
                AuxGeomDepthWriteType depthWrite,
                AuxGeomFaceCullMode faceCull,
                AZ::u8 width,
                int32_t viewProjOverrideIndex);

            void DrawPrimitiveWithSharedVerticesCommon(
                AuxGeomPrimitiveType primitiveType,
                uint32_t verticesPerPrimitiveType,
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
                int32_t viewProjOverrideIndex);

            void AddShape(DrawStyle style, const ShapeBufferEntry& shape);
            void AddBox(DrawStyle style, BoxBufferEntry& box);

        private: // data

            // We just toggle back and forth between two buffers, one being filled while the other is being processed
            // by the FeatureProcessor.
            static const int NumBuffers = 2;
            AuxGeomBufferData m_buffers[NumBuffers];
            int m_currentBufferIndex = 0;
            float m_pointSize = 3.0f;

            AZStd::recursive_mutex m_buffersWriteLock;
        };

    } // namespace Render
} // namespace AZ
