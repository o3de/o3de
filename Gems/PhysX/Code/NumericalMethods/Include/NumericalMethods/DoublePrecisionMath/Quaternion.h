/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Math/Quaternion.h>

namespace NumericalMethods::DoublePrecisionMath
{
    class Quaternion
    {
    public:
        Quaternion();
        Quaternion(double x, double y, double z, double w);
        Quaternion(const AZ::Quaternion& q);
        AZ::Quaternion ToSingle() const;
        double GetX() const;
        double GetY() const;
        double GetZ() const;
        double GetW() const;
        Quaternion operator*(const Quaternion& rhs) const;
        Quaternion operator-() const;
        Quaternion GetNormalized() const;
        Quaternion GetConjugate() const;

    private:
        double m_x;
        double m_y;
        double m_z;
        double m_w;
    };
} // namespace NumericalMethods::DoublePrecisionMath
