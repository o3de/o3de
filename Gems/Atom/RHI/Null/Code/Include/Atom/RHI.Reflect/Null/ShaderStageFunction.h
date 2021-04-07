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

#include <Atom/RHI.Reflect/Null/Base.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Null
    {
        class ShaderStageFunction
            : public RHI::ShaderStageFunction
        {
        public:
            AZ_RTTI(ShaderStageFunction, "{BA5D7597-6CFF-4521-B438-BEAD638E5FF8}", RHI::ShaderStageFunction);
            AZ_CLASS_ALLOCATOR(ShaderStageFunction, AZ::SystemAllocator, 0);
            
            static void Reflect(AZ::ReflectContext* context);
            
            static RHI::Ptr<ShaderStageFunction> Create(RHI::ShaderStage shaderStage);
            
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
            
        };
    }
}
