/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/WebGPU/ShaderStageFunction.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::WebGPU
{
    void ShaderStageFunction::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderStageFunction, RHI::ShaderStageFunction>()
                ->Version(1)
                ->Field("m_sourceCode", &ShaderStageFunction::m_sourceCode)
                ->Field("m_entryFunctionName", &ShaderStageFunction::m_entryFunctionName);
        }
    }

    ShaderStageFunction::ShaderStageFunction(RHI::ShaderStage shaderStage)
        : RHI::ShaderStageFunction(shaderStage)
    {}
    
    RHI::Ptr<ShaderStageFunction> ShaderStageFunction::Create(RHI::ShaderStage shaderStage)
    {
        return aznew ShaderStageFunction(shaderStage);
    }

    void ShaderStageFunction::SetSourceCode(const ShaderSourceCode& sourceCode)
    {
        m_sourceCode = sourceCode;
    }

    const ShaderSourceCode& ShaderStageFunction::GetSourceCode() const
    {
        return m_sourceCode;
    }

    void ShaderStageFunction::SetEntryFunctionName(AZStd::string_view entryFunctionName)
    {
        m_entryFunctionName = entryFunctionName;
    }

    const AZStd::string& ShaderStageFunction::GetEntryFunctionName() const
    {
        return m_entryFunctionName;
    }

    RHI::ResultCode ShaderStageFunction::FinalizeInternal()
    {
        HashValue64 hash = HashValue64{ 0 };
        hash = TypeHash64(reinterpret_cast<const uint8_t*>(m_sourceCode.data()), m_sourceCode.size(), hash);
        hash = TypeHash64(reinterpret_cast<const uint8_t*>(m_entryFunctionName.data()), m_entryFunctionName.size(), hash);
        SetHash(hash);
        return RHI::ResultCode::Success;
    }
} 
