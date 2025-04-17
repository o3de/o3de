/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Sphere.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Plane.h>
#include <AzCore/Math/SimdMath.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    class ReflectContext;

    //! Attributes required to construct a Frustum from a view volume.
    struct ViewFrustumAttributes
    {
        AZ_TYPE_INFO(ViewFrustumAttributes, "{4D36516C-96D5-4BB8-BAD9-5E68D281ACE5}");

        static void Reflect(ReflectContext* context);

        ViewFrustumAttributes() = default;
        ViewFrustumAttributes(
            const Transform& worldTransform, const float aspectRatio, const float verticalFovRadians,
            const float nearClip, const float farClip)
            : m_worldTransform(worldTransform)
            , m_aspectRatio(aspectRatio)
            , m_verticalFovRadians(verticalFovRadians)
            , m_nearClip(nearClip)
            , m_farClip(farClip)
        {
        }

        Transform m_worldTransform = Transform::CreateIdentity(); //! The transform (in world space) of the Frustum.
        float m_aspectRatio = 0.0f; //! The aspect ratio (width / height) of the Frustum.
        float m_verticalFovRadians = 0.0f; //! The vertical fov of the Frustum in radians.
        float m_nearClip = 0.0f; //! The distance to the near clip plane of the Frustum.
        float m_farClip = 0.0f; //! The distance to the far clip plane of the Frustum.
    };

    //! A frustum class that can be used for efficient primitive intersection tests.
    class Frustum final
    {
    public:

        AZ_TYPE_INFO(Frustum, "{AE7D2ADA-0266-494A-98C4-699F099171C2}");

        enum PlaneId
        {
            Near,
            Far,
            Left,
            Right,
            Top,
            Bottom,
            MAX,
        };

        enum class ReverseDepth
        {
            True,
            False,
        };

        enum CornerIndices
        {
            NearTopLeft, NearTopRight, NearBottomLeft, NearBottomRight,
            FarTopLeft, FarTopRight, FarBottomLeft, FarBottomRight,
            Count,
        };

        using CornerVertexArray = AZStd::array<AZ::Vector3, CornerIndices::Count>;

        //! AzCore Reflection.
        //! @param context reflection context
        static void Reflect(ReflectContext* context);

        //! Default constructor, leaves members uninitialized for speed.
        Frustum();

        //! Construct a view frustum from ViewFrustumAttributes.
        //! 
        //! @param viewFrustumAttributes Attributes to build a Frustum from a view volume.
        explicit Frustum(const ViewFrustumAttributes& viewFrustumAttributes);

        explicit Frustum(
            const Plane& nearPlane, const Plane& farPlane, const Plane& leftPlane, const Plane& rightPlane,
            const Plane& topPlane, const Plane& bottomPlane);

        //! Extract frustum from matrix.
        //! Matrix Usage and Z conventions form the matrix of use cases.
        //! Symmetric-z (OpenGL convention) implies -w <= z <= w
        //! Non-symmetric (DirectX convention) implies 0 <= z <= w
        //! RowMajor implies x*M convention
        //! ColumnMajor implies M*x convention
        //! @{
        static Frustum CreateFromMatrixRowMajor(const Matrix4x4& matrix, ReverseDepth reverseDepth = ReverseDepth::False);
        static Frustum CreateFromMatrixColumnMajor(const Matrix4x4& matrix, ReverseDepth reverseDepth = ReverseDepth::False);
        static Frustum CreateFromMatrixRowMajorSymmetricZ(const Matrix4x4& matrix, ReverseDepth reverseDepth = ReverseDepth::False);
        static Frustum CreateFromMatrixColumnMajorSymmetricZ(const Matrix4x4& matrix, ReverseDepth reverseDepth = ReverseDepth::False);
        //! @}

        //! Returns the requested plane.
        //! @param planeId the index of the plane to retrieve
        Plane GetPlane(PlaneId planeId) const;

        //! Sets the requested plane.
        //! @param planeId the index of the plane to set
        //! @param plane the new plane value to set to
        void SetPlane(PlaneId planeId, const Plane& plane);

        //! Clones the provided frustum.
        //! @param frustum the frustum instance to clone
        void Set(const Frustum& frustum);

        //! Intersects a sphere against the frustum.
        //! 
        //! @param center the center of the sphere to test against
        //! @param radius the radius of the sphere to test against
        //! 
        //! @return the intersection result of the sphere against the frustum
        IntersectResult IntersectSphere(const Vector3& center, float radius) const;

        //! Intersects a sphere against the frustum.
        //! 
        //! @param sphere the sphere to test against
        //! 
        //! @return the intersection result of the sphere against the frustum
        IntersectResult IntersectSphere(const Sphere& sphere) const;

        //! Intersects an axis-aligned bounding box.
        //! 
        //! @param minimum the smallest extents of the bounding volume to test against
        //! @param maximum the largest extents of the bounding volume to test against
        //! 
        //! @return the intersection result of the Aabb against the frustum
        IntersectResult IntersectAabb(const Vector3& minimum, const Vector3& maximum) const;

        //! Intersects an axis-aligned bounding box.
        //! 
        //! @param aabb the axis-aligned bounding box to test against
        //! 
        //! @return the intersection result of the Aabb against the frustum
        IntersectResult IntersectAabb(const Aabb& aabb) const;

        //! Returns true if the current frustum and provided frustum are close to identical.
        //! @param rhs the frustum to compare against for closeness
        bool IsClose(const Frustum& rhs, float tolerance = Constants::Tolerance) const;

        //! Fills an array with corner vertices and returns true for valid Frustums.
        //! @param corners The array of corner vertices to fill.
        bool GetCorners(CornerVertexArray& corners) const;

        //! Returns the corresponding view volume attributes for the frustum.
        ViewFrustumAttributes CalculateViewFrustumAttributes() const;

    private:
        //! Helper method that constructs the various Frustum test planes.
        void ConstructPlanes(const ViewFrustumAttributes& viewFrustumAttributes);

        // Precomputed simd friendly planes for intersection tests
        Simd::Vec4::FloatType m_planes[PlaneId::MAX];
        AZStd::array<Plane, PlaneId::MAX> m_serializedPlanes;
    };
} // namespace AZ

#include <AzCore/Math/Frustum.inl>
