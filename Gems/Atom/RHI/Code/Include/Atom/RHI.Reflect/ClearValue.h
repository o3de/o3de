/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    enum class ClearValueType : uint32_t
    {
        Vector4Float = 0,
        Vector4Uint,
        DepthStencil
    };

    struct ClearDepthStencil
    {
        AZ_TYPE_INFO(ClearDepthStencil, "{CDD1AA45-DDBC-452E-92BF-BAD140A668E0}");
        static void Reflect(AZ::ReflectContext* context);

        bool operator==(const ClearDepthStencil& other) const
        {
            return
                AZ::IsClose(m_depth, other.m_depth, std::numeric_limits<float>::epsilon()) &&
                m_stencil == other.m_stencil;
        }
            
        float m_depth = 0.0f;
        uint8_t m_stencil = 0;
    };

    //! Represents either a depth stencil, a float vector, or a uint vector clear value.
    struct ClearValue
    {
        AZ_TYPE_INFO(ClearValue, "{a64f14ac-3012-4fd6-9224-4cd046eff2e2}");

        static void Reflect(AZ::ReflectContext* context);

        ClearValue();

        static ClearValue CreateDepth(float depth);
        static ClearValue CreateStencil(uint8_t stencil);
        static ClearValue CreateDepthStencil(float depth, uint8_t stencil);
        static ClearValue CreateVector4Float(float x, float y, float z, float w);
        static ClearValue CreateVector4Uint(uint32_t x, uint32_t y, uint32_t z, uint32_t w);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;
        bool operator==(const ClearValue& other) const;
            
        ClearValueType m_type;

        // Note: these used to be a union, but unions don't allow for proper serialization, so the union was removed.
        ClearDepthStencil m_depthStencil;
        AZStd::array<float, 4> m_vector4Float;
        AZStd::array<uint32_t, 4> m_vector4Uint;
    };

    AZ_TYPE_INFO_SPECIALIZE(ClearValueType, "{EBA6E553-1FAE-47FC-9329-15DED520AEDC}");
}
