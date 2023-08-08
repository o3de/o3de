/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    // Note: All the classes below that start with "ShaderConstantMapping" consist of a value of 
    // a given type and a ShaderInputConstantId used to map that value to a shader resource group.

    // Color mapping for a shader constant
    struct ShaderDataMappingColor
    {
        AZ_TYPE_INFO(ShaderDataMappingColor, "{C71FE47C-FD3E-4032-A842-99D9BF86CE19}");
        static void Reflect(AZ::ReflectContext* context);

        Name m_name;
        Color m_value;
    };
    using ShaderMappingsColor = AZStd::vector<ShaderDataMappingColor>;

    // Uint mapping for a shader constant
    struct ShaderDataMappingUint
    {
        AZ_TYPE_INFO(ShaderDataMappingUint, "{1ECEAEF0-AA15-4836-8A48-402CC66E7651}");
        static void Reflect(AZ::ReflectContext* context);

        Name m_name;
        uint32_t m_value = 0;
    };
    using ShaderMappingsUint = AZStd::vector<ShaderDataMappingUint>;

    // Float mapping for a shader constant
    struct ShaderDataMappingFloat
    {
        AZ_TYPE_INFO(ShaderDataMappingFloat, "{AE684716-7F84-4DBC-B21F-82664E88F1AD}");
        static void Reflect(AZ::ReflectContext* context);

        Name m_name;
        float m_value = 0;
    };
    using ShaderMappingsFloat = AZStd::vector<ShaderDataMappingFloat>;

    // Float2 mapping for a shader constant
    struct ShaderDataMappingFloat2
    {
        AZ_TYPE_INFO(ShaderDataMappingFloat2, "{A8D3B3CC-B00E-44FE-9038-0C9CEE0F606E}");
        static void Reflect(AZ::ReflectContext* context);

        Name m_name;
        Vector2 m_value;
    };
    using ShaderMappingsFloat2 = AZStd::vector<ShaderDataMappingFloat2>;

    // Float3 mapping for a shader constant
    struct ShaderDataMappingFloat3
    {
        AZ_TYPE_INFO(ShaderDataMappingFloat3, "{E23F423D-7200-49CB-8048-9C3D7FCC3AAD}");
        static void Reflect(AZ::ReflectContext* context);

        Name m_name;
        Vector3 m_value;
    };
    using ShaderMappingsFloat3 = AZStd::vector<ShaderDataMappingFloat3>;

    // Float4 mapping for a shader constant
    struct ShaderDataMappingFloat4
    {
        AZ_TYPE_INFO(ShaderDataMappingFloat4, "{16FEB44A-FE90-4406-923A-3A5E1C849E97}");
        static void Reflect(AZ::ReflectContext* context);

        Name m_name;
        Vector4 m_value;
    };
    using ShaderMappingsFloat4 = AZStd::vector<ShaderDataMappingFloat4>;

    // Matrix3x3 mapping for a shader constant
    struct ShaderDataMappingMatrix3x3
    {
        AZ_TYPE_INFO(ShaderDataMappingMatrix3x3, "{86EA4178-F8CA-4AF3-8D6E-F91A1875608D}");
        static void Reflect(AZ::ReflectContext* context);

        Name m_name;
        Matrix3x3 m_value;
    };
    using ShaderMappingsMatrix3x3 = AZStd::vector<ShaderDataMappingMatrix3x3>;

    // Matrix4x4 mapping for a shader constant
    struct ShaderDataMappingMatrix4x4
    {
        AZ_TYPE_INFO(ShaderDataMappingMatrix4x4, "{7DBD1EFA-CA1B-4950-B9CB-C36426C30535}");
        static void Reflect(AZ::ReflectContext* context);

        Name m_name;
        Matrix4x4 m_value;
    };
    using ShaderMappingsMatrix4x4 = AZStd::vector<ShaderDataMappingMatrix4x4>;

    // [GFX TODO][ATOM-2338] Revisit this class and finalize the API around it's usage.
    // Collection of values and the names of the shader constants they map to
    // Consists of lists for each of the mapping types declared above
    struct ShaderDataMappings
    {
        AZ_TYPE_INFO(ShaderDataMappings, "{0C522693-ED7D-4E46-8C8F-1171994D9EEF}");
        static void Reflect(AZ::ReflectContext* context);

        ShaderMappingsColor m_colorMappings;
        ShaderMappingsUint m_uintMappings;
        ShaderMappingsFloat m_floatMappings;
        ShaderMappingsFloat2 m_float2Mappings;
        ShaderMappingsFloat3 m_float3Mappings;
        ShaderMappingsFloat4 m_float4Mappings;
        ShaderMappingsMatrix3x3 m_matrix3x3Mappings;
        ShaderMappingsMatrix4x4 m_matrix4x4Mappings;
    };
}
