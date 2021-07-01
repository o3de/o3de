/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RHI.Reflect/ShaderStages.h>
#include <Atom/RHI/Object.h>

namespace AZ
{
    class ReflectContext;

    namespace Vulkan
    {
        using ShaderByteCode = AZStd::vector<uint8_t>;

        // TODO: remove this class after checking it is no longer necessary.
        class ShaderDescriptor final
            : public RHI::Object
        {
            using Base = RHI::Object;
        public:
            AZ_CLASS_ALLOCATOR(ShaderDescriptor, AZ::SystemAllocator, 0);
            AZ_RTTI(ShaderDescriptor, "EB289A24-52DF-45E5-B3D0-C33B6DBAAAA7", Base);

            static void Reflect(AZ::ReflectContext* context);

            /// Clears all bytecodes and resets descriptor
            void Clear();

            /// Finalizes the descriptor and builds the hash value.
            void Finalize();

            /// Assigns bytecode to a specific shader stage.
            void SetByteCode(RHI::ShaderStage shaderStage, const ShaderByteCode& bytecode);

            /// Returns whether bytecode exists for the given shader stage.
            bool HasByteCode(RHI::ShaderStage shaderStage) const;

            /// Returns the bytecode for the given shader stage.
            const ShaderByteCode& GetByteCode(RHI::ShaderStage shaderStage) const;

        private:
            /// The set of shader bytecodes indexed by shader stage.
            AZStd::array<ShaderByteCode, static_cast<size_t>(RHI::ShaderStage::GraphicsCount)> m_byteCodesByStage;
            size_t m_hash = 0;
        };

    }
}
