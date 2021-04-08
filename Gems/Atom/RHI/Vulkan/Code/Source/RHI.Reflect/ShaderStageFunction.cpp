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

#include <Atom/RHI.Reflect/Vulkan/ShaderStageFunction.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace Vulkan
    {
        void ShaderStageFunction::Reflect(ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderStageFunction, RHI::ShaderStageFunction>()
                    ->Version(2)
                    ->Field("m_byteCodes", &ShaderStageFunction::m_byteCodes)
                    ->Field("m_entryFunctionName", &ShaderStageFunction::m_entryFunctionNames);
            }
        }

        ShaderStageFunction::ShaderStageFunction(RHI::ShaderStage shaderStage)
            : RHI::ShaderStageFunction(shaderStage)
        {}


        RHI::Ptr<ShaderStageFunction> ShaderStageFunction::Create(RHI::ShaderStage shaderStage)
        {
            return aznew ShaderStageFunction(shaderStage);
        }

        void ShaderStageFunction::SetByteCode(uint32_t subStageIndex, const ShaderByteCode& byteCode, const AZStd::string_view& entryFunctionName)
        {
            AZ_Assert(subStageIndex < ShaderSubStageCountMax, "SubStage index is out of bound.");
            m_byteCodes[subStageIndex] = byteCode;
            m_entryFunctionNames[subStageIndex] = entryFunctionName;
        }

        ShaderByteCodeView ShaderStageFunction::GetByteCode(uint32_t subStageIndex) const
        {
            AZ_Assert(subStageIndex < ShaderSubStageCountMax, "SubStage index is out of bound.");
            return ShaderByteCodeView(m_byteCodes[subStageIndex]);
        }

        const AZStd::string& ShaderStageFunction::GetEntryFunctionName(uint32_t subStageIndex) const
        {
            AZ_Assert(subStageIndex < ShaderSubStageCountMax, "SubStage index is out of bound.");
            return m_entryFunctionNames[subStageIndex];
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
