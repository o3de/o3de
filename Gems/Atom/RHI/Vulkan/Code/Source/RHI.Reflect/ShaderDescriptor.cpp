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
#include <Atom/RHI.Reflect/Vulkan/ShaderDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Vulkan
    {
        void ShaderDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderDescriptor, Base>()
                    ->Version(1)
                    ->Field("m_byteCodesByStage", &ShaderDescriptor::m_byteCodesByStage);
            }
        }

        void ShaderDescriptor::Clear()
        {
            m_hash = 0;
            m_byteCodesByStage.fill(ShaderByteCode());
        }

        void ShaderDescriptor::Finalize()
        {
            AZ::Crc32 crc;
            for (const ShaderByteCode& byteCode : m_byteCodesByStage)
            {
                if (!byteCode.empty())
                {
                    crc.Add(byteCode.data(), byteCode.size());
                }
            }
            m_hash = crc;
        }

        void ShaderDescriptor::SetByteCode(RHI::ShaderStage shaderStage, const ShaderByteCode& bytecode)
        {
            m_byteCodesByStage[static_cast<uint32_t>(shaderStage)] = bytecode;
        }

        bool ShaderDescriptor::HasByteCode(RHI::ShaderStage shaderStage) const
        {
            return !(m_byteCodesByStage[static_cast<uint32_t>(shaderStage)].empty());
        }

        const ShaderByteCode& ShaderDescriptor::GetByteCode(RHI::ShaderStage shaderStage) const
        {
            return m_byteCodesByStage[static_cast<uint32_t>(shaderStage)];
        }
    }
}
