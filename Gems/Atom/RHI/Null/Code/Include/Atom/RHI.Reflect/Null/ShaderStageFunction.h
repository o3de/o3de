/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
