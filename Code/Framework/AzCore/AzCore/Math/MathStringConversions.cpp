/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathStringConversions.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

namespace AZStd
{
    void to_string(string& str, const AZ::Vector2& value)
    {
        str = AZStd::string::format("%.8f,%.8f", 
            static_cast<float>(value.GetX()), 
            static_cast<float>(value.GetY()));
    }

    void to_string(string& str, const AZ::Vector3& value)
    {
        str = AZStd::string::format("%.8f,%.8f,%.8f", 
            static_cast<float>(value.GetX()), 
            static_cast<float>(value.GetY()), 
            static_cast<float>(value.GetZ()));
    }

    void to_string(string& str, const AZ::Vector4& value)
    {
        str = AZStd::string::format("%.8f,%.8f,%.8f,%.8f", 
            static_cast<float>(value.GetX()), 
            static_cast<float>(value.GetY()), 
            static_cast<float>(value.GetZ()), 
            static_cast<float>(value.GetW()));
    }
    
    void to_string(string& str, const AZ::Quaternion& value)
    {
        str = AZStd::string::format("%.8f,%.8f,%.8f,%.8f",
            static_cast<float>(value.GetX()),
            static_cast<float>(value.GetY()),
            static_cast<float>(value.GetZ()),
            static_cast<float>(value.GetW()));
    }

    void to_string(string& str, const AZ::Matrix4x4& value)
    {
        str = AZStd::string::format("%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f", 
            static_cast<float>(value(0, 0)), static_cast<float>(value(1, 0)), static_cast<float>(value(2, 0)), static_cast<float>(value(3, 0)),
            static_cast<float>(value(0, 1)), static_cast<float>(value(1, 1)), static_cast<float>(value(2, 1)), static_cast<float>(value(3, 1)),
            static_cast<float>(value(0, 2)), static_cast<float>(value(1, 2)), static_cast<float>(value(2, 2)), static_cast<float>(value(3, 2)),
            static_cast<float>(value(0, 3)), static_cast<float>(value(1, 3)), static_cast<float>(value(2, 3)), static_cast<float>(value(3, 3)));
    }

    void to_string(string& str, const AZ::Transform& value)
    {
        AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromTransform(value);

        str = AZStd::string::format("%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f",
            static_cast<float>(matrix3x4(0, 0)), static_cast<float>(matrix3x4(1, 0)), static_cast<float>(matrix3x4(2, 0)),
            static_cast<float>(matrix3x4(0, 1)), static_cast<float>(matrix3x4(1, 1)), static_cast<float>(matrix3x4(2, 1)),
            static_cast<float>(matrix3x4(0, 2)), static_cast<float>(matrix3x4(1, 2)), static_cast<float>(matrix3x4(2, 2)),
            static_cast<float>(matrix3x4(0, 3)), static_cast<float>(matrix3x4(1, 3)), static_cast<float>(matrix3x4(2, 3)));
    }
}
