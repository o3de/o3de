/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/CommandListValidator.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/DeviceShaderResourceGroup.h>
#include <Atom/RHI/DeviceShaderResourceGroupPool.h>
#include <Atom/RHI/DeviceResourcePool.h>
#include <Atom/RHI/DeviceImagePoolBase.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/DeviceResourceView.h>
#include <Atom/RHI/DeviceResource.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/ScopeAttachment.h>
#include <Atom/RHI/FrameAttachment.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>

namespace AZ::RHI
{
    void CommandListValidator::BeginScope(const Scope& scope)
    {
        if (!Validation::IsEnabled())
        {
            return;
        }
        AZ_PROFILE_FUNCTION(RHI);
        AZ_Assert(m_scope == nullptr, "BeginScope called twice.");
        m_scope = &scope;

        for (const ScopeAttachment* scopeAttachment : scope.GetAttachments())
        {
            auto resource = scopeAttachment->GetResourceView()->GetResource();
            m_attachments[resource].push_back(scopeAttachment);
        }
    }

    void CommandListValidator::EndScope()
    {
        if (!Validation::IsEnabled())
        {
            return;
        }
        m_scope = nullptr;
        m_attachments.clear();
    }

    bool CommandListValidator::ValidateShaderResourceGroup(const DeviceShaderResourceGroup& shaderResourceGroup, const ShaderResourceGroupBindingInfo& bindingInfo) const
    {
        if (!Validation::IsEnabled())
        {
            return true;
        }
        ValidateViewContext context;
        context.m_scopeName = m_scope->GetId().GetCStr();
        context.m_srgName = shaderResourceGroup.GetName().GetCStr();

        if (shaderResourceGroup.IsQueuedForCompile())
        {
            AZ_Warning("CommandListValidator", false,
                "[Scope '%s']: SRG '%s' is queued for compilation. This means the parent pool '%s' was not "
                "imported into the frame scheduler. This will result in SRG contents not being uploaded to "
                "the GPU.",
                context.m_scopeName,
                context.m_srgName,
                shaderResourceGroup.GetPool()->GetName().GetCStr());

            return false;
        }

        bool isSuccess = true;

        const DeviceShaderResourceGroupData& groupData = shaderResourceGroup.GetData();
        const ShaderResourceGroupLayout& groupLayout = *groupData.GetLayout();

        // Validate buffers

        const auto& bufferInputs = groupLayout.GetShaderInputListForBuffers();
        for (uint32_t shaderInputIndex = 0; shaderInputIndex < bufferInputs.size(); ++shaderInputIndex)
        {
            const RHI::ShaderInputBufferDescriptor& shaderInputBuffer = bufferInputs[shaderInputIndex];
            // First check if the buffer is being used.
            auto findIt = bindingInfo.m_resourcesRegisterMap.find(shaderInputBuffer.m_name);
            if (findIt == bindingInfo.m_resourcesRegisterMap.end() || findIt->second.m_shaderStageMask == RHI::ShaderStageMask::None)
            {
                continue;
            }

            context.m_shaderInputTypeName = GetShaderInputAccessName(shaderInputBuffer.m_access);
            context.m_scopeAttachmentAccess = GetAttachmentAccess(shaderInputBuffer.m_access);

            const RHI::ShaderInputBufferIndex bufferInputIndex(shaderInputIndex);
            auto bufferViews = groupData.GetBufferViewArray(bufferInputIndex);
            for (auto& bufferView : bufferViews)
            {
                if (bufferView)
                {
                    context.m_resourceView = bufferView.get();
                    isSuccess &= ValidateView(context, bufferView->IgnoreFrameAttachmentValidation());
                }
            }
        }

        // Validate images

        const auto& imageInputs = groupLayout.GetShaderInputListForImages();
        for (uint32_t shaderInputIndex = 0; shaderInputIndex < imageInputs.size(); ++shaderInputIndex)
        {
            const RHI::ShaderInputImageDescriptor& shaderInputImage = imageInputs[shaderInputIndex];

            // First check if the image is being used.
            auto findIt = bindingInfo.m_resourcesRegisterMap.find(shaderInputImage.m_name);
            if (findIt == bindingInfo.m_resourcesRegisterMap.end() || findIt->second.m_shaderStageMask == RHI::ShaderStageMask::None)
            {
                continue;
            }

            context.m_shaderInputTypeName = GetShaderInputAccessName(shaderInputImage.m_access);
            context.m_scopeAttachmentAccess = GetAttachmentAccess(shaderInputImage.m_access);

            const RHI::ShaderInputImageIndex imageInputIndex(shaderInputIndex);
            auto imageViews = groupData.GetImageViewArray(imageInputIndex);
            for (auto& imageView : imageViews)
            {
                if (imageView)
                {
                    context.m_resourceView = imageView.get();
                    isSuccess &= ValidateView(context, false);
                }
            }
            ++shaderInputIndex;
        }

        return isSuccess;
    }

    bool CommandListValidator::ValidateAttachment(
        const ValidateViewContext& context,
        const FrameAttachment* frameAttachment) const
    {
        AZ_Assert(frameAttachment, "Frame attachment is null.");
        [[maybe_unused]] const char* attachmentName = frameAttachment->GetId().GetCStr();

        const AZStd::vector<const ScopeAttachment*>* scopeAttachments = nullptr;
        auto findIt = m_attachments.find(frameAttachment->GetResource());
        if (findIt != m_attachments.end())
        {
            scopeAttachments = &findIt->second;
        }

        if (scopeAttachments && scopeAttachments->size() > 0)
        {
            bool isValidUsage = false;
            bool isValidAccess = false;

            for (const ScopeAttachment* scopeAttachment : *scopeAttachments)
            {
                isValidUsage = scopeAttachment->GetUsage() == ScopeAttachmentUsage::Shader || scopeAttachment->GetUsage() == ScopeAttachmentUsage::SubpassInput;
                isValidAccess = scopeAttachment->GetAccess() == context.m_scopeAttachmentAccess;

                if (isValidUsage && isValidAccess)
                {
                    return true;
                }
            }

            // We couldn't find a scope attachment that matches the usage, output warning
            AZ_Warning("CommandListValidator", false,
                "[Scope '%s', SRG '%s']: Failed to find a matching usage for attachment '%s'. Mismatches are as follows:",
                context.m_scopeName,
                context.m_srgName,
                attachmentName);

            // Output mismatch for each of the scope attachments in the list
            for (const ScopeAttachment* scopeAttachment : *scopeAttachments)
            {
                isValidUsage = scopeAttachment->GetUsage() == ScopeAttachmentUsage::Shader || scopeAttachment->GetUsage() == ScopeAttachmentUsage::SubpassInput;
                isValidAccess = scopeAttachment->GetAccess() == context.m_scopeAttachmentAccess;

                AZ_Warning("CommandListValidator", isValidUsage,
                    "[Scope '%s', SRG '%s']: Attachment '%s' is used as ['%s'], but usage needs to be 'Shader'",
                    context.m_scopeName,
                    context.m_srgName,
                    attachmentName,
                    scopeAttachment->GetTypeName());

                AZ_Warning("CommandListValidator", isValidAccess,
                    "[Scope '%s', SRG '%s']: Attachment '%s' is marked for '%s' access, but the scope declared ['%s'] access.",
                    context.m_scopeName,
                    context.m_srgName,
                    attachmentName,
                    ToString(context.m_scopeAttachmentAccess),
                    scopeAttachment->GetTypeName());
            }

            return false;
        }

        AZ_Warning(
            "CommandListValidator", false,
            "[Scope '%s', SRG '%s']: Attachment '%s' not declared for usage in this scope. Actual usage: '%s'",
            context.m_scopeName,
            context.m_srgName,
            attachmentName,
            context.m_shaderInputTypeName);

        return false;
    }

    ScopeAttachmentAccess CommandListValidator::GetAttachmentAccess(ShaderInputBufferAccess access)
    {
        return
            (access == ShaderInputBufferAccess::ReadWrite)
            ? ScopeAttachmentAccess::ReadWrite
            : ScopeAttachmentAccess::Read;
    }

    ScopeAttachmentAccess CommandListValidator::GetAttachmentAccess(ShaderInputImageAccess access)
    {
        return
            (access == ShaderInputImageAccess::ReadWrite)
            ? ScopeAttachmentAccess::ReadWrite
            : ScopeAttachmentAccess::Read;
    }

    bool CommandListValidator::ValidateView(const ValidateViewContext& context, bool ignoreAttachmentValidation) const
    {
        const DeviceResourceView& resourceView = *context.m_resourceView;
        const DeviceResource& resource = resourceView.GetResource();
        [[maybe_unused]] const char* resourceViewName = resourceView.GetName().GetCStr();
        [[maybe_unused]] const char* resourceName = resource.GetName().GetCStr();

        if (resourceView.IsStale())
        {
            AZ_Warning(
                "CommandListValidator", false,
                "[Scope '%s', SRG '%s']: DeviceResourceView '%s' of DeviceResource '%s' is stale! This indicates that the SRG was not properly "
                "compiled, or was invalidated after compilation during the command list recording phase.",
                context.m_scopeName,
                context.m_srgName,
                resourceViewName,
                resourceName);

            return false;
        }

        if (resource.IsAttachment())
        {
            return ValidateAttachment(context, resource.GetFrameAttachment());
        }

        // DeviceResource is not an attachment. It must be in a read-only state.
        if (!ignoreAttachmentValidation && context.m_scopeAttachmentAccess != ScopeAttachmentAccess::Read)
        {
            AZ_Warning(
                "CommandListValidator", false,
                "[Scope '%s', SRG '%s']: DeviceResourceView '%s' of DeviceResource '%s' is declared as '%s', but this type "
                "requires that the resource be an attachment.",
                context.m_scopeName,
                context.m_srgName,
                resourceViewName,
                resourceName,
                context.m_shaderInputTypeName);

            return false;
        }

        return true;
    }
}
