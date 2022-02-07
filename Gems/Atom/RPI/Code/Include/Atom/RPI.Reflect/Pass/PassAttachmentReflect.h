/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/AttachmentId.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <Atom/RHI.Reflect/UnifiedAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/UnifiedScopeAttachmentDescriptor.h>

#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Asset/AssetReference.h>

#include <AtomCore/std/containers/array_view.h>

#include <AzCore/std/containers/array.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>

namespace AZ
{
    namespace RPI
    {
        // --- Pass Attachment Slots & Connections ---

        //! Indicates whether a pass slot is an Input, an Output or an InputOuput
        enum class PassSlotType : uint32_t
        {
            Input = uint32_t(RHI::ScopeAttachmentAccess::Read),
            Output = uint32_t(RHI::ScopeAttachmentAccess::Write),
            InputOutput = uint32_t(RHI::ScopeAttachmentAccess::ReadWrite),
            Uninitialized
        };

        //! Mask values for the above enum. These are used to ignore certain slot
        //! types when using functions that iterate over a list of slots/bindings
        enum class PassSlotMask : uint32_t
        {
            Input = 1 << uint32_t(PassSlotType::Input),
            Output = 1 << uint32_t(PassSlotType::Output),
            InputOutput = 1 << uint32_t(PassSlotType::InputOutput)
        };

        //! Takes PassSlotType and returns the corresponding ScopeAttachmentAccess
        RHI::ScopeAttachmentAccess GetAttachmentAccess(PassSlotType slotType);

        //! Convert PassSlotType to a string
        const char* ToString(AZ::RPI::PassSlotType slotType);

        //! A slot for a PassAttachment to be bound to a Pass. Specifies what kind of
        //! PassAttachments can be bound as well as how the Pass will use the attachment.
        //! PassSlots and PassConnections are used to initialize PassAttachmentBindings.
        struct PassSlot
        {
            PassSlot() { };
            ~PassSlot() { };

            AZ_TYPE_INFO(PassSlot, "{35150886-D1E4-40CB-AF7B-C607E893CD03}");
            static void Reflect(AZ::ReflectContext* context);

            //! Returns the corresponding ScopeAttachmentAccess for this slot
            RHI::ScopeAttachmentAccess GetAttachmentAccess() const;

            //! Returns true if the filters allow the given format
            bool AcceptsFormat(const RHI::UnifiedAttachmentDescriptor& desc) const;

            //! Returns true if the filters allow the given image dimension
            bool AcceptsDimension(const RHI::UnifiedAttachmentDescriptor& desc) const;

            //! Name of the slot
            Name m_name;

            //! Name of the shader resource group member this slot binds to.
            //! The keyword "AutoBind" (default value) means buffer and image indices will be auto calculated based on the
            //! order of the slots (note: for this to work the slot order must match the order of the ShaderResourceGroup members)
            //! The keyword "NoBind" means the slot will not bind it's attachment to the SRG
            Name m_shaderInputName = Name("AutoBind");

            //! Name of the shader resource group constant (must be float4) to which the pass can automatically bind the following:
            //! X component = image width
            //! Y component = image height
            //! Z component = 1 / image width
            //! W component = 1 / image height
            Name m_shaderImageDimensionsName;

            //! This is to specify an array index if the shader input is an array.
            //! e.g. Texture2DMS<float4> m_color[4];
            uint16_t m_shaderInputArrayIndex = 0;

            //! Whether slot is an input, output or inputOutput
            PassSlotType m_slotType = PassSlotType::Uninitialized;
             
            //! ScopeAttachmentUsage used when binding the slot's attachment with the RHI
            RHI::ScopeAttachmentUsage m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Uninitialized;

            //! Optional image view descriptor to be applied to the slot. Note a PassSlot should have only
            //! a buffer or image view descriptor (or none at all, in which case a default is generated),
            //! but not both. If the user specifies both, the image descriptor will take precedence.
            //! If none is specified, we apply a default image or buffer view descriptor depending on attachment type.
            AZStd::shared_ptr<RHI::ImageViewDescriptor> m_imageViewDesc = nullptr;

            //! Optional buffer view descriptor to be applied to the slot. Note a PassSlot should have only
            //! a buffer or image view descriptor (or none at all, in which case a default is generated),
            //! but not both. If the user specifies both, the image descriptor will take precedence.
            //! If none is specified, we apply a default image or buffer view descriptor depending on attachment type.
            AZStd::shared_ptr<RHI::BufferViewDescriptor> m_bufferViewDesc = nullptr;

            //! Load store action for the attachment used by this slot
            RHI::AttachmentLoadStoreAction m_loadStoreAction;

            //! List of formats to fallback to if the format specified in the view descriptor is not supported by the device
            AZStd::vector<RHI::Format> m_formatFallbacks;

            //! List of allowed formats for the input. If list is empty, the input accepts all formats
            AZStd::vector<RHI::Format> m_formatFilter;

            //! List of allowed image dimensions for the input. If empty, the input accepts all dimensions
            AZStd::vector<RHI::ImageDimension> m_dimensionFilter;
        };

        using PassSlotList = AZStd::vector<PassSlot>;
        using PassSlotListView = AZStd::array_view<PassSlot>;

        //! Refers to a PassAttachment or a PassAttachmentBinding on an adjacent Pass in the hierarchy. Specifies the
        //! name of attachment or binding/slot as well as the name of the Pass on which the attachment or binding lives.
        //! Note: There are several keywords that can be used for the pass name with special effects:
        //! 'This' keyword will cause the pass to search for the attachment on itself
        //! 'Parent' keyword will cause the pass to search for the attachment on it's parent pass
        //! 'Pipeline' keyword will cause the pass to get settings directly from the render pipeline (attachment name is ignored)
        struct PassAttachmentRef final
        {
            AZ_TYPE_INFO(PassAttachmentRef, "{BEA90E90-95AB-45DB-968C-9E269AA53FC5}");
            static void Reflect(AZ::ReflectContext* context);

            //! The name of the pass from which we want to get the attachment
            Name m_pass;

            //! The name of the source attachment. Can be used to reference either a PassSlot or a PassAttachment.
            Name m_attachment;
        };

        //! Specifies a connection from a Pass's slot to a slot on an adjacent Pass (parent, neighbor or child pass)
        //! or to an attachment owned by the Pass itself (in which case the connecting Name will be "This").
        //! PassConnections and PassSlots are used to initialize PassAttachmentBindings.
        struct PassConnection final
        {
            AZ_TYPE_INFO(PassConnection, "{AC5E6572-3D9E-4F94-BB28-373A3FB59E63}");
            static void Reflect(AZ::ReflectContext* context);

            //! The local slot on the Pass for which this connection is specified
            Name m_localSlot;

            //! The other end of the connection
            PassAttachmentRef m_attachmentRef;
        };

        using PassConnectionList = AZStd::vector<PassConnection>;
        using PassConnectionListView = AZStd::array_view<PassConnection>;

        //! Specifies a connection from a Pass's output slot to one of it's input slots. This is used as a fallback
        //! for the output when the pass is disabled so the output can present a valid attachments to subsequent passes.
        struct PassFallbackConnection final
        {
            AZ_TYPE_INFO(PassConnection, "{281C6C09-2BB8-49C0-967E-DF6A57DE1095}");
            static void Reflect(AZ::ReflectContext* context);

            //! Name of the input slot that will provide the fallback attachment
            Name m_inputSlotName;

            //! Name of the output slot that will use the fallback attachment from the specified input slot
            Name m_outputSlotName;
        };

        using PassFallbackConnectionList = AZStd::vector<PassFallbackConnection>;
        using PassFallbackConnectionListView = AZStd::array_view<PassFallbackConnection>;

        // --- Pass Attachment Descriptor Classes ---

        //! A set of multipliers used to obtain the size of an attachment from an existing attachment's size
        struct PassAttachmentSizeMultipliers final
        {
            AZ_TYPE_INFO(PassAttachmentSizeMultipliers, "{218DB53E-5B33-4DD1-AC23-9BADE4148EE6}");
            static void Reflect(AZ::ReflectContext* context);

            /// Takes a source size and returns that size with multipliers applied
            RHI::Size ApplyModifiers(const RHI::Size& size) const;

            float m_widthMultiplier = 1.0f;
            float m_heightMultiplier = 1.0f;
            float m_depthMultiplier = 1.0f;
        };

        //! Used to query an attachment size from a source attachment using a PassAttachmentRef
        //! The size of the attachment is then multiplied by the width, height and depth multipliers
        //! See Pass::CreateAttachmentFromDesc
        struct PassAttachmentSizeSource final
        {
            AZ_TYPE_INFO(PassAttachmentSizeSource, "{22B2D186-5496-4359-B430-7B6F2436916E}");
            static void Reflect(AZ::ReflectContext* context);

            //! The source attachment from which to calculate the size
            //! Use provided width and height if source is not specified
            PassAttachmentRef m_source;

            //! The source attachment's size will be multiplied by
            //! these values to obtain the new attachment's size
            PassAttachmentSizeMultipliers m_multipliers;
        };

        //! Describes a PassAttachment, used for building attachments in a data-driven manner.
        //! Can specify size source and format source to derive attachment size and format from
        //! an existing attachment.
        struct PassAttachmentDesc
        {
            AZ_TYPE_INFO(PassAttachmentDesc, "{79942700-3E86-48AC-8851-2148AFAFF8B7}");
            static void Reflect(AZ::ReflectContext* context);

            virtual ~PassAttachmentDesc() = default;

            //! The name of the pass attachment
            Name m_name;

            //! Whether the attachment is transient or not
            RHI::AttachmentLifetimeType m_lifetime = RHI::AttachmentLifetimeType::Transient;

            //! Used to data drive the size of the attachment from a specified source attachment
            PassAttachmentSizeSource m_sizeSource;

            //! Used to data drive the array size of the attachment from a specified source attachment
            PassAttachmentRef m_arraySizeSource;
            
            //! Used to data drive the format of the attachment from a specified source attachment
            PassAttachmentRef m_formatSource;

            //! Used to data drive the multi-sample state of the attachment from a specified source attachment
            PassAttachmentRef m_multisampleSource;

            //! Reference to an external attachment asset, which used for imported attachment
            AssetReference m_assetRef;
        };

        //! A PassAttachmentDesc used for images
        struct PassImageAttachmentDesc final
            : public PassAttachmentDesc
        {
            AZ_TYPE_INFO(PassImageAttachmentDesc, "{FA075E02-6A2E-4899-B888-B22DD052FCCC}");
            static void Reflect(AZ::ReflectContext* context);

            //! The image descriptor for the attachment
            RHI::ImageDescriptor m_imageDescriptor;

            //! Whether to auto generate the number of mips based on the attachment
            //! so that we get a full mip chain with the smallest mip being 1x1 in size
            bool m_generateFullMipChain = false;

            //! List of formats to fallback to if the format specified in the image descriptor is not supported by the device
            AZStd::vector<RHI::Format> m_formatFallbacks;
        };

        using PassImageAttachmentDescList = AZStd::vector<PassImageAttachmentDesc>;
        using PassImageAttachmentDescListView = AZStd::array_view<PassAttachmentDesc>;

        //! A PassAttachmentDesc used for buffers
        struct PassBufferAttachmentDesc final
            : public PassAttachmentDesc
        {
            AZ_TYPE_INFO(PassBufferAttachmentDesc, "{AD8F9866-954D-4169-8041-74B946A75747}");
            static void Reflect(AZ::ReflectContext* context);

            //! The buffer descriptor for the transient buffer attachment
            RHI::BufferDescriptor m_bufferDescriptor;
        };

        using PassBufferAttachmentDescList = AZStd::vector<PassBufferAttachmentDesc>;
        using PassBufferAttachmentDescListView = AZStd::array_view<PassBufferAttachmentDesc>;

    }   // namespace RPI

    AZ_TYPE_INFO_SPECIALIZE(RPI::PassSlotType, "{D0189293-1ABE-4672-BDE6-5652F4B3866C}");

}   // namespace AZ
