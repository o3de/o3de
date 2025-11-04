/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Picking/BoundInterface.h>
#include <AzToolsFramework/Picking/Manipulators/ManipulatorBounds.h>

namespace AzToolsFramework
{
    namespace Picking
    {
        //! An interface concrete shape types can implement to create specific BoundShapeInterfaces.
        class BoundRequestShapeBase
        {
        public:
            AZ_RTTI(BoundRequestShapeBase, "{60D52E6E-54A6-4236-A397-322FD7607FA3}");

            BoundRequestShapeBase() = default;
            virtual ~BoundRequestShapeBase() = default;

            virtual AZStd::shared_ptr<BoundShapeInterface> MakeShapeInterface(RegisteredBoundId id) const = 0;
        };

        class BoundShapeBox : public BoundRequestShapeBase
        {
        public:
            AZ_RTTI(BoundShapeBox, "{6BF78BC6-5100-41A1-84E1-6F4E552E2FC5}", BoundRequestShapeBase);
            AZ_CLASS_ALLOCATOR(BoundShapeBox, AZ::SystemAllocator);

            AZStd::shared_ptr<BoundShapeInterface> MakeShapeInterface(RegisteredBoundId id) const override
            {
                AZStd::shared_ptr<BoundShapeInterface> box = AZStd::make_shared<ManipulatorBoundBox>(id);
                box->SetShapeData(*this);
                return box;
            }

            AZ::Vector3 m_center;
            AZ::Quaternion m_orientation;
            AZ::Vector3 m_halfExtents;
        };

        class BoundShapeSphere : public BoundRequestShapeBase
        {
        public:
            AZ_RTTI(BoundShapeSphere, "{786168B7-46BB-4C0E-9642-5A7B94BF00FA}", BoundRequestShapeBase);
            AZ_CLASS_ALLOCATOR(BoundShapeSphere, AZ::SystemAllocator);

            AZStd::shared_ptr<BoundShapeInterface> MakeShapeInterface(RegisteredBoundId id) const override
            {
                AZStd::shared_ptr<BoundShapeInterface> sphere = AZStd::make_shared<ManipulatorBoundSphere>(id);
                sphere->SetShapeData(*this);
                return sphere;
            }

            AZ::Vector3 m_center;
            float m_radius;
        };

        class BoundShapeCylinder : public BoundRequestShapeBase
        {
        public:
            AZ_RTTI(BoundShapeCylinder, "{3D9A8328-4371-4EC5-BEC2-783998B19200}", BoundRequestShapeBase);
            AZ_CLASS_ALLOCATOR(BoundShapeCylinder, AZ::SystemAllocator);

            AZStd::shared_ptr<BoundShapeInterface> MakeShapeInterface(RegisteredBoundId id) const override
            {
                AZStd::shared_ptr<BoundShapeInterface> cylinder = AZStd::make_shared<ManipulatorBoundCylinder>(id);
                cylinder->SetShapeData(*this);
                return cylinder;
            }

            AZ::Vector3 m_axis;
            AZ::Vector3 m_base;
            float m_height;
            float m_radius;
        };

        class BoundShapeCone : public BoundRequestShapeBase
        {
        public:
            AZ_RTTI(BoundShapeCone, "{68D67118-EAC9-4384-BE99-2CAB72A0450A}", BoundRequestShapeBase);
            AZ_CLASS_ALLOCATOR(BoundShapeCone, AZ::SystemAllocator);

            AZStd::shared_ptr<BoundShapeInterface> MakeShapeInterface(RegisteredBoundId id) const override
            {
                AZStd::shared_ptr<BoundShapeInterface> cone = AZStd::make_shared<ManipulatorBoundCone>(id);
                cone->SetShapeData(*this);
                return cone;
            }

            AZ::Vector3 m_axis;
            AZ::Vector3 m_base;
            float m_height;
            float m_radius;
        };

        //! The quad shape consists of 4 points in 3D space. Please set them from \ref m_corner1 to \ref m_corner4
        //! in either clock-wise winding or counter clock-wise winding. In another word, \ref m_corner1 and
        //! \ref corner_2 cannot be diagonal corners.
        class BoundShapeQuad : public BoundRequestShapeBase
        {
        public:
            AZ_RTTI(BoundShapeQuad, "{D1F73C4B-3B42-4493-B1D1-38EE59F2C7F3}", BoundRequestShapeBase)
            AZ_CLASS_ALLOCATOR(BoundShapeQuad, AZ::SystemAllocator)

            AZStd::shared_ptr<BoundShapeInterface> MakeShapeInterface(RegisteredBoundId id) const override
            {
                AZStd::shared_ptr<BoundShapeInterface> quad = AZStd::make_shared<ManipulatorBoundQuad>(id);
                quad->SetShapeData(*this);
                return quad;
            }

            AZ::Vector3 m_corner1;
            AZ::Vector3 m_corner2;
            AZ::Vector3 m_corner3;
            AZ::Vector3 m_corner4;
        };

        //! The line segment consists of two points in 3D space defining a line the user can interact with.
        class BoundShapeLineSegment : public BoundRequestShapeBase
        {
        public:
            AZ_RTTI(BoundShapeLineSegment, "{BC5DCB8B-E9F7-41BB-AD93-00D3EAB108D3}", BoundRequestShapeBase)
            AZ_CLASS_ALLOCATOR(BoundShapeLineSegment, AZ::SystemAllocator)

            AZStd::shared_ptr<BoundShapeInterface> MakeShapeInterface(RegisteredBoundId id) const override
            {
                AZStd::shared_ptr<BoundShapeInterface> lineSegment = AZStd::make_shared<ManipulatorBoundLineSegment>(id);
                lineSegment->SetShapeData(*this);
                return lineSegment;
            }

            AZ::Vector3 m_start;
            AZ::Vector3 m_end;
            float m_width;
        };

        //! The torus shape is approximated by a cylinder whose radius is the sum of the torus's major radius
        //! and minor radius and height is twice the torus's minor radius.
        class BoundShapeTorus : public BoundRequestShapeBase
        {
        public:
            AZ_RTTI(BoundShapeTorus, "{2EF456F8-87D4-44CD-9929-FC45981289D4}", BoundRequestShapeBase)
            AZ_CLASS_ALLOCATOR(BoundShapeTorus, AZ::SystemAllocator)

            AZStd::shared_ptr<BoundShapeInterface> MakeShapeInterface(RegisteredBoundId id) const override
            {
                AZStd::shared_ptr<BoundShapeInterface> torus = AZStd::make_shared<ManipulatorBoundTorus>(id);
                torus->SetShapeData(*this);
                return torus;
            }

            AZ::Vector3 m_axis;
            AZ::Vector3 m_center;
            float m_majorRadius;
            float m_minorRadius;
        };

        //! The spline is specified by a number of vertices. A piecewise approximation of the curve
        //! is computed by using a number of linear steps (defined by the granularity of the curve).
        class BoundShapeSpline : public BoundRequestShapeBase
        {
        public:
            AZ_RTTI(BoundShapeSpline, "{65CBF85A-5126-4F2A-AA2E-047367435DEC}", BoundRequestShapeBase)
            AZ_CLASS_ALLOCATOR(BoundShapeSpline, AZ::SystemAllocator)

            AZStd::shared_ptr<BoundShapeInterface> MakeShapeInterface(RegisteredBoundId id) const override
            {
                AZStd::shared_ptr<BoundShapeInterface> spline = AZStd::make_shared<ManipulatorBoundSpline>(id);
                spline->SetShapeData(*this);
                return spline;
            }

            AZStd::weak_ptr<const AZ::Spline> m_spline;
            AZ::Transform m_transform;
            float m_width;
        };

        //! Ray query for intersection against bounds.
        struct RaySelectInfo
        {
            AZ::Vector3 m_origin; //!< Start of ray.
            AZ::Vector3 m_direction; //!< Direction of ray - make sure m_direction is unit length.
            AZStd::vector<AZStd::pair<RegisteredBoundId, float>> m_boundIdsHit; //!< Store the id of the intersected bound
                                                                                //!< and the parameter of the corresponding
                                                                                //!< intersecting point.
        };
    } // namespace Picking
} // namespace AzToolsFramework
