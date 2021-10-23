/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <math.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <Atom/RPI.Reflect/Pass/PassAttachmentReflect.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>

namespace AZ
{
    namespace RPI
    {
        RHI::ScopeAttachmentAccess GetAttachmentAccess(PassSlotType slotType)
        {
            return RHI::ScopeAttachmentAccess(uint32_t(slotType));
        }
        
        const char* ToString(AZ::RPI::PassSlotType slotType)
        {
            switch (slotType)
            {
            case AZ::RPI::PassSlotType::Input:
                return "Input";
            case AZ::RPI::PassSlotType::InputOutput:
                return "InputOutput";
            case AZ::RPI::PassSlotType::Output:
                return "Output";
            case AZ::RPI::PassSlotType::Uninitialized:
            default:
                return "Uninitialized";
            }
        }

        // --- PassSlot ---

        RHI::ScopeAttachmentAccess PassSlot::GetAttachmentAccess() const
        {
            return RPI::GetAttachmentAccess(m_slotType);
        }

        void PassSlot::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Enum<PassSlotType>()
                    ->Value("Input", PassSlotType::Input)
                    ->Value("Output", PassSlotType::Output)
                    ->Value("InputOutput", PassSlotType::InputOutput)
                    ->Value("Uninitialized", PassSlotType::Uninitialized)
                    ;

                serializeContext->Class<PassSlot>()
                    ->Version(2)
                    ->Field("Name", &PassSlot::m_name)
                    ->Field("ShaderInputName", &PassSlot::m_shaderInputName)
                    ->Field("ShaderImageDimensionsConstant", &PassSlot::m_shaderImageDimensionsName)
                    ->Field("ShaderInputArrayIndex", &PassSlot::m_shaderInputArrayIndex)
                    ->Field("SlotType", &PassSlot::m_slotType)
                    ->Field("ScopeAttachmentUsage", &PassSlot::m_scopeAttachmentUsage)
                    ->Field("ImageViewDesc", &PassSlot::m_imageViewDesc)
                    ->Field("BufferViewDesc", &PassSlot::m_bufferViewDesc)
                    ->Field("LoadStoreAction", &PassSlot::m_loadStoreAction)
                    ->Field("FormatFallbacks", &PassSlot::m_formatFallbacks)
                    ->Field("FormatFilter", &PassSlot::m_formatFilter)
                    ->Field("DimensionFilter", &PassSlot::m_dimensionFilter)
                    ;
            }
        }

        template<typename FilterType>
        bool FilterListAcceptsInput(const AZStd::vector<FilterType>& filterList, FilterType input)
        {
            // If no filters are specified then we accept everything
            if (filterList.empty())
            {
                return true;
            }

            // Loop over the specified filter. If one matches the input, return true.
            for (const FilterType& filter : filterList)
            {
                if (input == filter)
                {
                    return true;
                }
            }

            // The input was not contained in the list of filters. Return false.
            return false;
        }

        bool PassSlot::AcceptsFormat(const RHI::UnifiedAttachmentDescriptor& desc) const
        {
            // Only filtering image attachments since the BufferDescriptor class has neither size nor format
            if(desc.m_type == RHI::AttachmentType::Image)
            {
                return FilterListAcceptsInput(m_formatFilter, desc.m_image.m_format);
            }
            return true;
        }

        bool PassSlot::AcceptsDimension(const RHI::UnifiedAttachmentDescriptor& desc) const
        {
            // Only filtering image attachments since the BufferDescriptor class has neither size nor format
            if (desc.m_type == RHI::AttachmentType::Image)
            {
                return FilterListAcceptsInput(m_dimensionFilter, desc.m_image.m_dimension);
            }
            return true;
        }


        // --- PassAttachmentRef ---

        void PassAttachmentRef::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PassAttachmentRef>()
                    ->Version(0)
                    ->Field("Pass", &PassAttachmentRef::m_pass)
                    ->Field("Attachment", &PassAttachmentRef::m_attachment)
                    ;
            }
        }

        // --- PassConnection ---

        void PassConnection::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PassConnection>()
                    ->Version(0)
                    ->Field("LocalSlot", &PassConnection::m_localSlot)
                    ->Field("AttachmentRef", &PassConnection::m_attachmentRef)
                    ;
            }
        }

        // --- PassFallbackConnection ---

        void PassFallbackConnection::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PassFallbackConnection>()
                    ->Version(0)
                    ->Field("Input", &PassFallbackConnection::m_inputSlotName)
                    ->Field("Output", &PassFallbackConnection::m_outputSlotName)
                    ;
            }
        }

        // --- PassAttachmentSizeMultipliers ---

        void PassAttachmentSizeMultipliers::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PassAttachmentSizeMultipliers>()
                    ->Version(0)
                    ->Field("WidthMultiplier", &PassAttachmentSizeMultipliers::m_widthMultiplier)
                    ->Field("HeightMultiplier", &PassAttachmentSizeMultipliers::m_heightMultiplier)
                    ->Field("DepthMultiplier", &PassAttachmentSizeMultipliers::m_depthMultiplier)
                    ;
            }
        }

        RHI::Size PassAttachmentSizeMultipliers::ApplyModifiers(const RHI::Size& size) const
        {
            RHI::Size newSize;
            newSize.m_width = uint32_t(ceil(float(size.m_width) * m_widthMultiplier));
            newSize.m_height = uint32_t(ceil(float(size.m_height) * m_heightMultiplier));
            newSize.m_depth = uint32_t(ceil(float(size.m_depth) * m_depthMultiplier));
            return newSize;
        }

        // --- PassAttachmentSizeSource ---

        void PassAttachmentSizeSource::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PassAttachmentSizeSource>()
                    ->Version(0)
                    ->Field("Source", &PassAttachmentSizeSource::m_source)
                    ->Field("Multipliers", &PassAttachmentSizeSource::m_multipliers)
                    ;
            }
        }

        // --- PassAttachmentDesc ---

        void PassAttachmentDesc::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PassAttachmentDesc>()
                    ->Version(2)    // Removing PassAttachmentArraySizeSource class
                    ->Field("Name", &PassAttachmentDesc::m_name)
                    ->Field("Lifetime", &PassAttachmentDesc::m_lifetime)
                    ->Field("SizeSource", &PassAttachmentDesc::m_sizeSource)
                    ->Field("ArraySizeSource", &PassAttachmentDesc::m_arraySizeSource)
                    ->Field("FormatSource", &PassAttachmentDesc::m_formatSource)
                    ->Field("MultisampleSource", &PassAttachmentDesc::m_multisampleSource)
                    ->Field("AssetRef", &PassAttachmentDesc::m_assetRef)
                    ;
            }
        }

        // --- PassImageAttachmentDesc ---

        void PassImageAttachmentDesc::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PassImageAttachmentDesc, PassAttachmentDesc>()
                    ->Version(0)
                    ->Field("ImageDescriptor", &PassImageAttachmentDesc::m_imageDescriptor)
                    ->Field("GenerateFullMipChain", &PassImageAttachmentDesc::m_generateFullMipChain)
                    ->Field("FormatFallbacks", &PassImageAttachmentDesc::m_formatFallbacks)
                    ;
            }
        }

        // --- PassBufferAttachmentDesc ---

        void PassBufferAttachmentDesc::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<PassBufferAttachmentDesc, PassAttachmentDesc>()
                    ->Version(0)
                    ->Field("BufferDescriptor", &PassBufferAttachmentDesc::m_bufferDescriptor)
                    ;
            }
        }

    }   // namespace RPI
}   // namespace AZ
