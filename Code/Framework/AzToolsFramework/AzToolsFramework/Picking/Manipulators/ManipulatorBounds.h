/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/Picking/BoundInterface.h>

namespace AZ
{
    class Spline;
}

namespace AzToolsFramework
{
    namespace Picking
    {
        //! Custom ray/cone intersect function for manipulator bounds to workaround a bug in the current AZ::Intersect implementation.
        //! @note Does not give reliable results if the ray origin is inside the cone.
        //! @param      rayOrigin      The origin of the ray to test.
        //! @param      rayDirection   The direction of the ray to test, it must be unit length.
        //! @param      coneApex       The apex of the cone.
        //! @param      coneAxis       The unit-length axis (direction) from the apex to the base.
        //! @param      coneHeight     The height of the cone, from the apex to the base.
        //! @param      coneBaseRadius The radius of the cone base.
        //! @param[out] t              A possible coefficient in the ray's explicit equation from which an intersecting point is calculated
        //!                            as "rayOrigin + t * rayDirection". 't' is the closest intersecting point to the ray origin.
        //! @return                    true for intersecting, false for not intersecting.
        bool IntersectRayCone(
            const AZ::Vector3& rayOrigin,
            const AZ::Vector3& rayDirection,
            const AZ::Vector3& coneApex,
            const AZ::Vector3& coneAxis,
            float coneHeight,
            float coneBaseRadius,
            float& t);

        class ManipulatorBoundSphere : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundSphere, "{64D1B863-F574-4B31-A4F2-C9744D8567B3}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundSphere, AZ::SystemAllocator);

            explicit ManipulatorBoundSphere(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            {
            }

            bool IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDir, float& rayIntersectionDistance) const override;
            void SetShapeData(const BoundRequestShapeBase& shapeData) override;

            AZ::Vector3 m_center = AZ::Vector3::CreateZero();
            float m_radius = 0.0f;
        };

        class ManipulatorBoundBox : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundBox, "{3AD46067-933F-49B4-82E1-DBF12C7BC02E}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundBox, AZ::SystemAllocator);

            explicit ManipulatorBoundBox(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            {
            }

            bool IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDir, float& rayIntersectionDistance) const override;
            void SetShapeData(const BoundRequestShapeBase& shapeData) override;

            AZ::Vector3 m_center = AZ::Vector3::CreateZero();
            AZ::Vector3 m_axis1 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_axis2 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_axis3 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_halfExtents = AZ::Vector3::CreateZero();
        };

        class ManipulatorBoundCylinder : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundCylinder, "{D248F9E4-22E6-41A8-898D-704DF307B533}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundCylinder, AZ::SystemAllocator);

            explicit ManipulatorBoundCylinder(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            {
            }

            bool IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDir, float& rayIntersectionDistance) const override;
            void SetShapeData(const BoundRequestShapeBase& shapeData) override;

            AZ::Vector3 m_base = AZ::Vector3::CreateZero(); //!< The center of the circle at the base of the cylinder.
            AZ::Vector3 m_axis = AZ::Vector3::CreateZero();
            float m_height = 0.0f;
            float m_radius = 0.0f;
        };

        class ManipulatorBoundCone : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundCone, "{9430440D-DFF2-4A60-9073-507C4E9DD65D}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundCone, AZ::SystemAllocator);

            explicit ManipulatorBoundCone(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            {
            }

            bool IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDir, float& rayIntersectionDistance) const override;
            void SetShapeData(const BoundRequestShapeBase& shapeData) override;

            AZ::Vector3 m_apexPosition = AZ::Vector3::CreateZero();
            AZ::Vector3 m_dir = AZ::Vector3::CreateZero();
            float m_radius = 0.0f;
            float m_height = 0.0f;
        };

        //! The quad shape consists of 4 points in 3D space. Please set them from \ref m_corner1 to \ref m_corner4
        //! in either clock-wise winding or counter clock-wise winding. In another word, \ref m_corner1 and
        //! \ref corner_2 cannot be diagonal corners.
        class ManipulatorBoundQuad : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundQuad, "{3CDED61C-5786-4299-B5F2-5970DE4457AD}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundQuad, AZ::SystemAllocator);

            explicit ManipulatorBoundQuad(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            {
            }

            bool IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDir, float& rayIntersectionDistance) const override;
            void SetShapeData(const BoundRequestShapeBase& shapeData) override;

            AZ::Vector3 m_corner1 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_corner2 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_corner3 = AZ::Vector3::CreateZero();
            AZ::Vector3 m_corner4 = AZ::Vector3::CreateZero();
        };

        class ManipulatorBoundTorus : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundTorus, "{46E4711C-178A-4F97-BC14-A048D096E7A1}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundTorus, AZ::SystemAllocator);

            explicit ManipulatorBoundTorus(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            {
            }

            bool IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDir, float& rayIntersectionDistance) const override;
            void SetShapeData(const BoundRequestShapeBase& shapeData) override;

            // Approximate a torus as a thin cylinder. A ray intersects a torus when the ray and the torus'
            // approximating cylinder have an intersecting point that is at certain distance away from the
            // center of the torus.
            AZ::Vector3 m_center = AZ::Vector3::CreateZero();
            AZ::Vector3 m_axis = AZ::Vector3::CreateZero();
            float m_majorRadius = 0.0f; //!< Usually denoted as "R", the distance from the center of the tube to the center of the torus.
            float m_minorRadius = 0.0f; //!< Usually denoted as "r", the radius of the tube.
        };

        class ManipulatorBoundLineSegment : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundLineSegment, "{66801554-1C1A-4E79-B1E7-342DFA779D53}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundLineSegment, AZ::SystemAllocator);

            explicit ManipulatorBoundLineSegment(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            {
            }

            bool IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDir, float& rayIntersectionDistance) const override;
            void SetShapeData(const BoundRequestShapeBase& shapeData) override;

            AZ::Vector3 m_worldStart = AZ::Vector3::CreateZero();
            AZ::Vector3 m_worldEnd = AZ::Vector3::CreateZero();
            float m_width = 0.0f;
        };

        class ManipulatorBoundSpline : public BoundShapeInterface
        {
        public:
            AZ_RTTI(ManipulatorBoundSpline, "{777760FF-8547-45AD-876F-16BA4D9D0584}", BoundShapeInterface);
            AZ_CLASS_ALLOCATOR(ManipulatorBoundSpline, AZ::SystemAllocator);

            explicit ManipulatorBoundSpline(RegisteredBoundId boundId)
                : BoundShapeInterface(boundId)
            {
            }

            bool IntersectRay(const AZ::Vector3& rayOrigin, const AZ::Vector3& rayDir, float& rayIntersectionDistance) const override;
            void SetShapeData(const BoundRequestShapeBase& shapeData) override;

            AZStd::weak_ptr<const AZ::Spline> m_spline;
            AZ::Transform m_transform;
            float m_width = 0.0f;
        };

        //! Approximate intersection with a torus-like shape.
        bool IntersectHollowCylinder(
            const AZ::Vector3& rayOrigin,
            const AZ::Vector3& rayDirection,
            const AZ::Vector3& center,
            const AZ::Vector3& axis,
            float minorRadius,
            float majorRadius,
            float& rayIntersectionDistance);

    } // namespace Picking
} // namespace AzToolsFramework
