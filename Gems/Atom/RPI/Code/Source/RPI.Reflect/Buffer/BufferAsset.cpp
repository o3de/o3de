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

#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(RPI::CommonBufferPoolType, "{E3FD19DF-4395-46FD-8092-D27BC73A3688}");

    namespace RPI
    {
        const char* BufferAsset::DisplayName = "BufferAsset";
        const char* BufferAsset::Group = "Buffer";
        const char* BufferAsset::Extension = "azbuffer";

        void BufferAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<BufferAsset>()
                    ->Version(2)
                    ->Field("Name", &BufferAsset::m_name)
                    ->Field("Buffer", &BufferAsset::m_buffer)
                    ->Field("BufferDescriptor", &BufferAsset::m_bufferDescriptor)
                    ->Field("BufferViewDescriptor", &BufferAsset::m_bufferViewDescriptor)
                    ->Field("BufferPoolAsset", &BufferAsset::m_poolAsset)
                    ->Field("CommonBufferPoolType", &BufferAsset::m_poolType)
                    ;

                // register enum strings
                serializeContext->Enum<CommonBufferPoolType>()
                    ->Value("Constant", CommonBufferPoolType::Constant)
                    ->Value("StaticInputAssembly", CommonBufferPoolType::StaticInputAssembly)
                    ->Value("DynamicInputAssembly", CommonBufferPoolType::DynamicInputAssembly)
                    ->Value("ReadBack", CommonBufferPoolType::ReadBack)
                    ->Value("ReadWrite", CommonBufferPoolType::ReadWrite)
                    ->Value("ReadOnly", CommonBufferPoolType::ReadOnly)
                    ->Value("Invalid", CommonBufferPoolType::Invalid)
                    ;
            }
        }

        AZStd::array_view<uint8_t> BufferAsset::GetBuffer() const
        {
            return AZStd::array_view<uint8_t>(m_buffer);
        }

        const RHI::BufferDescriptor& BufferAsset::GetBufferDescriptor() const
        {
            return m_bufferDescriptor;
        }

        const RHI::BufferViewDescriptor& BufferAsset::GetBufferViewDescriptor() const
        {
            return m_bufferViewDescriptor;
        }

        void BufferAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        const Data::Asset<ResourcePoolAsset>& BufferAsset::GetPoolAsset() const
        {
            return m_poolAsset;
        }

        CommonBufferPoolType BufferAsset::GetCommonPoolType() const
        {
            return m_poolType;
        }

        const AZStd::string& BufferAsset::GetName() const
        {
            return m_name;
        }
    } //namespace RPI
} // namespace AZ
