/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    void RenderAttachmentDescriptor::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<RenderAttachmentDescriptor>()
                ->Version(1) // Added ScopeAttachmentAccess and ScopeAttachmentStage.
                ->Field("AttachmentIndex", &RenderAttachmentDescriptor::m_attachmentIndex)
                ->Field("ResolveAttachmentIndex", &RenderAttachmentDescriptor::m_resolveAttachmentIndex)
                ->Field("AttachmentLoadStore", &RenderAttachmentDescriptor::m_loadStoreAction)
                ->Field("ScopeAttachmentAccess", &RenderAttachmentDescriptor::m_scopeAttachmentAccess)
                ->Field("ScopeAttachmentStage", &RenderAttachmentDescriptor::m_scopeAttachmentStage)
                ;
        }
    }

    bool RenderAttachmentDescriptor::IsValid() const
    {
        return m_attachmentIndex != InvalidRenderAttachmentIndex;
    }

    bool RenderAttachmentDescriptor::operator==(const RenderAttachmentDescriptor& other) const
    {
        return (m_attachmentIndex == other.m_attachmentIndex)
            && (m_resolveAttachmentIndex == other.m_resolveAttachmentIndex)
            && (m_loadStoreAction == other.m_loadStoreAction)
            && (m_scopeAttachmentAccess == other.m_scopeAttachmentAccess)
            && (m_scopeAttachmentStage == other.m_scopeAttachmentStage)
            ;
    }

    bool RenderAttachmentDescriptor::operator!=(const RenderAttachmentDescriptor& other) const
    {
        return !(*this == other);
    }

    void SubpassRenderAttachmentLayout::Reflect(ReflectContext* context)
    {
        SubpassInputDescriptor::Reflect(context);
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<SubpassRenderAttachmentLayout>()
                ->Version(0)
                ->Field("RenderTargetCount", &SubpassRenderAttachmentLayout::m_rendertargetCount)
                ->Field("SubpassInputCount", &SubpassRenderAttachmentLayout::m_subpassInputCount)
                ->Field("RenderTargetDescriptors", &SubpassRenderAttachmentLayout::m_rendertargetDescriptors)
                ->Field("SubpasInputAttachmentDescriptors", &SubpassRenderAttachmentLayout::m_subpassInputDescriptors)
                ->Field("DepthStencilDescriptor", &SubpassRenderAttachmentLayout::m_depthStencilDescriptor);
        }

        RenderAttachmentDescriptor::Reflect(context);
    }

    bool SubpassRenderAttachmentLayout::operator==(const SubpassRenderAttachmentLayout& other) const
    {
        if ((m_rendertargetCount != other.m_rendertargetCount)
            || (m_subpassInputCount != other.m_subpassInputCount)
            || (m_depthStencilDescriptor != other.m_depthStencilDescriptor))
        {
            return false;
        }

        for (uint32_t i = 0; i < m_rendertargetCount; ++i)
        {
            if (m_rendertargetDescriptors[i] != other.m_rendertargetDescriptors[i])
            {
                return false;
            }
        }

        for (uint32_t i = 0; i < m_subpassInputCount; ++i)
        {
            if (m_subpassInputDescriptors[i] != other.m_subpassInputDescriptors[i])
            {
                return false;
            }
        }

        return true;
    }

    bool SubpassRenderAttachmentLayout::operator!=(const SubpassRenderAttachmentLayout& other) const
    {
        return !(*this == other);
    }

    void RenderAttachmentLayout::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<RenderAttachmentLayout>()
                ->Version(0)
                ->Field("AttachmentCount", &RenderAttachmentLayout::m_attachmentCount)
                ->Field("SubpassCount", &RenderAttachmentLayout::m_subpassCount)
                ->Field("AttachmentFormats", &RenderAttachmentLayout::m_attachmentFormats)
                ->Field("SubpassLayouts", &RenderAttachmentLayout::m_subpassLayouts);
        }

        SubpassRenderAttachmentLayout::Reflect(context);
    }

    HashValue64 RenderAttachmentLayout::GetHash() const
    {
        return TypeHash64(*this);
    }

    bool RenderAttachmentLayout::operator==(const RenderAttachmentLayout& other) const
    {
        if ((m_attachmentCount != other.m_attachmentCount)
            || (m_subpassCount != other.m_subpassCount))
        {
            return false;
        }

        for (uint32_t i = 0; i < m_attachmentCount; ++i)
        {
            if (m_attachmentFormats[i] != other.m_attachmentFormats[i])
            {
                return false;
            }
        }

        for (uint32_t i = 0; i < m_subpassCount; ++i)
        {
            if(m_subpassLayouts[i] != other.m_subpassLayouts[i])
            {
                return false;
            }
        }

        return true;
    }

    void RenderAttachmentConfiguration::Reflect(ReflectContext* context)
    {
        RenderAttachmentLayout::Reflect(context);
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<RenderAttachmentConfiguration>()
                ->Version(0)
                ->Field("RenderAttachmentLayout", &RenderAttachmentConfiguration::m_renderAttachmentLayout)
                ->Field("SubpassIndex", &RenderAttachmentConfiguration::m_subpassIndex);
        }
    }

    HashValue64 RenderAttachmentConfiguration::GetHash() const
    {
        HashValue64 hash = m_renderAttachmentLayout.GetHash();
        hash = TypeHash64(m_subpassIndex, hash);
        return hash;
    }

    Format RenderAttachmentConfiguration::GetRenderTargetFormat(uint32_t index) const
    {
        const auto& subpassAttachmentLayout = m_renderAttachmentLayout.m_subpassLayouts[m_subpassIndex];
        return m_renderAttachmentLayout.m_attachmentFormats[subpassAttachmentLayout.m_rendertargetDescriptors[index].m_attachmentIndex];
    }

    Format RenderAttachmentConfiguration::GetSubpassInputFormat(uint32_t index) const
    {
        const auto& subpassAttachmentLayout = m_renderAttachmentLayout.m_subpassLayouts[m_subpassIndex];
        return m_renderAttachmentLayout.m_attachmentFormats[subpassAttachmentLayout.m_subpassInputDescriptors[index].m_attachmentIndex];
    }

    Format RenderAttachmentConfiguration::GetRenderTargetResolveFormat(uint32_t index) const
    {
        const auto& subpassAttachmentLayout = m_renderAttachmentLayout.m_subpassLayouts[m_subpassIndex];
        if (subpassAttachmentLayout.m_rendertargetDescriptors[index].m_resolveAttachmentIndex != InvalidRenderAttachmentIndex)
        {
            return m_renderAttachmentLayout.m_attachmentFormats[subpassAttachmentLayout.m_rendertargetDescriptors[index].m_resolveAttachmentIndex];
        }
        return Format::Unknown;
    }

    Format RenderAttachmentConfiguration::GetDepthStencilFormat() const
    {
        const auto& subpassAttachmentLayout = m_renderAttachmentLayout.m_subpassLayouts[m_subpassIndex];
        return subpassAttachmentLayout.m_depthStencilDescriptor.IsValid() ?
            m_renderAttachmentLayout.m_attachmentFormats[subpassAttachmentLayout.m_depthStencilDescriptor.m_attachmentIndex] :
            RHI::Format::Unknown;
    }

    uint32_t RenderAttachmentConfiguration::GetRenderTargetCount() const
    {
        return m_renderAttachmentLayout.m_subpassLayouts[m_subpassIndex].m_rendertargetCount;
    }

    uint32_t RenderAttachmentConfiguration::GetSubpassInputCount() const
    {
        return m_renderAttachmentLayout.m_subpassLayouts[m_subpassIndex].m_subpassInputCount;
    }

    bool RenderAttachmentConfiguration::DoesRenderTargetResolve(uint32_t index) const
    {
        return m_renderAttachmentLayout.m_subpassLayouts[m_subpassIndex].m_rendertargetDescriptors[index].m_resolveAttachmentIndex != InvalidRenderAttachmentIndex;
    }

    bool RenderAttachmentConfiguration::operator==(const RenderAttachmentConfiguration& other) const
    {
        return (m_renderAttachmentLayout == other.m_renderAttachmentLayout) && (m_subpassIndex == other.m_subpassIndex);
    }

    void SubpassInputDescriptor::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<SubpassInputDescriptor>()
                ->Version(1) // Added ScopeAttachmentAccess and ScopeAttachmentStage.
                ->Field("RenderAttachmentIndex", &SubpassInputDescriptor::m_attachmentIndex)
                ->Field("AspectFlags", &SubpassInputDescriptor::m_aspectFlags)
                ->Field("ScopeAttachmentAccess", &SubpassInputDescriptor::m_scopeAttachmentAccess)
                ->Field("ScopeAttachmentStage", &SubpassInputDescriptor::m_scopeAttachmentStage)
                ;
        }
    }

    bool SubpassInputDescriptor::operator==(const SubpassInputDescriptor& other) const
    {
        return (m_attachmentIndex == other.m_attachmentIndex)
            && (m_aspectFlags == other.m_aspectFlags)
            && (m_scopeAttachmentAccess == other.m_scopeAttachmentAccess)
            && (m_scopeAttachmentStage == other.m_scopeAttachmentStage)
            ;
    }

    bool SubpassInputDescriptor::operator!=(const SubpassInputDescriptor& other) const
    {
        return !(*this == other);
    }
}
