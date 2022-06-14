/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/span.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/RTTI/TypeInfo.h>

//! This file holds useful material related utility functions.

namespace AZ
{
    namespace Render
    {
        enum class TransformType
        {
            Invalid,
            Scale,
            Rotate,
            Translate
        };

        struct UvTransformDescriptor
        {
            Vector2 m_center{ Vector2::CreateZero() };
            float m_scale{ 1.0f };
            float m_scaleX{ 1.0f };
            float m_scaleY{ 1.0f };
            float m_translateX{ 0.0f };
            float m_translateY{ 0.0f };
            float m_rotateDegrees{ 0.0f };
        };

        // Create a 3x3 uv transform matrix from a set of input properties.
        Matrix3x3 CreateUvTransformMatrix(const UvTransformDescriptor& desc, const AZStd::span<const TransformType> transformOrder);
    }

    AZ_TYPE_INFO_SPECIALIZE(Render::TransformType, "{D8C15D33-CE3D-4297-A646-030B0625BF84}");
}
