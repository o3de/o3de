/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/MathStringConversions.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Matrix3x3.h>
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
        AZ::Locale::ScopedSerializationLocale locale; // make sure that %f is using the "C" locale (periods instead of commas)

        str = AZStd::string::format("%.8f,%.8f", 
            static_cast<float>(value.GetX()), 
            static_cast<float>(value.GetY()));
    }

    void to_string(string& str, const AZ::Vector3& value)
    {
        AZ::Locale::ScopedSerializationLocale locale;

        str = AZStd::string::format("%.8f,%.8f,%.8f", 
            static_cast<float>(value.GetX()), 
            static_cast<float>(value.GetY()), 
            static_cast<float>(value.GetZ()));
    }

    void to_string(string& str, const AZ::Vector4& value)
    {
        AZ::Locale::ScopedSerializationLocale locale;

        str = AZStd::string::format("%.8f,%.8f,%.8f,%.8f", 
            static_cast<float>(value.GetX()), 
            static_cast<float>(value.GetY()), 
            static_cast<float>(value.GetZ()), 
            static_cast<float>(value.GetW()));
    }
    
    void to_string(string& str, const AZ::Quaternion& value)
    {
        AZ::Locale::ScopedSerializationLocale locale;

        str = AZStd::string::format("%.8f,%.8f,%.8f,%.8f",
            static_cast<float>(value.GetX()),
            static_cast<float>(value.GetY()),
            static_cast<float>(value.GetZ()),
            static_cast<float>(value.GetW()));
    }

    void to_string(string& str, const AZ::Matrix3x3& value)
    {
        AZ::Locale::ScopedSerializationLocale locale;

        str = AZStd::string::format(
            "%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f",
            static_cast<float>(value(0, 0)), static_cast<float>(value(1, 0)), static_cast<float>(value(2, 0)),
            static_cast<float>(value(0, 1)), static_cast<float>(value(1, 1)), static_cast<float>(value(2, 1)),
            static_cast<float>(value(0, 2)), static_cast<float>(value(1, 2)), static_cast<float>(value(2, 2)));
    }

    void to_string(string& str, const AZ::Matrix4x4& value)
    {
        AZ::Locale::ScopedSerializationLocale locale;

        str = AZStd::string::format("%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f,%.8f", 
            static_cast<float>(value(0, 0)), static_cast<float>(value(1, 0)), static_cast<float>(value(2, 0)), static_cast<float>(value(3, 0)),
            static_cast<float>(value(0, 1)), static_cast<float>(value(1, 1)), static_cast<float>(value(2, 1)), static_cast<float>(value(3, 1)),
            static_cast<float>(value(0, 2)), static_cast<float>(value(1, 2)), static_cast<float>(value(2, 2)), static_cast<float>(value(3, 2)),
            static_cast<float>(value(0, 3)), static_cast<float>(value(1, 3)), static_cast<float>(value(2, 3)), static_cast<float>(value(3, 3)));
    }

    void to_string(string& str, const AZ::Transform& value)
    {
        AZ::Locale::ScopedSerializationLocale locale;

        AZ::Matrix3x4 matrix3x4 = AZ::Matrix3x4::CreateFromTransform(value);

        str = AZStd::string::format("%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f",
            static_cast<float>(matrix3x4(0, 0)), static_cast<float>(matrix3x4(1, 0)), static_cast<float>(matrix3x4(2, 0)),
            static_cast<float>(matrix3x4(0, 1)), static_cast<float>(matrix3x4(1, 1)), static_cast<float>(matrix3x4(2, 1)),
            static_cast<float>(matrix3x4(0, 2)), static_cast<float>(matrix3x4(1, 2)), static_cast<float>(matrix3x4(2, 2)),
            static_cast<float>(matrix3x4(0, 3)), static_cast<float>(matrix3x4(1, 3)), static_cast<float>(matrix3x4(2, 3)));
    }

    void to_string(string& str, const AZ::Aabb& value)
    {
        AZ::Locale::ScopedSerializationLocale locale;

        str = AZStd::string::format(
            "%.8f,%.8f,%.8f\n%.8f,%.8f,%.8f",
            static_cast<float>(value.GetMin().GetX()), static_cast<float>(value.GetMin().GetY()),
            static_cast<float>(value.GetMin().GetZ()), static_cast<float>(value.GetMax().GetX()),
            static_cast<float>(value.GetMax().GetY()), static_cast<float>(value.GetMax().GetZ()));
    }

    void to_string(string& str, const AZ::Color& color)
    {
        AZ::Locale::ScopedSerializationLocale locale;

        str = AZStd::string::format("R:%d, G:%d, B:%d A:%d", color.GetR8(), color.GetG8(), color.GetB8(), color.GetA8());
    }
}
