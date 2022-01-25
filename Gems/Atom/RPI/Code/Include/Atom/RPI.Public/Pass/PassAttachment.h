/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomCore/Instance/Instance.h>
#include <AtomCore/Instance/InstanceData.h>

#include <Atom/RHI.Reflect/ShaderInputNameIndex.h>

#include <Atom/RPI.Reflect/Pass/PassAttachmentReflect.h>

namespace AZ
{
    namespace RPI
    {
        struct PassAttachmentBinding;
        class Pass;
        class RenderPipeline;

        //! Describes an attachment to be used by a Pass.
        struct PassAttachment final
            : AZStd::intrusive_refcount<AZStd::atomic_uint, AZStd::intrusive_default_delete>
        {
            AZ_CLASS_ALLOCATOR(PassAttachment, SystemAllocator, 0);

            PassAttachment() = default;
            PassAttachment(const PassImageAttachmentDesc& attachmentDesc);
            PassAttachment(const PassBufferAttachmentDesc& attachmentDesc);

            Ptr<PassAttachment> Clone() const;

            //! Returns the AttachmentId used to bind the attachment with the RHI
            RHI::AttachmentId GetAttachmentId() const;

            //! Returns the type of this attachment (image, buffer)
            RHI::AttachmentType GetAttachmentType() const;

            //! Takes the path of the owning pass, concatenates it with name and stores the path
            void ComputePathName(const Name& passPath);

            //! Creates a TransientImageDescriptor from the image descriptor. Only use this if the attachment type is Image
            const RHI::TransientImageDescriptor GetTransientImageDescriptor() const;

            //! Creates a TransientBufferDescriptor from the buffer descriptor. Only use this if the attachment type is Buffer.
            const RHI::TransientBufferDescriptor GetTransientBufferDescriptor() const;

            //! Updates the size and format of this attachment using the sources below if specified
            //! @param updateImportedAttachments - Imported attchments will only update if this is true.
            void Update(bool updateImportedAttachments = false);

            //! Sets all formats to nearest device supported formats and warns if changes where made
            void ValidateDeviceFormats(const AZStd::vector<RHI::Format>& formatFallbacks, RHI::FormatCapabilities capabilities = RHI::FormatCapabilities::None);

            //! Called when a PassAttachmentBinding sets it's attachment to this
            void OnAttached(const PassAttachmentBinding& binding);

            //! Name of the attachment
            Name m_name;

            //! Path of the attachment (path of the owning pass + name)
            //! This is the Id used to bind the attachment with the RHI
            RHI::AttachmentId m_path;

            //! A descriptor of the attachment image
            RHI::UnifiedAttachmentDescriptor m_descriptor;

            //! Whether the attachment is transient or not
            RHI::AttachmentLifetimeType m_lifetime = RHI::AttachmentLifetimeType::Transient;

            //! The source attachment from which to derive this attachment's format
            //! If null, keep this attachment's format as is
            const PassAttachmentBinding* m_formatSource = nullptr;

            //! The source attachment from which to derive this attachment's multi-sample state
            //! If null, keep this attachment's multi-sample state as is
            const PassAttachmentBinding* m_multisampleSource = nullptr;

            //! The source attachment from which to derive this attachment's size
            //! If null, keep this attachment's size as is
            const PassAttachmentBinding* m_sizeSource = nullptr;

            //! Multiply source size by these values to obtain new size
            PassAttachmentSizeMultipliers m_sizeMultipliers;

            //! The source attachment from which to derive this attachment's array size
            //! If null, keep this attachment's array size as is
            const PassAttachmentBinding* m_arraySizeSource = nullptr;

            //! The render pipeline to use when querying render source settings for size, format, multisample state, etc.
            RenderPipeline* m_renderPipelineSource = nullptr;

            //! Whether to auto generate the number of mips based on the attachment
            //! so that we get a full mip chain with the smallest mip being 1x1 in size
            bool m_generateFullMipChain = false;

            //! The resource's instance of this attachment if the attachment is imported (which m_lifetime is Imported)
            Data::Instance<Data::InstanceData> m_importedResource;

            //! Reference to owner pass
            Pass* m_ownerPass = nullptr;

            //! Collection of flags that influence how source data is queried
            struct
            {
                union
                {
                    struct
                    {
                        u8 m_getSizeFromPipeline : 1;
                        u8 m_getFormatFromPipeline : 1;
                        u8 m_getMultisampleStateFromPipeline : 1;
                    };
                    u8 m_allFlags = 0;
                };
            } m_settingFlags;
        };

        //! An attachment binding points to a PassAttachment and specifies how the pass uses that attachment.
        //! In data driven usages, a PassAttachmentBinding is constructed from a PassSlot (specifies how to
        //! use the attachment) and a PassConnection (specifies which attachment to use).
        //! 
        //! A attachment binding can point to another attachment binding, which means it is connected to
        //! that binding. In this case, the attachment pointed to by the connected binding will be used.
        //! Example: an input binding can point to another Pass's output binding, in which case the
        //! input binding will refer to the same attachment at the connected output binding.
        struct PassAttachmentBinding final
        {
            PassAttachmentBinding() { };
            PassAttachmentBinding(const PassSlot& slot);
            ~PassAttachmentBinding() { };

            void SetAttachment(const Ptr<PassAttachment>& attachment);

            //! Returns the corresponding ScopeAttachmentAccess for this binding
            RHI::ScopeAttachmentAccess GetAttachmentAccess() const;

            //! Sets all formats to nearest device supported formats and warns if changes where made
            void ValidateDeviceFormats(const AZStd::vector<RHI::Format>& formatFallbacks);

            //! Name of the attachment binding so we can find it in a list of attachment binding
            Name m_name;

            //! Name of the SRG member this binds to (see PassSlot::m_shaderInputName for more details)
            Name m_shaderInputName = Name("AutoBind");

            //! Name index of the SRG constant to which, if specified, we automatically calculate
            //! and bind the image dimensions (if this binding is of type image)
            RHI::ShaderInputNameIndex m_shaderImageDimensionsNameIndex;

            //! Whether binding is an input, output or inputOutput
            PassSlotType m_slotType = PassSlotType::Uninitialized;

            //! ScopeAttachmentUsage used when binding the attachment with the RHI
            RHI::ScopeAttachmentUsage m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Uninitialized;

            //! The scope descriptor to be used for this binding during rendering
            RHI::UnifiedScopeAttachmentDescriptor m_unifiedScopeDesc;

            //! Pointer to the attachment used by the scope
            Ptr<PassAttachment> m_attachment = nullptr;

            //! Save the original attachment when using fallback
            Ptr<PassAttachment> m_originalAttachment = nullptr;

            //! Pointer to the binding slot connected to this binding slot
            PassAttachmentBinding* m_connectedBinding = nullptr;

            //! Only used if this PassAttachmentBinding is an output, in which case
            //! this is the fallback we will use when the pass is disabled.
            PassAttachmentBinding* m_fallbackBinding = nullptr;

            static const int16_t ShaderInputAutoBind = -1;
            static const int16_t ShaderInputNoBind = -2;

            //! This tracks which SRG slot to bind the attachment to. This value gets applied in RenderPass::BindPassSrg
            //! after being converted to either an RHI::ShaderInputImageIndex or an RHI::ShaderInputBufferIndex using
            //! the specified shader name (see m_shaderInputName)
            int16_t m_shaderInputIndex = ShaderInputAutoBind;

            // This is to specify an array index if the shader input is an array.
            // e.g. Texture2DMS<float4> m_color[4];
            uint16_t m_shaderInputArrayIndex = 0;

            //! An attachment can be used multiple times by the same pass (for example reading an writing to different
            //! mips of the same texture). This indicates which number usage this binding corresponds to.
            uint8_t m_attachmentUsageIndex = 0;
        };

        using PassAttachmentBindingList = AZStd::vector<PassAttachmentBinding>;
        using PassAttachmentBindingListView = AZStd::array_view<PassAttachmentBinding>;

    }   // namespace RPI

}   // namespace AZ
