/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>

namespace AZ
{
    namespace RHI
    {
        //! Describes how an attachment is accessed by a scope.
        enum class ScopeAttachmentAccess : uint32_t
        {
            Unknown = 0,
            
            //! The scope has read access to the attachment.
            Read      = AZ_BIT(0),

            //! The scope has write access to the attachment.
            Write     = AZ_BIT(1),

            //! The scope has read/write access to the attachment.
            ReadWrite = Read | Write
        };
        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::ScopeAttachmentAccess);

        const char* ToString(ScopeAttachmentAccess attachmentAccess);

        //! Describes the underlying resource lifetime of an attachment with regards to the
        //! frame graph. Imported attachments are owned by the user and are persistent across
        //! frames. Transient attachments are owned by the transient attachment pool and are
        //! considered valid only for the current frame.
        enum class AttachmentLifetimeType : uint32_t
        {
            //! Imported by the user through the FrameGraph.
            Imported = 0,

            //! Created by the user for the current frame using the transient attachment FrameGraph.
            Transient
        };

        //! Describes how a Scope uses an Attachment
        enum class ScopeAttachmentUsage : uint32_t
        {
            //! Error value to catch uninitialized usage of this enum
            Uninitialized = 0,

            //! Render targets use the fixed-function output merger stage on the graphics queue.
            RenderTarget,

            //! A depth stencil attachment uses the fixed-function depth-stencil output merger stage on the graphics queue.
            DepthStencil,

            //! A shader attachment is exposed directly to the shader with either read or read-write access.
            Shader,

            //! A copy attachment is available for copy access via CopyItem.
            Copy,

            //! A resolve attachment target
            Resolve,

            //! An attachment used for predication
            Predication,

            //! An attachment used for indirect draw/dispatch.
            Indirect,

            //! An attachment that allows reading the output of a previous subpass.
            SubpassInput,

            //! An attachment used as Input Assembly in the scope. Only needed for buffers that are modified by the GPU (e.g Skinned Meshes), not
            //! for static data.
            InputAssembly,

            Count,
        };

        enum class ScopeAttachmentUsageMask : uint32_t
        {
            None = 0,
            RenderTarget = AZ_BIT(static_cast<uint32_t>(ScopeAttachmentUsage::RenderTarget)),
            DepthStencil = AZ_BIT(static_cast<uint32_t>(ScopeAttachmentUsage::DepthStencil)),
            Shader = AZ_BIT(static_cast<uint32_t>(ScopeAttachmentUsage::Shader)),
            Copy = AZ_BIT(static_cast<uint32_t>(ScopeAttachmentUsage::Copy)),
            Resolve = AZ_BIT(static_cast<uint32_t>(ScopeAttachmentUsage::Resolve)),
            Predication = AZ_BIT(static_cast<uint32_t>(ScopeAttachmentUsage::Predication)),
            Indirect = AZ_BIT(static_cast<uint32_t>(ScopeAttachmentUsage::Indirect)),
            SubpassInput = AZ_BIT(static_cast<uint32_t>(ScopeAttachmentUsage::SubpassInput)),
            InputAssembly = AZ_BIT(static_cast<uint32_t>(ScopeAttachmentUsage::InputAssembly)),
            All = RenderTarget | DepthStencil | Shader | Copy | Resolve | Predication | Indirect | SubpassInput | InputAssembly
        };

        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::ScopeAttachmentUsageMask)

        const char* ToString(ScopeAttachmentUsage attachmentUsage);

        //! Modifies access to fit the constraints of the scope attachment usage. For example, a scope attachment
        //! with the usage 'Shader' and 'Write' access becomes a UAV under the hood, so it should be remapped to 'ReadWrite'.
        ScopeAttachmentAccess AdjustAccessBasedOnUsage(ScopeAttachmentAccess access, ScopeAttachmentUsage usage);

        //! Describes the three major logical classes of GPU hardware queues. Each queue class is a superset
        //! of the next. Graphics can do everything, compute can do compute / copy, and copy can only do copy
        //! operations. Scopes can be assigned a queue class, which gives hints to the scheduler for async
        //! queue operations.
        enum class HardwareQueueClass : uint32_t
        {
            //! Supports graphics, compute, and copy operations.
            Graphics = 0,

            //! Supports compute and copy operations.
            Compute,

            //! Supports only copy operations.
            Copy,

            Count
        };

        const uint32_t HardwareQueueClassCount = static_cast<uint32_t>(HardwareQueueClass::Count);

        const char* ToString(HardwareQueueClass hardwareClass);

        //! Describes hardware queues as a mask, where each bit represents the queue family.
        enum class HardwareQueueClassMask : uint32_t
        {
            None        = 0,
            Graphics    = AZ_BIT(static_cast<uint32_t>(HardwareQueueClass::Graphics)),
            Compute     = AZ_BIT(static_cast<uint32_t>(HardwareQueueClass::Compute)),
            Copy        = AZ_BIT(static_cast<uint32_t>(HardwareQueueClass::Copy)),
            All         = Graphics | Compute | Copy
        };
        AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::HardwareQueueClassMask)

        //! Returns the hardware queue class mask bit associated with the enum value.
        HardwareQueueClassMask GetHardwareQueueClassMask(HardwareQueueClass hardwareQueueClass);

        //! Returns the name associated with the hardware queue.
        const char* GetHardwareQueueClassName(HardwareQueueClass hardwareQueueClass);

        //! Scans the bit mask and returns the most capable queue from the set.
        HardwareQueueClass GetMostCapableHardwareQueue(HardwareQueueClassMask queueMask);

        //! Returns whether the first queue is more capable than the second queue.
        bool IsHardwareQueueMoreCapable(HardwareQueueClass queueA, HardwareQueueClass queueB);

        //! Describes the action the hardware should use when loading an attachment prior to a scope.
        enum class AttachmentLoadAction : uint8_t
        {
            //! The attachment contents should be preserved (loaded from memory).
            Load = 0,

            //! The attachment contents should be cleared (using the provided clear value).
            Clear,

            //! The attachment contents are undefined. Use when writing to entire contents of view.
            DontCare
        };

        //! Describes the action the hardware should use when storing an attachment after a scope.
        enum class AttachmentStoreAction : uint8_t
        {
            //! The attachment contents must be preserved after the current scope.
            Store = 0,

            //! The attachment contents can be undefined after the current scope.
            DontCare
        };

        //! Describes the type of data the attachment represents
        enum class AttachmentType : uint8_t
        {
            //! The attachment is an image.
            Image = 0,

            //! The attachment is a buffer.
            Buffer,

            //! The attachment is a resolve (for example resolving an MSAA texture)
            Resolve,

            //! Error value to catch uninitialized usage of this enum
            Uninitialized,
        };

        const char* ToString(AZ::RHI::AttachmentType attachmentType);

        //! Describes the type of scope attachment for a QueryPool
        enum class QueryPoolScopeAttachmentType : uint8_t
        {
            Local, // The results of the queries will be use by another scope in the graph.
            Global // The results of the queries will be accessed in subsequent frames.
        };

        //! Describes the type of support for Subpass inputs.
        enum class SubpassInputSupportType : uint8_t
        {
            NotSupported = 0,
            Native,             // Subpass inputs are supported natively by the platform.
            Emulated            // Subpass inputs are supported through emulation with texture views.
        };
    }

    AZ_TYPE_INFO_SPECIALIZE(RHI::ScopeAttachmentAccess, "{C937CE07-7ADD-423E-BB2B-2ED2AE8DAB8F}");
    AZ_TYPE_INFO_SPECIALIZE(RHI::AttachmentLifetimeType, "{DE636A9A-FA57-49E6-B10D-BCEF25093797}");
    AZ_TYPE_INFO_SPECIALIZE(RHI::ScopeAttachmentUsage, "{A3F9FAAC-30A3-4230-9F9B-F4EB5B1A593C}");
    AZ_TYPE_INFO_SPECIALIZE(RHI::HardwareQueueClass, "{AA3D6C1D-C1B1-48A2-A56B-1A41A96B75DE}");
    AZ_TYPE_INFO_SPECIALIZE(RHI::HardwareQueueClassMask, "{D7577768-5F44-4128-93A4-DDC85CF69B71}");
    AZ_TYPE_INFO_SPECIALIZE(RHI::AttachmentLoadAction, "{1DB7E288-1C11-4316-B6A8-8D62BA963541}");
    AZ_TYPE_INFO_SPECIALIZE(RHI::AttachmentStoreAction, "{F580ED24-1537-47D8-90D6-2E620087BE14}");
    AZ_TYPE_INFO_SPECIALIZE(RHI::AttachmentType, "{41A254E8-C4BF-459A-80D8-5B959501943E}");
}
