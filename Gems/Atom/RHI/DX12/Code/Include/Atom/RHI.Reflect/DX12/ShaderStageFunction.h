/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::Serialize
{
    template<class T, bool U, bool A>
    struct InstanceFactory;
}
namespace AZ
{
    template<typename ValueType, typename>
    struct AnyTypeInfoConcept;
}

namespace AZ
{
    class ReflectContext;

    namespace DX12
    {
        using ShaderByteCode = AZStd::vector<uint8_t>;
        using ShaderByteCodeView = AZStd::span<const uint8_t>;

        /**
         * A set of indices used to access physical sub-stages within a virtual stage.
         */
        namespace ShaderSubStage
        {
            /// Used when the sub-stage is 1-to-1 with the virtual stage.
            const uint32_t Default = 0;

            /// Tessellation is composed of two physical stages in HLSL.
            const uint32_t TessellationHull = 0;
            const uint32_t TessellationDomain = 1;
        }

        const uint32_t ShaderSubStageCountMax = 2;

        class ShaderStageFunction
            : public RHI::ShaderStageFunction
        {
        public:
            AZ_RTTI(ShaderStageFunction, "{1BAEE536-96CA-4AEB-BA73-D5D72EE35B45}", RHI::ShaderStageFunction);
            AZ_CLASS_ALLOCATOR(ShaderStageFunction, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            static RHI::Ptr<ShaderStageFunction> Create(RHI::ShaderStage shaderStage);

            /// Assigns byte code to the function.
            void SetByteCode(uint32_t subStageIndex, const ShaderByteCode& byteCode);

            /// Returns the assigned byte code.
            ShaderByteCodeView GetByteCode(uint32_t subStageIndex = 0) const;

        private:
            ShaderStageFunction() = default;
            ShaderStageFunction(RHI::ShaderStage shaderStage);
            template <typename, typename>
            friend struct AnyTypeInfoConcept;
            template <typename, bool, bool>
            friend struct Serialize::InstanceFactory;

            ///////////////////////////////////////////////////////////////////
            // RHI::ShaderStageFunction
            RHI::ResultCode FinalizeInternal() override;
            ///////////////////////////////////////////////////////////////////

            AZStd::array<ShaderByteCode, ShaderSubStageCountMax> m_byteCodes;
        };
    }
}
