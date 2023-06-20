/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <NumericalMethods/DoublePrecisionMath/Quaternion.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Casting/numeric_cast.h>

namespace NumericalMethods::DoublePrecisionMath
{
    Quaternion::Quaternion()
        : m_x(0.0)
        , m_y(0.0)
        , m_z(0.0)
        , m_w(1.0)
    {
    }

    Quaternion::Quaternion(double x, double y, double z, double w)
        : m_x(x)
        , m_y(y)
        , m_z(z)
        , m_w(w)
    {
    }

    Quaternion::Quaternion(const AZ::Quaternion& q)
        : m_x(q.GetX())
        , m_y(q.GetY())
        , m_z(q.GetZ())
        , m_w(q.GetW())
    {
    }

    AZ::Quaternion Quaternion::ToSingle() const
    {
        return AZ::Quaternion(
            aznumeric_cast<float>(m_x), aznumeric_cast<float>(m_y), aznumeric_cast<float>(m_z), aznumeric_cast<float>(m_w));
    }

    double Quaternion::GetX() const
    {
        return m_x;
    }

    double Quaternion::GetY() const
    {
        return m_y;
    }

    double Quaternion::GetZ() const
    {
        return m_z;
    }

    double Quaternion::GetW() const
    {
        return m_w;
    }

    Quaternion Quaternion::operator*(const Quaternion& rhs) const
    {
        return Quaternion(
            +m_x * rhs.m_w + m_y * rhs.m_z - m_z * rhs.m_y + m_w * rhs.m_x,
            -m_x * rhs.m_z + m_y * rhs.m_w + m_z * rhs.m_x + m_w * rhs.m_y,
            +m_x * rhs.m_y - m_y * rhs.m_x + m_z * rhs.m_w + m_w * rhs.m_z,
            -m_x * rhs.m_x - m_y * rhs.m_y - m_z * rhs.m_z + m_w * rhs.m_w);
    }

    Quaternion Quaternion::operator-() const
    {
        return Quaternion(-m_x, -m_y, -m_z, -m_w);
    }

    Quaternion Quaternion::GetNormalized() const
    {
        double magSq = m_x * m_x + m_y * m_y + m_z * m_z + m_w * m_w;
        if (magSq < 1e-20)
        {
            return Quaternion();
        }
        else
        {
            double mag = std::sqrt(magSq);
            return Quaternion(m_x / mag, m_y / mag, m_z / mag, m_w / mag);
        }
    }

    Quaternion Quaternion::GetConjugate() const
    {
        return Quaternion(-m_x, -m_y, -m_z, m_w);
    }
} // namespace NumericalMethods::DoublePrecisionMath
