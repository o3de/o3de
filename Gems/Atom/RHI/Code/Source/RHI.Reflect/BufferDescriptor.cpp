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

#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RHI
    {
        BufferBindFlags GetBufferBindFlags(ScopeAttachmentUsage usage, ScopeAttachmentAccess access)
        {
            switch (usage)
            {
            case ScopeAttachmentUsage::RenderTarget:
                break;
            case ScopeAttachmentUsage::DepthStencil:
                break;
            case ScopeAttachmentUsage::Shader:
                switch (access)
                {
                case ScopeAttachmentAccess::ReadWrite:
                    return BufferBindFlags::ShaderReadWrite;
                case ScopeAttachmentAccess::Read:
                    return BufferBindFlags::ShaderRead;
                case ScopeAttachmentAccess::Write:
                    return BufferBindFlags::ShaderWrite;
                }
                break;
            case ScopeAttachmentUsage::Copy:
                switch (access)
                {
                case ScopeAttachmentAccess::Read:
                    return BufferBindFlags::CopyRead;
                case ScopeAttachmentAccess::Write:
                    return BufferBindFlags::CopyWrite;
                }
                break;
            case ScopeAttachmentUsage::Resolve:
                break;
            case ScopeAttachmentUsage::Predication:
                return BufferBindFlags::Predication;
            case ScopeAttachmentUsage::Indirect:
                return BufferBindFlags::Indirect;
            case ScopeAttachmentUsage::SubpassInput:
                break;
            case ScopeAttachmentUsage::InputAssembly:
                return BufferBindFlags::InputAssembly;
            case ScopeAttachmentUsage::Uninitialized:
                break;
            default:
                break;
            }
            return BufferBindFlags::None;
        }

        BufferDescriptor::BufferDescriptor(BufferBindFlags bindFlags, size_t byteCount)
            : m_bindFlags{bindFlags}
            , m_byteCount{byteCount}
        {}

        AZ::HashValue64 BufferDescriptor::GetHash(AZ::HashValue64 seed /* = 0 */) const
        {
            return TypeHash64(*this, seed);
        }

        void BufferDescriptor::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<BufferDescriptor>()
                    ->Version(0)
                    ->Field("m_bindFlags", &BufferDescriptor::m_bindFlags)
                    ->Field("m_byteCount", &BufferDescriptor::m_byteCount)
                    ->Field("m_alignment", &BufferDescriptor::m_alignment)
                    ;
            }
        }
    }
}
