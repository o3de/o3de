/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/BufferPoolBase.h>
#include <Atom/RHI/FrameGraph.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/SwapChain.h>

#include <Atom/RHI.Reflect/BufferScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ResolveScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/ScopeId.h>

#include <AzCore/std/containers/span.h>

namespace AZ
{
    namespace RHI
    {
        class ResourcePool;
        class QueryPool;
        class Fence;
        struct Interval;

        //! This interface exposes FrameGraph functionality to non-RHI systems (like the RPI).
        //! This is in order to reduce access to certain public functions in FrameGraph that are intended for RHI use only.
        //! FrameGraph is a class that builds and orders scopes (associated with a specific scope id) for the current frame.
        //! This interface is broken down into two parts: global attachment registration and local scope configuration.
        //!
        //! Attachments are resources registered with the frame scheduler. Persistent resources are
        //! "imported" into the frame scheduler directly. Transient resources are created and internally
        //! managed by the frame scheduler. Their lifetime is only valid for the scopes that use them.
        //!
        //! Global attachment registration is done through this API via the create / import methods. Those
        //! operations are considered immediate and global. This means any scopes built later can reference
        //! the attachment id. This is useful if a downstream scope just wants to use an attachment without
        //! caring where it came from.
        //!
        //! Local scope configuration is done via the Use* and other remaining methods. A scope must declare
        //! usage of an attachment via Use*. This is true even if the scope created or imported the attachment.
        class FrameGraphInterface
        {
        public:
            FrameGraphInterface(FrameGraph& frameGraph)
                : m_frameGraph(frameGraph)
            { }
            ~FrameGraphInterface() = default;

            //! Acquires the attachment builder interface for declaring new attachments.
            FrameGraphAttachmentInterface GetAttachmentDatabase()
            {
                return m_frameGraph.GetAttachmentDatabase();
            }

            //! Declares a buffer attachment for use on the current scope.
            ResultCode UseAttachment(
                const BufferScopeAttachmentDescriptor& descriptor,
                ScopeAttachmentAccess access,
                ScopeAttachmentUsage usage)
            {
                return m_frameGraph.UseAttachment(descriptor, access, usage);
            }

            //! Declares an image attachment for use on the current scope.
            ResultCode UseAttachment(
                const ImageScopeAttachmentDescriptor& descriptor,
                ScopeAttachmentAccess access,
                ScopeAttachmentUsage usage)
            {
                return m_frameGraph.UseAttachment(descriptor, access, usage);
            }

            //! Declares an array of image attachments for use on the current scope.
            ResultCode UseAttachments(
                AZStd::span<const ImageScopeAttachmentDescriptor> descriptors,
                ScopeAttachmentAccess access,
                ScopeAttachmentUsage usage)
            {
                return m_frameGraph.UseAttachments(descriptors, access, usage);
            }
            
            //! Declares an array of color attachments for use on the current scope.
            ResultCode UseColorAttachments(AZStd::span<const ImageScopeAttachmentDescriptor> descriptors)
            {
                return m_frameGraph.UseColorAttachments(descriptors);
            }
            
            //! Declares a single color attachment for use on the current scope.
            ResultCode UseColorAttachment(const ImageScopeAttachmentDescriptor& descriptor)
            {
                return m_frameGraph.UseColorAttachment(descriptor);
            }
            
            //! Declares an array of subpass input attachments for use on the current scope.
            //! See UseSubpassInputAttachment for a definition about a SubpassInput.
            ResultCode UseSubpassInputAttachments(AZStd::span<const ImageScopeAttachmentDescriptor> descriptors)
            {
                return m_frameGraph.UseSubpassInputAttachments(descriptors);
            }
            
            //! Declares a single subpass input attachment for use on the current scope.
            //! Subpass input attachments are image views that can be used for pixel local load operations inside a fragment shader.
            //! This means that framebuffer attachments written in one subpass can be read from at the exact same pixel
            //! in subsequent subpasses. Certain platform have optimization for this type of attachments.
            ResultCode UseSubpassInputAttachment(const ImageScopeAttachmentDescriptor& descriptor)
            {
                return m_frameGraph.UseSubpassInputAttachment(descriptor);
            }
            
            //! Declares a single resolve attachment for use on the current scope.
            ResultCode UseResolveAttachment(const ResolveScopeAttachmentDescriptor& descriptor)
            {
                return m_frameGraph.UseResolveAttachment(descriptor);
            }
            
            //! Declares a depth-stencil attachment for use on the current scope.
            //! @param descriptor The depth stencil scope attachment.
            //! @param access How the attachment is accessed by the scope. Must be read-write if a clear action is specified.
            ResultCode UseDepthStencilAttachment(
                const ImageScopeAttachmentDescriptor& descriptor,
                ScopeAttachmentAccess access)
            {
                return m_frameGraph.UseDepthStencilAttachment(descriptor, access);
            }
            
            //! Declares a buffer shader attachment for use on the current scope.
            //! @param descriptor The buffer scope attachment.
            //! @param access How the attachment is accessed by the scope. Must be read-write if a clear action is specified.
            ResultCode UseShaderAttachment(
                const BufferScopeAttachmentDescriptor& descriptor,
                ScopeAttachmentAccess access)
            {
                return m_frameGraph.UseShaderAttachment(descriptor, access);
            }
            
            //! Declares an image shader attachment for use on the current scope.
            //! @param descriptor The image scope attachment.
            //! @param access How the attachment is accessed by the scope. Must be read-write if a clear action is specified.
            ResultCode UseShaderAttachment(
                const ImageScopeAttachmentDescriptor& descriptor,
                ScopeAttachmentAccess access)
            {
                return m_frameGraph.UseShaderAttachment(descriptor, access);
            }
            
            //! Declares a buffer copy attachment for use on the current scope.
            //! @param descriptor The buffer scope attachment.
            //! @param access How the attachment is accessed by the scope. Must be read-write if a clear action is specified.
            ResultCode UseCopyAttachment(
                const BufferScopeAttachmentDescriptor& descriptor,
                ScopeAttachmentAccess access)
            {
                return m_frameGraph.UseCopyAttachment(descriptor, access);
            }
            
            //! Declares an image copy attachment for use on the current scope.
            //! @param descriptor The image scope attachment.
            //! @param access How the attachment is accessed by the scope. Must be read-write if a clear action is specified.
            ResultCode UseCopyAttachment(
                const ImageScopeAttachmentDescriptor& descriptor,
                ScopeAttachmentAccess access)
            {
                return m_frameGraph.UseCopyAttachment(descriptor, access);
            }

            //! Declares a buffer input assembly attachment for use on the current scope.
            //! @param descriptor The buffer scope attachment.
            ResultCode UseInputAssemblyAttachment(
                const BufferScopeAttachmentDescriptor& descriptor)
            {
                return m_frameGraph.UseAttachment(descriptor, ScopeAttachmentAccess::Read, ScopeAttachmentUsage::InputAssembly);
            }
            
            //! Declares a query pool for use on the current scope.
            //! @param queryPool The query pool to be used.
            //! @param interval The range of queries from the pool that will be use by the current scope.
            //! @param type The type of query pool attachment.
            //! @param access How the attachment is accessed by the scope.
            ResultCode UseQueryPool(
                Ptr<QueryPool> queryPool,
                const RHI::Interval& interval,
                QueryPoolScopeAttachmentType type,
                ScopeAttachmentAccess access)
            {
                return m_frameGraph.UseQueryPool(queryPool, interval, type, access);
            }
            
            //! Declares that this scope depends on the given scope id, and must wait for it to complete.
            void ExecuteAfter(const ScopeId& producerScopeId)
            {
                m_frameGraph.ExecuteAfter(producerScopeId);
            }
            
            //! Declares that the given scope at @param consumerScopeId depends on this scope, forcing this
            //! scope to execute first.
            void ExecuteBefore(const ScopeId& consumerScopeId)
            {
                m_frameGraph.ExecuteBefore(consumerScopeId);
            }
            
            //! Requests that the provided fence be signaled after the scope has completed.
            void SignalFence(Fence& fence)
            {
                m_frameGraph.SignalFence(fence);
            }
            
            //! Sets the number of work items (Draw / Dispatch / etc) that will be processed by
            //! this scope. This value is used to load-balance the scope across command lists. A small
            //! value may result in the scope being merged onto a single command list, whereas a large
            //! one may result in the scope being split across several command lists in order to best
            //! parallelize submission
            void SetEstimatedItemCount(uint32_t itemCount)
            {
                m_frameGraph.SetEstimatedItemCount(itemCount);
            }
            
            //! Requests that a specific GPU hardware queue be used for processing this scope.
            void SetHardwareQueueClass(HardwareQueueClass hardwareQueueClass)
            {
                m_frameGraph.SetHardwareQueueClass(hardwareQueueClass);
            }

        private:

            //! Reference to the underlying FrameGraph. All function calls are forwarded to this member
            FrameGraph& m_frameGraph;
        };
    }
}
