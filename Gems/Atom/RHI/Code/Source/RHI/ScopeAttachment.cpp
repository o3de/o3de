/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ScopeAttachment.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <Atom/RHI/FrameAttachment.h>

namespace AZ
{
    namespace RHI
    {
        ScopeAttachment::ScopeAttachment(
            Scope& scope,
            FrameAttachment& attachment,
            ScopeAttachmentUsage usage,
            ScopeAttachmentAccess access)
            : m_scope{&scope}
            , m_attachment{&attachment}
        {
            m_usageAndAccess.push_back(ScopeAttachmentUsageAndAccess{usage, access});
            m_isSwapChainAttachment = azrtti_cast<const RHI::SwapChainFrameAttachment*>(m_attachment) != nullptr;
        }

        const Scope& ScopeAttachment::GetScope() const
        {
            return *m_scope;
        }

        Scope& ScopeAttachment::GetScope()
        {
            return *m_scope;
        }

        const FrameAttachment& ScopeAttachment::GetFrameAttachment() const
        {
            return *m_attachment;
        }

        FrameAttachment& ScopeAttachment::GetFrameAttachment()
        {
            return *m_attachment;
        }        
    
        bool ScopeAttachment::HasUsage(const ScopeAttachmentUsage usage) const
        {
            auto findIter = AZStd::find_if(m_usageAndAccess.begin(), m_usageAndAccess.end(), [&usage](const auto& usageAndAccess)
            {
                return usage == usageAndAccess.m_usage;
            });

            if (findIter == m_usageAndAccess.end())
            {
                return false;
            }
            return true;
        }
    
        bool ScopeAttachment::HasAccessAndUsage(const ScopeAttachmentUsage usage, const ScopeAttachmentAccess access) const
        {
            auto findIter = AZStd::find_if(m_usageAndAccess.begin(), m_usageAndAccess.end(), [&usage](const auto& usageAndAccess)
            {
                return usage == usageAndAccess.m_usage;
            });
            
            if (findIter == m_usageAndAccess.end())
            {
                return false;
            }
            
            return findIter->m_access == access;
        }

        const ScopeAttachment* ScopeAttachment::GetPrevious() const
        {
            return m_prev;
        }

        ScopeAttachment* ScopeAttachment::GetPrevious()
        {
            return m_prev;
        }

        const ScopeAttachment* ScopeAttachment::GetNext() const
        {
            return m_next;
        }

        ScopeAttachment* ScopeAttachment::GetNext()
        {
            return m_next;
        }
    
        AZStd::string ScopeAttachment::GetUsageTypes() const
        {
            //Used for logging
            AZStd::string usageTypesStr;
            for (const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess : m_usageAndAccess)
            {
                usageTypesStr += AZStd::string::format("%s ", GetTypeName(usageAndAccess));
            }
            return usageTypesStr;
        }
    
        AZStd::string ScopeAttachment::GetAccessTypes() const
        {
            //Used for logging
            AZStd::string usageTypesStr;
            for (const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess : m_usageAndAccess)
            {
                usageTypesStr += AZStd::string::format("%s ", ToString(usageAndAccess.m_access));
            }
            return usageTypesStr;
        }
    
        const char* ScopeAttachment::GetTypeName(const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess) const
        {
            switch (usageAndAccess.m_usage)
            {
            case ScopeAttachmentUsage::RenderTarget:
                return "RenderTarget";

            case ScopeAttachmentUsage::DepthStencil:
                    return CheckBitsAny(usageAndAccess.m_access, ScopeAttachmentAccess::Write) ? "DepthStencilReadWrite" : "DepthStencilRead";

            case ScopeAttachmentUsage::SubpassInput:
                return "SubpassInput";

            case ScopeAttachmentUsage::Shader:
                return CheckBitsAny(usageAndAccess.m_access, ScopeAttachmentAccess::Write) ? "ShaderReadWrite" : "ShaderRead";

            case ScopeAttachmentUsage::Copy:
                return CheckBitsAny(usageAndAccess.m_access, ScopeAttachmentAccess::Write) ? "CopyDest" : "CopySource";

            case ScopeAttachmentUsage::Predication:
                return "Predication";

            case ScopeAttachmentUsage::InputAssembly:
                return "InputAssembly";

            case ScopeAttachmentUsage::Uninitialized:
                return "Uninitialized";
            }

            return "Unknown";
        }
    
        const ResourceView* ScopeAttachment::GetResourceView() const
        {
            return m_resourceView.get();
        }

        void ScopeAttachment::SetResourceView(ConstPtr<ResourceView> resourceView)
        {
            m_resourceView = AZStd::move(resourceView);
        }
    
        void ScopeAttachment::AddUsageAndAccess(ScopeAttachmentUsage usage, ScopeAttachmentAccess access)
        {
#if defined (AZ_RHI_ENABLE_VALIDATION) 
            ValidateMultipleScopeAttachmentUsages(usage, access);
#endif
            m_usageAndAccess.push_back(ScopeAttachmentUsageAndAccess{usage, access});
        }
    
        const AZStd::vector<ScopeAttachmentUsageAndAccess>& ScopeAttachment::GetUsageAndAccess() const
        {
            return m_usageAndAccess;
        }
        
        void ScopeAttachment::ValidateMultipleScopeAttachmentUsages(const ScopeAttachmentUsage usage, const ScopeAttachmentAccess access)
        {
            for (const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess : m_usageAndAccess)
            {
                //Validation for access type
                switch (access)
                {
                case ScopeAttachmentAccess::Read:
                    AZ_Assert((usageAndAccess.m_access != ScopeAttachmentAccess::Write &&
                        usageAndAccess.m_access != ScopeAttachmentAccess::ReadWrite), "Read access state mixed with Write/ReadWrite for resource %s", GetFrameAttachment().GetId().GetCStr());
                    break;
                case ScopeAttachmentAccess::Write:
                case ScopeAttachmentAccess::ReadWrite:
                    AZ_Assert(usageAndAccess.m_access != ScopeAttachmentAccess::Read, "Read access state mixed with Write/ReadWrite for resource %s", GetFrameAttachment().GetId().GetCStr());
                    break;
                default:
                    AZ_Assert(false, "Access type not supported");
                }
                
                //Validation for usage type
                switch (usage)
                {
                case ScopeAttachmentUsage::RenderTarget:
                {
                    switch (usageAndAccess.m_usage)
                    {
                    case ScopeAttachmentUsage::RenderTarget:
                        AZ_Warning("FrameGraph", usage != usageAndAccess.m_usage, "Multiple usages of same type RenderTarget getting added for resource %s", GetFrameAttachment().GetId().GetCStr());
                        break;
                    default:
                        AZ_Assert(false, "ScopeAttachmentUsage::RenderTarget usage mixed with ScopeAttachmentUsage::%s for resource %s", GetTypeName(usageAndAccess), GetFrameAttachment().GetId().GetCStr());
                        break;
                    }
                    break;
                }
                case ScopeAttachmentUsage::DepthStencil:
                {
                    switch (usageAndAccess.m_usage)
                    {
                    case ScopeAttachmentUsage::DepthStencil:
                        AZ_Warning("FrameGraph", usage != usageAndAccess.m_usage, "Multiple usages of same type DepthStencil getting added for resource %s", GetFrameAttachment().GetId().GetCStr());
                        break;
                    case ScopeAttachmentUsage::RenderTarget:
                    case ScopeAttachmentUsage::Predication:
                    case ScopeAttachmentUsage::Resolve:
                    case ScopeAttachmentUsage::InputAssembly:
                        AZ_Assert(false, "ScopeAttachmentUsage::DepthStencil usage mixed with ScopeAttachmentUsage::%s for resource %s", GetTypeName(usageAndAccess), GetFrameAttachment().GetId().GetCStr());
                        break;
                    }
                    break;
                }
                case ScopeAttachmentUsage::Shader:
                {
                    switch (usageAndAccess.m_usage)
                    {
                    case ScopeAttachmentUsage::Resolve:
                    case ScopeAttachmentUsage::Predication:
                    case ScopeAttachmentUsage::InputAssembly:
                        AZ_Assert(false, "ScopeAttachmentUsage::Shader usage mixed with ScopeAttachmentUsage::%s for resource %s", GetTypeName(usageAndAccess), GetFrameAttachment().GetId().GetCStr());
                        break;
                    }
                    break;
                }
                case ScopeAttachmentUsage::Resolve:
                {
                    switch (usageAndAccess.m_usage)
                    {
                    case ScopeAttachmentUsage::Resolve:
                        AZ_Warning("FrameGraph", usage != usageAndAccess.m_usage, "Multiple usages of same type DepthStencil getting added for resource %s", GetFrameAttachment().GetId().GetCStr());
                        break;
                    case ScopeAttachmentUsage::RenderTarget:
                    case ScopeAttachmentUsage::DepthStencil:
                    case ScopeAttachmentUsage::Shader:
                    case ScopeAttachmentUsage::Predication:
                    case ScopeAttachmentUsage::SubpassInput:
                    case ScopeAttachmentUsage::InputAssembly:
                        AZ_Assert(false, "ScopeAttachmentUsage::Resolve usage mixed with ScopeAttachmentUsage::%s for resource %s", GetTypeName(usageAndAccess), GetFrameAttachment().GetId().GetCStr());
                        break;
                    }
                    break;
                }
                case ScopeAttachmentUsage::Predication:
                {
                    switch (usageAndAccess.m_usage)
                    {
                    case ScopeAttachmentUsage::Predication:
                        AZ_Warning("FrameGraph", usage != usageAndAccess.m_usage, "Multiple usages of same type DepthStencil getting added for resource %s", GetFrameAttachment().GetId().GetCStr());
                        break;
                    case ScopeAttachmentUsage::RenderTarget:
                    case ScopeAttachmentUsage::DepthStencil:
                    case ScopeAttachmentUsage::Shader:
                    case ScopeAttachmentUsage::Resolve:
                    case ScopeAttachmentUsage::SubpassInput:
                    case ScopeAttachmentUsage::InputAssembly:
                        AZ_Assert(false, "ScopeAttachmentUsage::Predication usage mixed with ScopeAttachmentUsage::%s for resource %s", GetTypeName(usageAndAccess), GetFrameAttachment().GetId().GetCStr());
                        break;
                    }
                    break;
                }
                case ScopeAttachmentUsage::Indirect:
                {
                    break;
                }
                case ScopeAttachmentUsage::SubpassInput:
                {
                    switch (usageAndAccess.m_usage)
                    {
                    case ScopeAttachmentUsage::Resolve:
                    case ScopeAttachmentUsage::Predication:
                    case ScopeAttachmentUsage::InputAssembly:
                        AZ_Assert(false, "ScopeAttachmentUsage::SubpassInput usage mixed with ScopeAttachmentUsage::%s for resource %s", GetTypeName(usageAndAccess), GetFrameAttachment().GetId().GetCStr());
                        break;
                    }
                    break;
                }
                case ScopeAttachmentUsage::InputAssembly:
                {
                    AZ_Assert(false, "ScopeAttachmentUsage::InputAssembly usage mixed with ScopeAttachmentUsage::%s for resource %s", GetTypeName(usageAndAccess), GetFrameAttachment().GetId().GetCStr());
                    break;
                }
                }
            }
        }
        
        bool ScopeAttachment::IsSwapChainAttachment() const
        {
            return m_isSwapChainAttachment;
        }
    }
}
