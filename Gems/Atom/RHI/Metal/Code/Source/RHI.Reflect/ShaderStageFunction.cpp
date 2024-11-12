/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Metal/ShaderStageFunction.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace Metal
    {
        AZ_CLASS_ALLOCATOR_IMPL(ShaderStageFunction, RHI::ShaderStageFunctionAllocator)

        static bool ConvertOldVersions(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 4)
            {
                auto crc32 = AZ::Crc32("m_byteCode");
                auto* vectorElement = classElement.FindSubElement(crc32);
                if (vectorElement)
                {
                    // Get the old data
                    AZStd::vector<uint8_t> oldData;
                    if (vectorElement->GetData(oldData))
                    {
                        // Convert the vector with the new allocator
                        vectorElement->Convert(context, AZ::AzTypeInfo<ShaderByteCode>::Uuid());
                        // Copy old data to new data
                        ShaderByteCode newData(oldData.size());
                        ::memcpy(newData.data(), oldData.data(), newData.size());
                        // Set the new data
                        vectorElement->SetData(context, newData);
                    }
                }
            }
            return true;
        }

        void ShaderStageFunction::Reflect(ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                // Need to register the old type with the Serializer so we can read it in order to convert it
                serializeContext->RegisterGenericType<AZStd::vector<uint8_t>>();

                serializeContext->Class<ShaderStageFunction, RHI::ShaderStageFunction>()
                    ->Version(4, &ConvertOldVersions)
                    ->Field("m_sourceCode", &ShaderStageFunction::m_sourceCode)
                    ->Field("m_byteCode", &ShaderStageFunction::m_byteCode)
                    ->Field("m_byteCodeLength", &ShaderStageFunction::m_byteCodeLength)
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
            m_sourceCode = AZStd::string(sourceCode.begin(), sourceCode.end());
        }
    
        void ShaderStageFunction::SetSourceCode(AZStd::string_view sourceCode)
        {
            m_sourceCode = sourceCode;
        }
        
        const AZStd::string& ShaderStageFunction::GetSourceCode() const
        {
            return m_sourceCode;
        }

        void ShaderStageFunction::SetByteCode(const AZStd::vector<uint8_t>& byteCode)
        {
            m_byteCode.resize(byteCode.size());
            ::memcpy(m_byteCode.data(), byteCode.data(), m_byteCode.size());
            m_byteCodeLength = aznumeric_cast<uint32_t>(byteCode.size());
        }

        void ShaderStageFunction::SetEntryFunctionName(AZStd::string_view entryFunctionName)
        {
            m_entryFunctionName = entryFunctionName;
        }
        
        const ShaderByteCode& ShaderStageFunction::GetByteCode() const
        {
            return m_byteCode;
        }

        const uint32_t ShaderStageFunction::GetByteCodeLength() const
        {
            return m_byteCodeLength;
        }

        const AZStd::string& ShaderStageFunction::GetEntryFunctionName() const
        {
            return m_entryFunctionName;
        }

        RHI::ResultCode ShaderStageFunction::FinalizeInternal()
        {
            if (m_byteCode.empty() && m_sourceCode.empty())
            {
                AZ_Error("ShaderStageFunction", false, "Finalizing shader stage function with empty bytecodes.");
                return RHI::ResultCode::InvalidArgument;
            }

            HashValue64 hash = HashValue64{ 0 };
            if (!m_byteCode.empty())
            {
                hash = TypeHash64(m_byteCode.data(), m_byteCode.size(), hash);
            }
            
            if(!m_sourceCode.empty())
            {
                hash = TypeHash64(reinterpret_cast<const uint8_t*>(m_sourceCode.data()), m_sourceCode.size(), hash);
            }
            SetHash(hash);

            return RHI::ResultCode::Success;
        }
    }
}
