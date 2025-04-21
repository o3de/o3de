/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/MathScriptHelpers.h>
#include <AzCore/Math/ShapeIntersection.h>

namespace AZ
{
    void ViewFrustumAttributes::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ViewFrustumAttributes>()
                ->Version(1)
                ->Field("WorldTransform", &ViewFrustumAttributes::m_worldTransform)
                ->Field("AspectRatio", &ViewFrustumAttributes::m_aspectRatio)
                ->Field("FovRadians", &ViewFrustumAttributes::m_verticalFovRadians)
                ->Field("NearClip", &ViewFrustumAttributes::m_nearClip)
                ->Field("FarClip", &ViewFrustumAttributes::m_farClip)
                ;

            if (EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ViewFrustumAttributes>("ViewFrustumAttributes", "")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                    ->DataElement(
                        Edit::UIHandlers::Default, &ViewFrustumAttributes::m_worldTransform, "WorldTransform",
                        "The world transform used to construct the frustum")
                    ->DataElement(
                        Edit::UIHandlers::Default, &ViewFrustumAttributes::m_aspectRatio, "AspectRatio",
                        "The aspect ratio of the frustum")
                    ->DataElement(
                        Edit::UIHandlers::Default, &ViewFrustumAttributes::m_verticalFovRadians, "FovRadians",
                        "The vertical field of view of the frustum in radians")
                    ->DataElement(
                        Edit::UIHandlers::Default, &ViewFrustumAttributes::m_nearClip, "NearClip",
                        "Distance to the frustums near clip plane")
                    ->DataElement(
                        Edit::UIHandlers::Default, &ViewFrustumAttributes::m_farClip, "FarClip",
                        "Distance to the frustums far clip plane")
                    ;
            }
        }
    }


    void Frustum::Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<Frustum>()
                ->Version(2)
                ->Field("Planes", &Frustum::m_serializedPlanes)
                ;
        }
    }


    Frustum::Frustum(const ViewFrustumAttributes& viewFrustumAttributes)
    {
        ConstructPlanes(viewFrustumAttributes);
    }


    Frustum::Frustum(
        const Plane& nearPlane, const Plane& farPlane, const Plane& leftPlane, const Plane& rightPlane,
        const Plane& topPlane, const Plane& bottomPlane)
    {
        SetPlane(PlaneId::Near,   nearPlane);
        SetPlane(PlaneId::Far,    farPlane);
        SetPlane(PlaneId::Left,   leftPlane);
        SetPlane(PlaneId::Right,  rightPlane);
        SetPlane(PlaneId::Top,    topPlane);
        SetPlane(PlaneId::Bottom, bottomPlane);
    }


    Frustum Frustum::CreateFromMatrixRowMajor(const Matrix4x4& matrix, ReverseDepth reverseDepth)
    {
        const PlaneId nearPlaneId = (reverseDepth == ReverseDepth::True) ? PlaneId::Far  : PlaneId::Near;
        const PlaneId farPlaneId  = (reverseDepth == ReverseDepth::True) ? PlaneId::Near : PlaneId::Far;

        Frustum frustum;
        frustum.SetPlane(nearPlaneId,     Plane::CreateFromVectorCoefficients(matrix.GetColumn(2)));
        frustum.SetPlane(farPlaneId,      Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(2)));
        frustum.SetPlane(PlaneId::Left,   Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) + matrix.GetColumn(0)));
        frustum.SetPlane(PlaneId::Right,  Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(0)));
        frustum.SetPlane(PlaneId::Top,    Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(1)));
        frustum.SetPlane(PlaneId::Bottom, Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) + matrix.GetColumn(1)));
        return frustum;
    }


    Frustum Frustum::CreateFromMatrixColumnMajor(const Matrix4x4& matrix, ReverseDepth reverseDepth)
    {
        const PlaneId nearPlaneId = (reverseDepth == ReverseDepth::True) ? PlaneId::Far  : PlaneId::Near;
        const PlaneId farPlaneId  = (reverseDepth == ReverseDepth::True) ? PlaneId::Near : PlaneId::Far;

        Frustum frustum;
        frustum.SetPlane(nearPlaneId,     Plane::CreateFromVectorCoefficients(matrix.GetRow(2)));
        frustum.SetPlane(farPlaneId,      Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(2)));
        frustum.SetPlane(PlaneId::Left,   Plane::CreateFromVectorCoefficients(matrix.GetRow(3) + matrix.GetRow(0)));
        frustum.SetPlane(PlaneId::Right,  Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(0)));
        frustum.SetPlane(PlaneId::Top,    Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(1)));
        frustum.SetPlane(PlaneId::Bottom, Plane::CreateFromVectorCoefficients(matrix.GetRow(3) + matrix.GetRow(1)));
        return frustum;
    }


    Frustum Frustum::CreateFromMatrixRowMajorSymmetricZ(const Matrix4x4& matrix, ReverseDepth reverseDepth)
    {
        const PlaneId nearPlaneId = (reverseDepth == ReverseDepth::True) ? PlaneId::Far  : PlaneId::Near;
        const PlaneId farPlaneId  = (reverseDepth == ReverseDepth::True) ? PlaneId::Near : PlaneId::Far;

        Frustum frustum;
        frustum.SetPlane(nearPlaneId,     Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) + matrix.GetColumn(2)));
        frustum.SetPlane(farPlaneId,      Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(2)));
        frustum.SetPlane(PlaneId::Left,   Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) + matrix.GetColumn(0)));
        frustum.SetPlane(PlaneId::Right,  Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(0)));
        frustum.SetPlane(PlaneId::Top,    Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) - matrix.GetColumn(1)));
        frustum.SetPlane(PlaneId::Bottom, Plane::CreateFromVectorCoefficients(matrix.GetColumn(3) + matrix.GetColumn(1)));
        return frustum;
    }


    Frustum Frustum::CreateFromMatrixColumnMajorSymmetricZ(const Matrix4x4& matrix, ReverseDepth reverseDepth)
    {
        const PlaneId nearPlaneId = (reverseDepth == ReverseDepth::True) ? PlaneId::Far  : PlaneId::Near;
        const PlaneId farPlaneId  = (reverseDepth == ReverseDepth::True) ? PlaneId::Near : PlaneId::Far;

        Frustum frustum;
        frustum.SetPlane(nearPlaneId,     Plane::CreateFromVectorCoefficients(matrix.GetRow(3) + matrix.GetRow(2)));
        frustum.SetPlane(farPlaneId,      Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(2)));
        frustum.SetPlane(PlaneId::Left,   Plane::CreateFromVectorCoefficients(matrix.GetRow(3) + matrix.GetRow(0)));
        frustum.SetPlane(PlaneId::Right,  Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(0)));
        frustum.SetPlane(PlaneId::Top,    Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(1)));
        frustum.SetPlane(PlaneId::Bottom, Plane::CreateFromVectorCoefficients(matrix.GetRow(3) - matrix.GetRow(1)));
        return frustum;
    }


    void Frustum::Set(const Frustum& frustum)
    {
        for (PlaneId planeId = PlaneId::Near; planeId < PlaneId::MAX; ++planeId)
        {
            SetPlane(planeId, frustum.m_serializedPlanes[planeId]);
        }
    }


    void Frustum::ConstructPlanes(const ViewFrustumAttributes& viewFrustumAttributes)
    {
        const float tanHalfFov = std::tan(viewFrustumAttributes.m_verticalFovRadians * 0.5f);
        const float nearPlaneHalfHeight = tanHalfFov * viewFrustumAttributes.m_nearClip;
        const float nearPlaneHalfWidth = nearPlaneHalfHeight * viewFrustumAttributes.m_aspectRatio;

        const Vector3 translation = viewFrustumAttributes.m_worldTransform.GetTranslation();
        const Vector3 forward = viewFrustumAttributes.m_worldTransform.GetBasisY();
        const Vector3 right = viewFrustumAttributes.m_worldTransform.GetBasisX();
        const Vector3 up = viewFrustumAttributes.m_worldTransform.GetBasisZ();

        SetPlane(
            PlaneId::Near,
            Plane::CreateFromNormalAndPoint(forward, translation + (forward * viewFrustumAttributes.m_nearClip)));
        SetPlane(
            PlaneId::Far,
            Plane::CreateFromNormalAndPoint(-forward, translation + (forward * viewFrustumAttributes.m_farClip)));

        const Vector3 leftNormal =
            (right + forward * (nearPlaneHalfWidth / viewFrustumAttributes.m_nearClip)).GetNormalized();
        const Vector3 rightNormal =
            (-right + forward * (nearPlaneHalfWidth / viewFrustumAttributes.m_nearClip)).GetNormalized();

        SetPlane(PlaneId::Left, Plane::CreateFromNormalAndPoint(leftNormal, translation));
        SetPlane(PlaneId::Right, Plane::CreateFromNormalAndPoint(rightNormal, translation));

        const Vector3 topNormal =
            (-up + forward * (nearPlaneHalfHeight / viewFrustumAttributes.m_nearClip)).GetNormalized();
        const Vector3 bottomNormal =
            (up + forward * (nearPlaneHalfHeight / viewFrustumAttributes.m_nearClip)).GetNormalized();

        SetPlane(PlaneId::Top, Plane::CreateFromNormalAndPoint(topNormal, translation));
        SetPlane(PlaneId::Bottom, Plane::CreateFromNormalAndPoint(bottomNormal, translation));
    }


    ViewFrustumAttributes Frustum::CalculateViewFrustumAttributes() const
    {
        using Simd::Vec4;

        ViewFrustumAttributes viewFrustumAttributes;

        const Vector3 forward = Vector4(m_planes[PlaneId::Near]).GetAsVector3().GetNormalized();
        const Vector3 right =
            Vector4(Vec4::Sub(m_planes[PlaneId::Left], m_planes[PlaneId::Right])).GetAsVector3().GetNormalized();
        const Vector3 up =
            Vector4(Vec4::Sub(m_planes[PlaneId::Bottom], m_planes[PlaneId::Top])).GetAsVector3().GetNormalized();

        const Matrix3x3 orientation = Matrix3x3::CreateFromColumns(right, forward, up);
        const Plane bottom(m_planes[PlaneId::Bottom]);
        const Plane top(m_planes[PlaneId::Top]);
        const Plane left(m_planes[PlaneId::Left]);

        // solve a set of simultaneous equations to find the point that is contained within all planes
        // (note: this is not a transformation, it is simply using Matrix3x3 to perform some linear algebra)
        const Vector3 origin =
            -Matrix3x3::CreateFromRows(bottom.GetNormal(), top.GetNormal(), left.GetNormal()).GetInverseFull() *
            Vector3(bottom.GetDistance(), top.GetDistance(), left.GetDistance());

        viewFrustumAttributes.m_worldTransform = Transform::CreateFromMatrix3x3AndTranslation(orientation, origin);

        const float originDotForward = origin.Dot(forward);
        const float nearClip = -Vec4::SelectIndex3(m_planes[PlaneId::Near]) - originDotForward;

        viewFrustumAttributes.m_nearClip = nearClip;
        viewFrustumAttributes.m_farClip = Vec4::SelectIndex3(m_planes[PlaneId::Far]) - originDotForward;

        const float leftNormalDotForward = left.GetNormal().Dot(forward);
        const float frustumNearHeight =
            2.0f * nearClip * leftNormalDotForward / std::sqrt(1.0f - leftNormalDotForward * leftNormalDotForward);
        const float bottomNormalDotForward = bottom.GetNormal().Dot(forward);
        const float tanHalfFov =
            bottomNormalDotForward / std::sqrt(1.0f - bottomNormalDotForward * bottomNormalDotForward);
        const float frustumNearWidth = 2.0f * nearClip * tanHalfFov;

        viewFrustumAttributes.m_aspectRatio = frustumNearHeight / frustumNearWidth;
        viewFrustumAttributes.m_verticalFovRadians = 2.0f * std::atan(tanHalfFov);

        return viewFrustumAttributes;
    }

    bool Frustum::GetCorners(CornerVertexArray& corners) const
    {
        using ShapeIntersection::IntersectThreePlanes;

        return
            IntersectThreePlanes(GetPlane(Near), GetPlane(Top), GetPlane(Left), corners[NearTopLeft]) &&
            IntersectThreePlanes(GetPlane(Near), GetPlane(Top), GetPlane(Right), corners[NearTopRight]) &&
            IntersectThreePlanes(GetPlane(Near), GetPlane(Bottom), GetPlane(Left), corners[NearBottomLeft]) &&
            IntersectThreePlanes(GetPlane(Near), GetPlane(Bottom), GetPlane(Right), corners[NearBottomRight]) &&
            IntersectThreePlanes(GetPlane(Far), GetPlane(Top), GetPlane(Left), corners[FarTopLeft]) &&
            IntersectThreePlanes(GetPlane(Far), GetPlane(Top), GetPlane(Right), corners[FarTopRight]) &&
            IntersectThreePlanes(GetPlane(Far), GetPlane(Bottom), GetPlane(Left), corners[FarBottomLeft]) &&
            IntersectThreePlanes(GetPlane(Far), GetPlane(Bottom), GetPlane(Right), corners[FarBottomRight])
            ;
    }
} // namespace AZ
