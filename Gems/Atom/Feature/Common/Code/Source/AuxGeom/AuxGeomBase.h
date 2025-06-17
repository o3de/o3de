/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Color.h>

#include <Atom/RHI/GeometryView.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>

namespace AZ
{
    namespace Render
    {
        /**
         * Define common types used for data throughout the AuxGeom system
         */

        //! Index type used for indexed draws
        using AuxGeomIndex = u32;

        //! Colors are packed into one byte per color
        using AuxGeomColor = AZ::u32;

        /**
         * Positions are stored in this struct.
         * We use this struct rather than AZ::Vector3 because an AZStd::vector of AZ::Vector3 has a stride of 16 bytes because of
         * alignment constraints.
         */
        struct AuxGeomPosition
        {
            AuxGeomPosition(const AZ::Vector3& pos)
            {
                m_x = pos.GetX();
                m_y = pos.GetY();
                m_z = pos.GetZ();
            };

            AuxGeomPosition(float x, float y, float z)
            {
                m_x = x;
                m_y = y;
                m_z = z;
            };

            float m_x;
            float m_y;
            float m_z;
        };

        struct AuxGeomDynamicVertex
        {
            AuxGeomDynamicVertex(const AZ::Vector3& pos, AuxGeomColor color)
                : m_position(pos)
                , m_color(color)
            {
            };

            AuxGeomPosition m_position;
            AuxGeomColor    m_color;
        };

        //! Used for dynamic primitives
        //! This is not a scoped enum because we want to be able to use its values as array indices
        enum AuxGeomPrimitiveType
        {
            PrimitiveType_PointList,
            PrimitiveType_LineList,
            PrimitiveType_TriangleList,

            PrimitiveType_Count
        };


        enum AuxGeomDepthReadType
        {
            DepthRead_On,
            DepthRead_Off,

            DepthRead_Count
        };


        enum AuxGeomDepthWriteType
        {
            DepthWrite_On,
            DepthWrite_Off,

            DepthWrite_Count,
        };

        enum AuxGeomBlendMode
        {
            BlendMode_Alpha,
            BlendMode_Off,

            BlendMode_Count
        };

        enum AuxGeomFaceCullMode
        {
            FaceCull_None,
            FaceCull_Front,
            FaceCull_Back,

            FaceCull_Count
        };

        //! Each dynamic primitive drawn through the AuxGeom draw interface is stored in the scene data as an instance of this struct
        struct PrimitiveBufferEntry
        {
            RHI::GeometryView           m_geometryView;
            AZ::Vector3                 m_center;       // used for depth sorting blended draws
            AuxGeomPrimitiveType        m_primitiveType;
            AuxGeomDepthReadType        m_depthReadType;
            AuxGeomDepthWriteType       m_depthWriteType;
            AuxGeomFaceCullMode         m_faceCullMode;
            AuxGeomBlendMode            m_blendMode;
            AuxGeomIndex                m_indexOffset;  // The index into the shared index buffer for all primitives
            AuxGeomIndex                m_indexCount;   // The number of indices (a primitive can be a list of lines rather than one line for example)
            AZ::u8                      m_width;        // for points and lines
        
            //if < 0, then will render using the View's view and proj matrices, otherwise this indexes into AuxGeomBufferData::m_viewProjOverrides
            int32_t                 m_viewProjOverrideIndex = -1;
        };

        //! Internally we use a non-scoped enum for this so that we can use it as an index
        enum AuxGeomShapePerpectiveType
        {
            PerspectiveType_ViewProjection,
            PerspectiveType_ManualOverride, // View Perspective transform passed in through the view Projection override

            PerspectiveType_Count
        };

        //! Internally we use a non-scoped enum for this so that we can use it as an index
        enum AuxGeomDrawStyle
        {
            DrawStyle_Point,
            DrawStyle_Line,
            DrawStyle_Solid,
            DrawStyle_Shaded, // only available for fixed shapes.

            DrawStyle_Count
        };

        //! Used for shape objects
        //! This is not a scoped enum because we want to be able to use its values as array indices
        enum AuxGeomShapeType
        {
            ShapeType_Sphere,
            ShapeType_Hemisphere,
            ShapeType_Cone,
            ShapeType_Cylinder,
            ShapeType_CylinderNoEnds,       // Cylinder without disks on either end
            ShapeType_Disk,
            ShapeType_Quad,

            ShapeType_Count
        };

        //! Each fixed shape drawn through the AuxGeom draw interface is stored in the scene data as an instance of this struct
        struct ShapeBufferEntry
        {
            AuxGeomShapeType        m_shapeType;
            AuxGeomDepthReadType    m_depthRead;
            AuxGeomDepthWriteType   m_depthWrite;
            AuxGeomFaceCullMode     m_faceCullMode;
            AZ::Color               m_color;
            AZ::Vector3             m_position;
            AZ::Vector3             m_scale;
            AZ::Matrix3x3           m_rotationMatrix;

            //if < 0, then will render using the View's view and proj matrices, otherwise this indexes into AuxGeomBufferData::m_viewProjOverrides
            int32_t                 m_viewProjOverrideIndex = -1;
            float                   m_pointSize; // only used for DrawStyle_Point
        };

        //! Each box drawn through the AuxGeom draw interface is stored in the scene data as an instance of this struct
        //! Objects can either be Shapes or Boxes.
        //! Boxes are kept separate because they do not have LODs so they have a different processing path. It also saves the
        //! memory of storing a shape type for boxes.
        //! Keeping them in a separate list also makes instancing possible.
        struct BoxBufferEntry
        {
            AuxGeomDepthReadType    m_depthRead;
            AuxGeomDepthWriteType   m_depthWrite;
            AuxGeomFaceCullMode     m_faceCullMode;
            AZ::Color               m_color;
            AZ::Vector3             m_position;
            AZ::Vector3             m_scale;
            AZ::Matrix3x3           m_rotationMatrix;

            //if < 0, then will render using the View's view and proj matrices, otherwise this indexes into AuxGeomBufferData::m_viewProjOverrides
            int32_t                 m_viewProjOverrideIndex = -1;
            float                   m_pointSize; // only used for DrawStyle_Point
        };

        using PrimitiveBuffer = AZStd::vector<PrimitiveBufferEntry>;
        using VertexBuffer = AZStd::vector<AuxGeomDynamicVertex>;
        using IndexBuffer = AZStd::vector<AuxGeomIndex>;
        using ShapeBuffer = AZStd::vector<ShapeBufferEntry>;
        using BoxBuffer = AZStd::vector<BoxBufferEntry>;

        //! We have a single index and vertex buffer for all dynamic primitives.
        //! Each AuxGeom API call is a separate draw call
        struct DynamicPrimitiveData
        {
            PrimitiveBuffer     m_primitiveBuffer;  //!< State for each dynamic primitive draw
            VertexBuffer        m_vertexBuffer;     //!< The vertices for all dynamic verts
            IndexBuffer         m_indexBuffer;      //!< The indices for all dynamic primitives
        };

        //! This is all the data that is stored for each frame and returned from AuxGeomDrawQueue::Commit
        struct AuxGeomBufferData
        {
            DynamicPrimitiveData    m_primitiveData;                            //!< The dynamic primitives
            ShapeBuffer             m_opaqueShapes[DrawStyle_Count];            //!< The opaque shape objects
            ShapeBuffer             m_translucentShapes[DrawStyle_Count];       //!< The translucent shape objects
            BoxBuffer               m_opaqueBoxes[DrawStyle_Count];             //!< The opaque box objects
            BoxBuffer               m_translucentBoxes[DrawStyle_Count];        //!< The translucent box objects

            AZStd::vector<AZ::Matrix4x4> m_viewProjOverrides;
            int32_t                      m_2DViewProjOverrideIndex = -1;
        };

        //! The maximum index allowed for of dynamic vertex indices
        static const size_t MaxDynamicVertexIndex = std::numeric_limits<AuxGeomIndex>::max();

        //! The maximum number of dynamic vertices we allow in one vertex buffer
        static const size_t MaxDynamicVertexCount = AZStd::min<size_t>(MaxDynamicVertexIndex + 1, 1*1024*1024); // limit max vertex count to 1M

        //! Utility functions to convert api enums to internal enums.
        //! We prefer scoped enums in public interfaces but internally we use the unscoped enum for array sizes,
        //! indices and loop counters.
        inline AuxGeomDrawStyle ConvertRPIDrawStyle(RPI::AuxGeomDraw::DrawStyle rpiDrawStyle)
        {
            switch (rpiDrawStyle)
            {
            case RPI::AuxGeomDraw::DrawStyle::Point: return DrawStyle_Point;
            case RPI::AuxGeomDraw::DrawStyle::Line: return DrawStyle_Line;
            case RPI::AuxGeomDraw::DrawStyle::Solid: return DrawStyle_Solid;
            case RPI::AuxGeomDraw::DrawStyle::Shaded: return DrawStyle_Shaded;
            }
            AZ_Assert(false, "Invalid RPI::DrawStyle value passed to AuxGeom");
            return DrawStyle_Count;
        }

        inline AuxGeomDepthReadType ConvertRPIDepthTestFlag(RPI::AuxGeomDraw::DepthTest rpiDepthTest)
        {
            switch (rpiDepthTest)
            {
            case RPI::AuxGeomDraw::DepthTest::On:  return DepthRead_On;
            case RPI::AuxGeomDraw::DepthTest::Off: return DepthRead_Off;
            }
            AZ_Assert(false, "Invalid RPI::DepthTest value passed to AuxGeom");
            return DepthRead_Count;
        }

        inline AuxGeomDepthWriteType ConvertRPIDepthWriteFlag(RPI::AuxGeomDraw::DepthWrite rpiDepthWrite)
        {
            switch (rpiDepthWrite)
            {
            case RPI::AuxGeomDraw::DepthWrite::On:  return DepthWrite_On;
            case RPI::AuxGeomDraw::DepthWrite::Off: return DepthWrite_Off;
            }
            AZ_Assert(false, "Invalid RPI::DepthWrite value passed to AuxGeom");
            return DepthWrite_Count;
        }

        inline AuxGeomFaceCullMode ConvertRPIFaceCullFlag(RPI::AuxGeomDraw::FaceCullMode rpiFaceCull)
        {
            switch(rpiFaceCull)
            {
                case RPI::AuxGeomDraw::FaceCullMode::None:      return FaceCull_None;
                case RPI::AuxGeomDraw::FaceCullMode::Front:     return FaceCull_Front;
                case RPI::AuxGeomDraw::FaceCullMode::Back:      return FaceCull_Back;
            }
            AZ_Assert(false, "Invalid RPI::FaceCullMode value passed to AuxGeom");
            return FaceCull_Count;
        }
    } // namespace Render
} // namespace AZ
