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

#include <Atom/RHI/IndirectBufferSignature.h>

namespace AZ
{
    namespace RHI
    {
        ResultCode IndirectBufferSignature::Init(Device& device, const IndirectBufferSignatureDescriptor& descriptor)
        {
            ResultCode result = InitInternal(device, descriptor);
            if (result == ResultCode::Success)
            {
                Base::Init(device);
                m_descriptor = descriptor;
            }

            return result;
        }

        uint32_t IndirectBufferSignature::GetByteStride() const
        {
            AZ_Assert(IsInitialized(), "Signature is not initialized");
            return GetByteStrideInternal();
        }

        uint32_t IndirectBufferSignature::GetOffset(IndirectCommandIndex index) const
        {
            AZ_Assert(IsInitialized(), "Signature is not initialized");
            if (Validation::IsEnabled())
            {
                if (index.IsNull())
                {
                    AZ_Assert(false, "Invalid index");
                    return 0;
                }

                if (index.GetIndex() >= m_descriptor.m_layout.GetCommands().size())
                {
                    AZ_Assert(false, "Index %d is greater than the number of commands on the layout", index.GetIndex());
                    return 0;
                }
            }

            return GetOffsetInternal(index);
        }

        const IndirectBufferSignatureDescriptor& IndirectBufferSignature::GetDescriptor() const
        {
            return m_descriptor;
        }

        const AZ::RHI::IndirectBufferLayout& IndirectBufferSignature::GetLayout() const
        {
            return m_descriptor.m_layout;
        }

        void IndirectBufferSignature::Shutdown()
        {
            ShutdownInternal();
            DeviceObject::Shutdown();
        }
    }
}
