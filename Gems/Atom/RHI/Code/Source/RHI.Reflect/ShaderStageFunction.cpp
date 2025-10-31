/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ::RHI
{
    void ShaderStageFunction::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderStageFunction>()
                ->Version(1)
                ->Field("m_hash", &ShaderStageFunction::m_hash)
                ->Field("m_shaderStage", &ShaderStageFunction::m_shaderStage);
        }
    }

    ShaderStageFunction::ShaderStageFunction(ShaderStage shaderStage)
        : m_shaderStage{shaderStage}
    {}

    ShaderStage ShaderStageFunction::GetShaderStage() const
    {
        return m_shaderStage;
    }

    HashValue64 ShaderStageFunction::GetHash() const
    {
        return m_hash;
    }

    void ShaderStageFunction::SetHash(HashValue64 hash)
    {
        m_hash = hash;
    }

    ResultCode ShaderStageFunction::Finalize()
    {
        if (m_shaderStage == ShaderStage::Unknown)
        {
            AZ_Error("ShaderStageFunction", false, "The shader stage is Unknown. This is not valid.");
            return ResultCode::InvalidArgument;
        }

        // Reset hash so we can validate the platform actually built it.
        m_hash = HashValue64{ 0 };

        const ResultCode resultCode = FinalizeInternal();

        // Do post finalize validation if the platform claims it succeeded.
        if (resultCode == ResultCode::Success)
        {
            if (m_hash == HashValue64{ 0 })
            {
                AZ_Error("ShaderStageFunction", false, "The hash value was not assigned in the platform Finalize implementation.");
                return ResultCode::Fail;
            }
        }

        return resultCode;
    }
}
