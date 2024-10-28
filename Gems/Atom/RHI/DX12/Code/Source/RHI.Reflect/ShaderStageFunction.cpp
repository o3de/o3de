/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        AZ_CLASS_ALLOCATOR_IMPL(ShaderStageFunction, RHI::ShaderStageFunctionAllocator)

        static bool ConvertOldVersions(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 3)
            {
                // We need to convert the shader byte code because we added a custom allocator
                // and the serialization system doens't automatically convert between two different classes
                auto crc32 = AZ::Crc32("m_byteCodes");
                auto* arrayElement = classElement.FindSubElement(crc32);
                if (arrayElement)
                {
                    // When we convert all sub elements are removed, so we first need to
                    // get the child data, so we can re-insert it after we convert the element
                    AZStd::array<AZStd::vector<uint8_t>, ShaderSubStageCountMax> oldData;
                    if (classElement.GetChildData(crc32, oldData))
                    {
                        // Convert the array with the new allocator
                        arrayElement->Convert(context, AZ::AzTypeInfo<AZStd::array<ShaderByteCode, ShaderSubStageCountMax>>::Uuid());
                        for (const auto& element : oldData)
                        {
                            // Convert each vector and re add it. During convertion all sub elements were removed.
                            ShaderByteCode newData(element.size());
                            ::memcpy(newData.data(), element.data(), element.size());
                            arrayElement->AddElementWithData<ShaderByteCode>(context, "element", newData);
                        }
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
                serializeContext->RegisterGenericType<AZStd::array<AZStd::vector<uint8_t>, ShaderSubStageCountMax>>();

                serializeContext->Class<ShaderStageFunction, RHI::ShaderStageFunction>()
                    ->Version(3, &ConvertOldVersions)
                    ->Field("m_byteCodes", &ShaderStageFunction::m_byteCodes)
                    ->Field("m_specializationOffsets", &ShaderStageFunction::m_specializationOffsets);
            }
        }

        ShaderStageFunction::ShaderStageFunction(RHI::ShaderStage shaderStage)
            : RHI::ShaderStageFunction(shaderStage)
        {}


        RHI::Ptr<ShaderStageFunction> ShaderStageFunction::Create(RHI::ShaderStage shaderStage)
        {
            return aznew ShaderStageFunction(shaderStage);
        }

        void ShaderStageFunction::SetByteCode(uint32_t subStageIndex, const AZStd::vector<uint8_t>& byteCode)
        {
            m_byteCodes[subStageIndex].resize(byteCode.size());
            ::memcpy(m_byteCodes[subStageIndex].data(), byteCode.data(), byteCode.size());
        }

        ShaderByteCodeView ShaderStageFunction::GetByteCode(uint32_t subStageIndex) const
        {
            return ShaderByteCodeView(m_byteCodes[subStageIndex]);
        }

        void ShaderStageFunction::SetSpecializationOffsets(uint32_t subStageIndex, const SpecializationOffsets& offsets)
        {
            m_specializationOffsets[subStageIndex] = offsets;
        }

        const ShaderStageFunction::SpecializationOffsets& ShaderStageFunction::GetSpecializationOffsets(uint32_t subStageIndex) const
        {
            return m_specializationOffsets[subStageIndex];
        }

        bool ShaderStageFunction::UseSpecializationConstants(uint32_t subStageIndex) const
        {
            return !GetSpecializationOffsets(subStageIndex).empty();
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
