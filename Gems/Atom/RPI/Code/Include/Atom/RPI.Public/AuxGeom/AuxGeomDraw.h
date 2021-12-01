/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Matrix3x4.h>

#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace RPI
    {
        using AuxGeomDrawPtr = AZStd::shared_ptr<class AuxGeomDraw>;

       //! The drawing interface of the AuxGeom system, which is used for drawing Auxiliary Geometry, both for debug and things like editor manipulators.
       //! An object implementing this interface can have internal state indicating which scene it is drawing to and whether it is immediate mode or persistent.
       //! To get a pointer to an AuxGeomDraw interface use a helper function like AuxGeomFeatureProcessorInterface::GetDrawQueueForScene.
       //! 
       //! Translucency: If a geometry is considered translucent that then it will be depth sorted. Also translucent
       //! geometry will not be combined with any other geometry to reduce draw calls. For functions where a single color is provided, the given color
       //! is used to determine if the geometry is opaque or translucent. If multiple colors are provided then a separate parameter of type OpacityType
       //! is used to indicate if the geometry is opaque or translucent.
        class AuxGeomDraw
        {
        public: //types

            enum class DrawStyle : uint8_t
            {
                Point,      //!< render each vertex as a point
                Line,       //!< Wireframe geometry
                Solid,      //!< Solid geometry
                Shaded      //!< Solid geometry with fake lighting
            };

            enum class DepthTest : uint8_t
            {
                On,
                Off,
            };

            enum class DepthWrite : uint8_t
            {
                On,
                Off,
            };

            enum class FaceCullMode : uint8_t
            {
                None, // Front and Back are drawn
                Front, // Front facing triangles are culled
                Back, // Back facing triangles are culled
            };

            enum class PolylineEnd : uint8_t
            {
                Open, // End open, no line joining first and last vert
                Closed, // End closed, add a line joining the first and last vert
            };

            //! Used to indicate whether geometry should be considered opaque or translucent.
            //! This is only used when more than one color is provided. If there is a single color
            //! then the alpha is used to determine whether it is opaque.
            enum class OpacityType : uint8_t
            {
                Opaque,
                Translucent
            };

            //! Common arguments for free polygon (point, line, Triangle) draws.
            struct AuxGeomDynamicDrawArguments
            {
                const AZ::Vector3* m_verts = nullptr; //!< An array of points, 1 for each vertex.
                uint32_t m_vertCount = 0; //!< The number of vertices.
                const AZ::Color* m_colors; //!< An array of colors, must have either vertCount entries or 1 entry.
                uint32_t m_colorCount = 0; //!< The number of colors, must equal 1 or vertCount.
                AZ::u8 m_size = 1;         //!< size of points or width of lines in pixels - unsupported at the moment
                OpacityType m_opacityType = OpacityType::Opaque; //!< Indicates whether the triangles should be drawn opaque or translucent.
                DepthTest m_depthTest = DepthTest::On; //!< If depth testing should be enabled
                DepthWrite m_depthWrite = DepthWrite::On; //!< If depth writing should be enabled
                int32_t m_viewProjectionOverrideIndex = -1; //!< Index of the view projection override (2d or orthographic) for this draw, -1 if no override.
            };

            //! Common arguments for free polygon (point, line, Triangle) indexed draws.
            struct AuxGeomDynamicIndexedDrawArguments : public AuxGeomDynamicDrawArguments
            {
                const uint32_t* m_indices = nullptr; // An array of indices into the verts array.
                uint32_t m_indexCount = 0; // The number of indices, must be at least equal to the number of verts for the draws polygon type or an integer multiple of it.
            };

        public: // functions
            virtual ~AuxGeomDraw() = default;

            /////////////////////////////////////////////////////////////////////////////////////////////
            // manual override of the view projection transform
            virtual int32_t AddViewProjOverride(const AZ::Matrix4x4& viewProj) = 0;
            virtual int32_t GetOrAdd2DViewProjOverride() = 0;
            /////////////////////////////////////////////////////////////////////////////////////////////
            // control point size for fixed shapes
            virtual void SetPointSize(float pointSize) = 0;
            virtual float GetPointSize() = 0;

            /////////////////////////////////////////////////////////////////////////////////////////////
            // dynamic draw functions

            virtual void DrawPoints(const AuxGeomDynamicDrawArguments& args) = 0;
            virtual void DrawLines(const AuxGeomDynamicDrawArguments& args) = 0;
            virtual void DrawLines(const AuxGeomDynamicIndexedDrawArguments& args) = 0;

            //! @param closed        If true then a line will be drawn from the last point to the first.
            virtual void DrawPolylines(const AuxGeomDynamicDrawArguments& args, PolylineEnd end = PolylineEnd::Open) = 0;

            //! @param faceCull      Which (if any) facing triangles should be culled
            virtual void DrawTriangles(const AuxGeomDynamicDrawArguments& args, FaceCullMode faceCull = FaceCullMode::None) = 0;
            virtual void DrawTriangles(const AuxGeomDynamicIndexedDrawArguments& args, FaceCullMode faceCull = FaceCullMode::None) = 0;

            //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            // Fixed shape draw functions

            //! Draw a quad.
            //! @param width         The width of the quad.
            //! @param height        The height of the quad.
            //! @param transform     The world space transform for the quad.
            //! @param color         The color to draw the quad.
            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawQuad(float width, float height, const AZ::Matrix3x4& transform, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;

            //! Draw a sphere.
            //! @param center        The center of the sphere.
            //! @param radius        The radius.
            //! @param color         The color to draw the sphere.
            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawSphere( const AZ::Vector3& center, float radius, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;

            //! Draw a sphere.
            //! @param center        The center of the sphere.
            //! @param direction     The direction vector. The Pole of the hemisphere will point along this vector.
            //! @param radius        The radius.
            //! @param color         The color to draw the sphere.
            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawSphere(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;

            //! Draw a hemisphere.
            //! @param center        The center of the sphere.
            //! @param direction     The direction vector. The Pole of the hemisphere will point along this vector.
            //! @param radius        The radius.
            //! @param color         The color to draw the sphere.
            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawHemisphere( const AZ::Vector3& center, const AZ::Vector3& direction, float radius, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;

            //! Draw a disk.
            //! @param center        The center of the disk.
            //! @param direction     The direction vector. The disk will be orthogonal this vector.
            //! @param radius        The radius of the disk.
            //! @param color         The color to draw the disk.
            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawDisk(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;

            //! Draw a cone.
            //! @param center        The center of the base circle.
            //! @param direction     The direction vector. The tip of the cone will point along this vector.
            //! @param radius        The radius.
            //! @param height        The height of the cone (the distance from the base center to the tip).
            //! @param color         The color to draw the cone.
            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawCone(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, float height, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;

            //! Draw a cylinder (with flat disks on the end).
            //! @param center        The center of the base circle.
            //! @param direction     The direction vector. The top end cap of the cylinder will face along this vector.
            //! @param radius        The radius.
            //! @param height        The height of the cylinder.
            //! @param color         The color to draw the cylinder.
            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawCylinder(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, float height, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;

            //! Draw a cylinder without flat disk on the end.
            //! @param center        The center of the base circle.
            //! @param direction     The direction vector. The top end cap of the cylinder will face along this vector.
            //! @param radius        The radius.
            //! @param height        The height of the cylinder.
            //! @param color         The color to draw the cylinder.
            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawCylinderNoEnds(const AZ::Vector3& center, const AZ::Vector3& direction, float radius, float height, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;

            //! Draw an axis-aligned bounding box with no transform.
            //! @param aabb          The AABB (typically the bounding box of a set of world space points).
            //! @param color         The color to draw the box.
            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawAabb(const AZ::Aabb& aabb, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;

            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawAabb(const AZ::Aabb& aabb, const AZ::Matrix3x4& transform, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;

            //! Draw an oriented bounding box with a given position.
            //! @param obb           The OBB (typically an oriented bounding box in model space).
            //! @param position      The position to translate the origin of the OBB to.
            //! @param color         The color to draw the box.
            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawObb(const AZ::Obb& obb, const AZ::Vector3& position, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;

            //! @param style         The draw style (point, wireframe, solid, shaded etc).
            //! @param depthTest     If depth testing should be enabled
            //! @param depthWrite    If depth writing should be enabled
            //! @param faceCull      Which (if any) facing triangles should be culled
            //! @param viewProjOverrideIndex Which view projection override entry to use, -1 if unused
            virtual void DrawObb(const AZ::Obb& obb, const AZ::Matrix3x4& transform, const AZ::Color& color, DrawStyle style = DrawStyle::Shaded, DepthTest depthTest = DepthTest::On, DepthWrite depthWrite = DepthWrite::On, FaceCullMode faceCull = FaceCullMode::Back, int32_t viewProjOverrideIndex = -1) = 0;
        };

    } // namespace RPI
} // namespace AZ
