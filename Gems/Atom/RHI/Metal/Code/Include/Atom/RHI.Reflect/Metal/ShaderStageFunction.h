/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Metal/Base.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <AtomCore/std/containers/array_view.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Metal
    {

        using ShaderSourceCode = AZStd::vector<char>;
        using ShaderByteCode = AZStd::vector<uint8_t>;
        
        class ShaderStageFunction
            : public RHI::ShaderStageFunction
        {
        public:
            AZ_RTTI(ShaderStageFunction, "{44E51B8E-CFEE-4A63-8DC2-65CDCA0E373B}", RHI::ShaderStageFunction);
            AZ_CLASS_ALLOCATOR(ShaderStageFunction, AZ::SystemAllocator, 0);
            
            static void Reflect(AZ::ReflectContext* context);
            
            static RHI::Ptr<ShaderStageFunction> Create(RHI::ShaderStage shaderStage);

            /// Assigns source code to the function.
            void SetSourceCode(const ShaderSourceCode& sourceCode);

            /// Returns the assigned source code.
            const AZStd::string& GetSourceCode() const;

            /// Assigns byte code and byte code length.
            void SetByteCode(const ShaderByteCode& byteCode);
            
            /// Assigns entry function name.
            void SetEntryFunctionName(AZStd::string_view entryFunctionName);
            
            /// Returns the assigned bytecode.
            const ShaderByteCode& GetByteCode() const;
            
            /// Return the entry function name.
            const AZStd::string& GetEntryFunctionName() const;
            
            /// Return the size of byte code length.
            const uint32_t GetByteCodeLength() const;
            
        private:
            /// Default constructor for serialization. Assumes shader stage is bound by data.
            ShaderStageFunction() = default;
            /// Used for manual construction. Shader stage must be provided.
            ShaderStageFunction(RHI::ShaderStage shaderStage);
            AZ_SERIALIZE_FRIEND();
            
            ///////////////////////////////////////////////////////////////////
            // RHI::ShaderStageFunction
            RHI::ResultCode FinalizeInternal() override;
            ///////////////////////////////////////////////////////////////////

            AZStd::string m_sourceCode;
            ShaderByteCode m_byteCode;
            uint32_t m_byteCodeLength;
            AZStd::string m_entryFunctionName;
            
        };
    }
}
