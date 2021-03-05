/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RHI.Reflect/Vulkan/Base.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <AtomCore/std/containers/array_view.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Vulkan
    {
        using ShaderByteCode = AZStd::vector<uint8_t>;
        using ShaderByteCodeView = AZStd::array_view<uint8_t>;

        /**
         * A set of indices used to access physical sub-stages within a virtual stage.
         */
        namespace ShaderSubStage
        {
            /// Used when the sub-stage is 1-to-1 with the virtual stage.
            const uint32_t Default = 0;

            /// Tessellation is composed of two physical stages in Vulkan.
            const uint32_t TesselationControl = 0;
            const uint32_t TesselationEvaluattion = 1;
        }
        
        const uint32_t ShaderSubStageCountMax = 2;

        class ShaderStageFunction
            : public RHI::ShaderStageFunction
        {
            using Base = RHI::ShaderStageFunction;
            
        public:
            AZ_RTTI(ShaderStageFunction, "{A606478A-97E9-402D-A776-88EE72DAC6F9}", RHI::ShaderStageFunction);
            AZ_CLASS_ALLOCATOR(ShaderStageFunction, AZ::SystemAllocator, 0);
            
            static void Reflect(AZ::ReflectContext* context);

            static RHI::Ptr<ShaderStageFunction> Create(RHI::ShaderStage shaderStage);

            /// Assigns byte code to the function.
            void SetByteCode(uint32_t subStageIndex, const ShaderByteCode& byteCode, const AZStd::string_view& entryFunctionName);

            /// Returns the assigned bytecode.
            ShaderByteCodeView GetByteCode(uint32_t subStageIndex) const;

            /// Return the entry function name.
            const AZStd::string& GetEntryFunctionName(uint32_t subStageIndex) const;

        private:
            ShaderStageFunction() = default;
            ShaderStageFunction(RHI::ShaderStage shaderStage);
            AZ_SERIALIZE_FRIEND();

            ///////////////////////////////////////////////////////////////////
            // RHI::ShaderStageFunction
            RHI::ResultCode FinalizeInternal() override;
            ///////////////////////////////////////////////////////////////////

            AZStd::array<ShaderByteCode, ShaderSubStageCountMax> m_byteCodes;
            AZStd::array<AZStd::string, ShaderSubStageCountMax> m_entryFunctionNames;
        };
    }
}
