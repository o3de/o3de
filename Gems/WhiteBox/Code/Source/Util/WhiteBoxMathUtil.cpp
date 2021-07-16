/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxMathUtil.h"

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

namespace WhiteBox
{
    //! Reference: Real-Time Collision Detection - 5.3.7 Intersecting Ray or Segment Against Cylinder
    //! @note the parameters and style here are copied verbatim from the book
    //!  to make comparison and bug finding trivial.
    bool IntersectSegmentCylinder(
        const AZ::Vector3& sa, const AZ::Vector3& sb, const AZ::Vector3& p, const AZ::Vector3& q, float r, float& t)
    {
        // clang-format off
        const AZ::Vector3 d = q - p;
        const AZ::Vector3 m = sa - p;
        const AZ::Vector3 n = sb - sa;

        const float md = m.Dot(d);
        const float nd = n.Dot(d);
        const float dd = d.Dot(d);

        if (md < 0.0f && md + nd < 0.0f) return false;
        if (md > dd && md + nd > dd) return false;
        const float nn = n.Dot(n);
        const float mn = m.Dot(n);
        const float a = dd * nn - nd * nd;
        const float k = m.Dot(m) - r * r;
        const float c = dd * k - md * md;
        if (AZ::GetAbs(a) < AZ::Constants::FloatEpsilon) {
            if (c > 0.0f) return false;
            if (md < 0.0f) t = -mn / nn;
            else if (md > dd) t = -mn / nn;
            else t = 0.0f;
            return true;
        }
        const float b = dd * mn - nd * md;
        const float discr = b * b - a * c;
        if (discr < 0.0f) return false;
        t = (-b - sqrt(discr)) / a;
        if (t < 0.0f || t > 1.0f) return false;
        if (md + t * nd < 0.0f) {
            if (nd <= 0.0f) return false;
            t = -md / nd;
            return k + 2 * t * (mn + t * nn) <= 0.0f;
        }
        else if (md + t * nd > dd) {
            if (nd >= 0.0f) return false;
            t = (dd - md) / nd;
            return k + dd - 2 * md + t * (2 * (mn - nd) + t * nn) <= 0.0f;
        }
        return true;
        // clang-format on
    }

    AZ::Vector3 ScalePosition(const float scale, const AZ::Vector3& localPosition, const AZ::Transform& localFromSpace)
    {
        const AZ::Transform spaceFromLocal = localFromSpace.GetInverse();
        const AZ::Vector3 spacePosition = spaceFromLocal.TransformPoint(localPosition);
        const AZ::Vector3 spaceScaledPosition =
            AZ::Transform::CreateUniformScale(scale).TransformPoint(spacePosition);
        return localFromSpace.TransformPoint(spaceScaledPosition);
    }

    void CalculateOrthonormalBasis(const AZ::Vector3& n, AZ::Vector3& b1, AZ::Vector3& b2)
    {
        // choose a vector orthogonal to n as the direction of b2
        if (fabsf(n.GetX()) > fabs(n.GetZ()))
        {
            b2 = AZ::Vector3(-n.GetY(), n.GetX(), 0.0f);
        }
        else
        {
            b2 = AZ::Vector3(0.0f, -n.GetZ(), n.GetY());
        }

        b2.Normalize();
        b1 = b2.Cross(n);
    }

    AZ::Quaternion CalculateLocalOrientation(const AZ::Vector3& normal)
    {
        AZ::Vector3 b1, b2;
        CalculateOrthonormalBasis(normal, b1, b2);
        const AZ::Matrix3x3 mat = AZ::Matrix3x3::CreateFromColumns(normal, b1, b2);
        return AZ::Quaternion::CreateFromMatrix3x3(mat);
    }
} // namespace WhiteBox
