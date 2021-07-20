/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/DX12/ShaderStageFunction.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace DX12
    {
        void ShaderStageFunction::Reflect(ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderStageFunction, RHI::ShaderStageFunction>()
                    ->Version(1)
                    ->Field("m_byteCodes", &ShaderStageFunction::m_byteCodes);
            }
        }

        ShaderStageFunction::ShaderStageFunction(RHI::ShaderStage shaderStage)
            : RHI::ShaderStageFunction(shaderStage)
        {}


        RHI::Ptr<ShaderStageFunction> ShaderStageFunction::Create(RHI::ShaderStage shaderStage)
        {
            return aznew ShaderStageFunction(shaderStage);
        }

        void ShaderStageFunction::SetByteCode(uint32_t subStageIndex, const ShaderByteCode& byteCode)
        {
            m_byteCodes[subStageIndex] = byteCode;
        }

        ShaderByteCodeView ShaderStageFunction::GetByteCode(uint32_t subStageIndex) const
        {
            return ShaderByteCodeView(m_byteCodes[subStageIndex]);
        }

        RHI::ResultCode ShaderStageFunction::FinalizeInternal()
        {
            bool emptyByteCodes = true;
            for (const ShaderByteCode& byteCode : m_byteCodes)
            {
                emptyByteCodes &= !byteCode.empty();
            }

            if (emptyByteCodes)
            {
                AZ_Error("ShaderStageFunction", false, "Finalizing shader stage function with empty bytecodes.");
                return RHI::ResultCode::InvalidArgument;
            }

            HashValue64 hash = HashValue64{ 0 };
            for (const ShaderByteCode& byteCode : m_byteCodes)
            {
                if (!byteCode.empty())
                {
                    hash = TypeHash64(byteCode.data(), byteCode.size(), hash);
                }
            }
            SetHash(hash);
            return RHI::ResultCode::Success;
        }
    }
}
