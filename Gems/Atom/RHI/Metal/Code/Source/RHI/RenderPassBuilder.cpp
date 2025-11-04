/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <Atom/RHI/SwapChainFrameAttachment.h>
#include <RHI/RenderPassBuilder.h>
#include <RHI/Scope.h>

namespace AZ::Metal
{
    RenderPassBuilder::~RenderPassBuilder()
    {
        Reset();
    }

    void RenderPassBuilder::Init()
    {
        Reset();
        m_renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    }

    void RenderPassBuilder::AddScopeAttachments(Scope* scope)
    {
        AZStd::unordered_map<RHI::AttachmentId, ResolveAttachmentData> attachmentsIndex;
        id<MTLTexture> depthAttachmentTexture = m_renderPassDescriptor.depthAttachment.texture;
        id<MTLTexture> stencilAttachmentTexture = m_renderPassDescriptor.stencilAttachment.texture;
        for (RHI::ImageScopeAttachment* scopeAttachment : scope->GetImageAttachments())
        {
            const ImageView* imageView = static_cast<const ImageView*>(scopeAttachment->GetImageView()->GetDeviceImageView(scope->GetDeviceIndex()).get());
            const RHI::ImageScopeAttachmentDescriptor& bindingDescriptor = scopeAttachment->GetDescriptor();
            id<MTLTexture> imageViewMtlTexture = imageView->GetMemoryView().GetGpuAddress<id<MTLTexture>>();
            const RHI::AttachmentId attachmentId = bindingDescriptor.m_attachmentId;
            
            const bool isClearAction        = bindingDescriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
            const bool isClearActionStencil = bindingDescriptor.m_loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
            
            const bool isLoadAction         = bindingDescriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Load;
            
            // Metal doesn't support RHI::AttachmentStoreAction::None so we treat it as RHI::AttachmentStoreAction::Store
            const bool isStoreAction = bindingDescriptor.m_loadStoreAction.m_storeAction == RHI::AttachmentStoreAction::Store ||
            bindingDescriptor.m_loadStoreAction.m_storeAction == RHI::AttachmentStoreAction::None;
            const bool isStoreActionStencil  = bindingDescriptor.m_loadStoreAction.m_storeActionStencil == RHI::AttachmentStoreAction::Store ||
            bindingDescriptor.m_loadStoreAction.m_storeActionStencil == RHI::AttachmentStoreAction::None;
            
            
            MTLLoadAction mtlLoadAction = MTLLoadActionDontCare;
            MTLStoreAction mtlStoreAction = MTLStoreActionDontCare;
            MTLLoadAction mtlLoadActionStencil = MTLLoadActionDontCare;
            MTLStoreAction mtlStoreActionStencil = MTLStoreActionDontCare;
            
            if(isClearAction)
            {
                mtlLoadAction = MTLLoadActionClear;
            }
            else if(isLoadAction)
            {
                mtlLoadAction = MTLLoadActionLoad;
            }
            
            if(isStoreAction)
            {
                mtlStoreAction = MTLStoreActionStore;
            }
            
            if(isClearActionStencil)
            {
                mtlLoadActionStencil = MTLLoadActionClear;
            }
            else if(isLoadAction)
            {
                mtlLoadActionStencil = MTLLoadActionLoad;
            }
            
            if(isStoreActionStencil)
            {
                mtlStoreActionStencil = MTLStoreActionStore;
            }
            
            switch (scopeAttachment->GetUsage())
            {
                case RHI::ScopeAttachmentUsage::Shader:
                {
                    break;
                }
                case RHI::ScopeAttachmentUsage::RenderTarget:
                {
                    uint32_t colorAttachmentIndex = m_currentColorAttachmentIndex;
                    auto findIter = m_colorAttachmentsIndex.find(attachmentId);
                    if (findIter == m_colorAttachmentsIndex.end())
                    {
                        id<MTLTexture> renderTargetTexture = imageViewMtlTexture;
                        m_renderPassDescriptor.colorAttachments[colorAttachmentIndex].texture = renderTargetTexture;
                        
                        MTLRenderPassColorAttachmentDescriptor* colorAttachment = m_renderPassDescriptor.colorAttachments[colorAttachmentIndex];
                        colorAttachment.loadAction = mtlLoadAction;
                        
                        RHI::ClearValue clearVal = bindingDescriptor.m_loadStoreAction.m_clearValue;
                        if (mtlLoadAction == MTLLoadActionClear)
                        {
                             if(clearVal.m_type == RHI::ClearValueType::Vector4Float)
                            {
                                colorAttachment.clearColor = MTLClearColorMake(clearVal.m_vector4Float[0], clearVal.m_vector4Float[1], clearVal.m_vector4Float[2], clearVal.m_vector4Float[3]);
                            }
                            else if(clearVal.m_type == RHI::ClearValueType::Vector4Uint)
                            {
                                colorAttachment.clearColor = MTLClearColorMake(clearVal.m_vector4Uint[0], clearVal.m_vector4Uint[1], clearVal.m_vector4Uint[2], clearVal.m_vector4Uint[3]);
                            }
                        }
 
                        //Cubemap/cubemaparray and 3d textures have restrictions placed on them by the
                        //drivers when creating a new texture view. Hence we cant get a view with subresource range
                        //of the original texture. As a result in order to write into specific slice or depth plane
                        //we specify it here. It also means that we cant write into these texturee types via a compute shader
                        const RHI::ImageViewDescriptor& imgViewDescriptor = imageView->GetDescriptor();
                        if(renderTargetTexture.textureType == MTLTextureTypeCube || renderTargetTexture.textureType == MTLTextureTypeCubeArray)
                        {
                            m_renderPassDescriptor.colorAttachments[colorAttachmentIndex].slice = imgViewDescriptor.m_arraySliceMin;
                        }
                        else if(renderTargetTexture.textureType == MTLTextureType3D)
                        {
                            m_renderPassDescriptor.colorAttachments[colorAttachmentIndex].depthPlane = imgViewDescriptor.m_depthSliceMin;
                        }
                        
                        ApplyMSAACustomPositions(imageView);
                        m_colorAttachmentsIndex[attachmentId] = colorAttachmentIndex;
                        m_currentColorAttachmentIndex++;
                    }
                    else
                    {
                        colorAttachmentIndex = findIter->second;
                    }
                    m_renderPassDescriptor.colorAttachments[colorAttachmentIndex].storeAction = mtlStoreAction;
                    attachmentsIndex[attachmentId] = ResolveAttachmentData{colorAttachmentIndex, m_renderPassDescriptor, RHI::ScopeAttachmentUsage::RenderTarget, isStoreAction};
                    if(auto* swapChainFrameAttachment = (azrtti_cast<const RHI::SwapChainFrameAttachment*>(&scopeAttachment->GetFrameAttachment())))
                    {
                        m_swapChainFrameAttachment = swapChainFrameAttachment;
                        m_swapChainAttachmentIndex = colorAttachmentIndex;
                    }
                    break;
                }
                case RHI::ScopeAttachmentUsage::DepthStencil:
                {
                    // We can have multiple DepthStencil attachments in order to specify depth and stencil access separately.
                    // One attachment is depth only, and the other is stencil only.
                    const RHI::ImageViewDescriptor& viewDescriptor = imageView->GetDescriptor();
                    if(RHI::CheckBitsAll(viewDescriptor.m_aspectFlags, RHI::ImageAspectFlags::Depth) ||
                       m_renderPassDescriptor.depthAttachment.texture == nil)
                    {
                        MTLRenderPassDepthAttachmentDescriptor* depthAttachment = m_renderPassDescriptor.depthAttachment;
                        // First usage so we need to set all the MTLRenderPassDepthAttachmentDescriptor info
                        if (depthAttachmentTexture == nil)
                        {
                            depthAttachment.texture = imageViewMtlTexture;
                            depthAttachment.loadAction = mtlLoadAction;
                            if (bindingDescriptor.m_loadStoreAction.m_clearValue.m_type == RHI::ClearValueType::DepthStencil)
                            {
                                depthAttachment.clearDepth = bindingDescriptor.m_loadStoreAction.m_clearValue.m_depthStencil.m_depth;
                            }
                        }
                        depthAttachment.storeAction = mtlStoreAction;
                    }
                    
                    // Set the stencil only if the format support it and we either have a null stencil or the attachment is stencil only.
                    if(IsDepthStencilMerged(imageView->GetSpecificFormat()) &&
                       (RHI::CheckBitsAll(viewDescriptor.m_aspectFlags, RHI::ImageAspectFlags::Stencil) ||
                        m_renderPassDescriptor.stencilAttachment.texture == nil))
                    {
                        MTLRenderPassStencilAttachmentDescriptor* stencilAttachment = m_renderPassDescriptor.stencilAttachment;
                        // First usage so we need to set all the MTLRenderPassStencilAttachmentDescriptor info
                        if (stencilAttachmentTexture == nil)
                        {
                            stencilAttachment.texture = imageViewMtlTexture;
                            stencilAttachment.loadAction = mtlLoadActionStencil;
                            if (bindingDescriptor.m_loadStoreAction.m_clearValue.m_type == RHI::ClearValueType::DepthStencil)
                            {
                                stencilAttachment.clearStencil = bindingDescriptor.m_loadStoreAction.m_clearValue.m_depthStencil.m_stencil;
                            }
                        }
                        stencilAttachment.storeAction = mtlStoreActionStencil;
                    }
                    
                    if (depthAttachmentTexture == nil || stencilAttachmentTexture == nil)
                    {
                        ApplyMSAACustomPositions(imageView);
                    }
                    attachmentsIndex[attachmentId] = ResolveAttachmentData{-1, m_renderPassDescriptor, RHI::ScopeAttachmentUsage::DepthStencil, isStoreAction};
                    break;
                }
                case RHI::ScopeAttachmentUsage::Resolve:
                {
                    auto resolveScopeAttachment = static_cast<const RHI::ResolveScopeAttachment*>(scopeAttachment);
                    auto resolveAttachmentId = resolveScopeAttachment->GetDescriptor().m_resolveAttachmentId;
                    AZ_Assert(attachmentsIndex.find(resolveAttachmentId) != attachmentsIndex.end(), "Failed to find resolvable attachment %s", resolveAttachmentId.GetCStr());
                    ResolveAttachmentData resolveAttachmentData = attachmentsIndex[resolveAttachmentId];
                    MTLRenderPassDescriptor* renderPassDesc = resolveAttachmentData.m_renderPassDesc;
                    
                    id<MTLTexture> renderTargetTexture = imageViewMtlTexture;
                    
                    MTLStoreAction resolveStoreAction = resolveAttachmentData.m_isStoreAction ? MTLStoreActionStoreAndMultisampleResolve : MTLStoreActionMultisampleResolve;
                    
                    if(resolveAttachmentData.m_attachmentUsage == RHI::ScopeAttachmentUsage::RenderTarget)
                    {
                        MTLRenderPassColorAttachmentDescriptor* colorAttachment = renderPassDesc.colorAttachments[resolveAttachmentData.m_colorAttachmentIndex];
                        colorAttachment.resolveTexture = renderTargetTexture;
                        colorAttachment.storeAction = resolveStoreAction;
                    }
                    else if (resolveAttachmentData.m_attachmentUsage == RHI::ScopeAttachmentUsage::DepthStencil)
                    {
                        MTLRenderPassDepthAttachmentDescriptor* depthAttachment = renderPassDesc.depthAttachment;
                        depthAttachment.resolveTexture = renderTargetTexture;
                        depthAttachment.storeAction = resolveStoreAction;
                        //Metal drivers support min/max depth resolve filters but there is no way to set them at a higher level yet.
                    }
                    break;
                }
                case RHI::ScopeAttachmentUsage::SubpassInput:
                {
                    auto findIter = m_colorAttachmentsIndex.find(attachmentId);
                    AZ_Assert(findIter != m_colorAttachmentsIndex.end(), "Failed to find input attachment %s", attachmentId.GetCStr());
                    m_renderPassDescriptor.colorAttachments[findIter->second].storeAction = mtlStoreAction;
                    break;
                }
            }
        }
        m_scopes.push_back(scope);
    }

    RHI::ResultCode RenderPassBuilder::End(RenderPassContext& context)
    {
        if (m_scopes.empty() ||
            (m_colorAttachmentsIndex.empty() && (m_renderPassDescriptor.depthAttachment.texture == nil && m_renderPassDescriptor.stencilAttachment.texture == nil)))
        {
            return RHI::ResultCode::InvalidArgument;
        }
        
        context.m_renderPassDescriptor = m_renderPassDescriptor;
        context.m_scopeMultisampleState = m_scopeMultisampleState;
        context.m_swapChainAttachment = m_swapChainFrameAttachment;
        context.m_swapChainAttachmentIndex = m_swapChainAttachmentIndex;
        m_renderPassDescriptor = nil;
        return RHI::ResultCode::Success;
    }

    void RenderPassBuilder::Reset()
    {
        if (m_renderPassDescriptor)
        {
            m_renderPassDescriptor = nil;
        }
        
        m_scopes.clear();
        m_colorAttachmentsIndex.clear();
        m_currentColorAttachmentIndex = 0;
    }

    void RenderPassBuilder::ApplyMSAACustomPositions(const ImageView* imageView)
    {
        const Image& image = imageView->GetImage();
        const RHI::ImageDescriptor& imageDescriptor = image.GetDescriptor();
        
        m_scopeMultisampleState = imageDescriptor.m_multisampleState;
        if(m_scopeMultisampleState.m_customPositionsCount > 0)
        {
            NSUInteger numSampleLocations = m_scopeMultisampleState.m_samples;
            AZStd::vector<MTLSamplePosition> mtlCustomSampleLocations;
            AZStd::transform( m_scopeMultisampleState.m_customPositions.begin(),
                             m_scopeMultisampleState.m_customPositions.begin() + numSampleLocations,
                             AZStd::back_inserter(mtlCustomSampleLocations), [&](const auto& item)
            {
                return ConvertSampleLocation(item);
            });
            [m_renderPassDescriptor setSamplePositions:mtlCustomSampleLocations.data() count:numSampleLocations];
        }
    }
}
