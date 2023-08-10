/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/ShaderDataMappings.h>

namespace AZ::RHI
{
    void ShaderDataMappingColor::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShaderDataMappingColor>()
                ->Version(0)
                ->Field("Name", &ShaderDataMappingColor::m_name)
                ->Field("Value", &ShaderDataMappingColor::m_value);
        }
    }

    void ShaderDataMappingUint::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShaderDataMappingUint>()
                ->Version(0)
                ->Field("Name", &ShaderDataMappingUint::m_name)
                ->Field("Value", &ShaderDataMappingUint::m_value);
        }
    }

    void ShaderDataMappingFloat::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShaderDataMappingFloat>()
                ->Version(0)
                ->Field("Name", &ShaderDataMappingFloat::m_name)
                ->Field("Value", &ShaderDataMappingFloat::m_value);
        }
    }

    void ShaderDataMappingFloat2::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShaderDataMappingFloat2>()
                ->Version(0)
                ->Field("Name", &ShaderDataMappingFloat2::m_name)
                ->Field("Value", &ShaderDataMappingFloat2::m_value);
        }
    }

    void ShaderDataMappingFloat3::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShaderDataMappingFloat3>()
                ->Version(0)
                ->Field("Name", &ShaderDataMappingFloat3::m_name)
                ->Field("Value", &ShaderDataMappingFloat3::m_value);
        }
    }

    void ShaderDataMappingFloat4::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShaderDataMappingFloat4>()
                ->Version(0)
                ->Field("Name", &ShaderDataMappingFloat4::m_name)
                ->Field("Value", &ShaderDataMappingFloat4::m_value);
        }
    }

    void ShaderDataMappingMatrix3x3::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShaderDataMappingMatrix3x3>()
                ->Version(0)
                ->Field("Name", &ShaderDataMappingMatrix3x3::m_name)
                ->Field("Value", &ShaderDataMappingMatrix3x3::m_value);
        }
    }

    void ShaderDataMappingMatrix4x4::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ShaderDataMappingMatrix4x4>()
                ->Version(0)
                ->Field("Name", &ShaderDataMappingMatrix4x4::m_name)
                ->Field("Value", &ShaderDataMappingMatrix4x4::m_value);
        }
    }

    void ShaderDataMappings::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            ShaderDataMappingColor::Reflect(context);
            ShaderDataMappingUint::Reflect(context);
            ShaderDataMappingFloat::Reflect(context);
            ShaderDataMappingFloat2::Reflect(context);
            ShaderDataMappingFloat3::Reflect(context);
            ShaderDataMappingFloat4::Reflect(context);
            ShaderDataMappingMatrix3x3::Reflect(context);
            ShaderDataMappingMatrix4x4::Reflect(context);

            serializeContext->Class<ShaderDataMappings>()
                ->Version(0)
                ->Field("ColorMappings", &ShaderDataMappings::m_colorMappings)
                ->Field("UintMappings", &ShaderDataMappings::m_uintMappings)
                ->Field("FloatMappings", &ShaderDataMappings::m_floatMappings)
                ->Field("Float2Mappings", &ShaderDataMappings::m_float2Mappings)
                ->Field("Float3Mappings", &ShaderDataMappings::m_float3Mappings)
                ->Field("Float4Mappings", &ShaderDataMappings::m_float4Mappings)
                ->Field("Matrix3x3Mappings", &ShaderDataMappings::m_matrix3x3Mappings)
                ->Field("Matrix4x4Mappings", &ShaderDataMappings::m_matrix4x4Mappings)
                ;
        }
    }
}
