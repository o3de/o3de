/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/ClearValue.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    void ClearDepthStencil::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ClearDepthStencil>()
                ->Version(0)
                ->Field("Depth", &ClearDepthStencil::m_depth)
                ->Field("Stencil", &ClearDepthStencil::m_stencil)
                ;
        }
    }

    void ClearValue::Reflect(AZ::ReflectContext* context)
    {
        ClearDepthStencil::Reflect(context);

        if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Enum<ClearValueType>()
                ->Value("Vector4Float", ClearValueType::Vector4Float)
                ->Value("Vector4Uint", ClearValueType::Vector4Uint)
                ->Value("DepthStencil", ClearValueType::DepthStencil)
                ;

            serializeContext->Class<ClearValue>()
                ->Version(2)
                ->Field("Type", &ClearValue::m_type)
                ->Field("Value", &ClearValue::m_vector4Float)
                ->Field("UintValue", &ClearValue::m_vector4Uint)
                ->Field("DepthStencilValue", &ClearValue::m_depthStencil)
                ;
        }
    }

    ClearValue::ClearValue()
    {
        memset(this, 0, sizeof(ClearValue));
        m_type = ClearValueType::Vector4Float;
    }

    HashValue64 ClearValue::GetHash(HashValue64 seed /* = 0 */) const
    {
        return TypeHash64(*this, seed);
    }

    ClearValue ClearValue::CreateVector4Float(float x, float y, float z, float w)
    {
        ClearValue value;
        value.m_type = ClearValueType::Vector4Float;
        value.m_vector4Float[0] = x;
        value.m_vector4Float[1] = y;
        value.m_vector4Float[2] = z;
        value.m_vector4Float[3] = w;
        return value;
    }

    ClearValue ClearValue::CreateVector4Uint(uint32_t x, uint32_t y, uint32_t z, uint32_t w)
    {
        ClearValue value;
        value.m_type = ClearValueType::Vector4Uint;
        value.m_vector4Uint[0] = x;
        value.m_vector4Uint[1] = y;
        value.m_vector4Uint[2] = z;
        value.m_vector4Uint[3] = w;
        return value;
    }

    ClearValue ClearValue::CreateStencil(uint8_t stencil)
    {
        ClearValue value;
        value.m_type = ClearValueType::DepthStencil;
        value.m_depthStencil.m_depth = 0.0;
        value.m_depthStencil.m_stencil = stencil;
        return value;
    }

    ClearValue ClearValue::CreateDepth(float depth)
    {
        ClearValue value;
        value.m_type = ClearValueType::DepthStencil;
        value.m_depthStencil.m_depth = depth;
        value.m_depthStencil.m_stencil = 0;
        return value;
    }

    ClearValue ClearValue::CreateDepthStencil(float depth, uint8_t stencil)
    {
        ClearValue value;
        value.m_type = ClearValueType::DepthStencil;
        value.m_depthStencil.m_depth = depth;
        value.m_depthStencil.m_stencil = stencil;
        return value;
    }
    
    bool ClearValue::operator==(const ClearValue& other) const
    {
        return
            m_type == other.m_type &&
            m_depthStencil == other.m_depthStencil &&
            m_vector4Uint == other.m_vector4Uint &&
            AZ::IsClose(m_vector4Float[0], other.m_vector4Float[0], std::numeric_limits<float>::epsilon()) &&
            AZ::IsClose(m_vector4Float[1], other.m_vector4Float[1], std::numeric_limits<float>::epsilon()) &&
            AZ::IsClose(m_vector4Float[2], other.m_vector4Float[2], std::numeric_limits<float>::epsilon()) &&
            AZ::IsClose(m_vector4Float[3], other.m_vector4Float[3], std::numeric_limits<float>::epsilon());
    }
}
