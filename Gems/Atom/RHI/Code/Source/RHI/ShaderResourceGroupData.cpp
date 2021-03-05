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
#include <Atom/RHI/ShaderResourceGroupData.h>
#include <Atom/RHI/ShaderResourceGroupPool.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferView.h>

namespace AZ
{
    namespace RHI
    {
        const ConstPtr<ImageView> ShaderResourceGroupData::s_nullImageView;
        const ConstPtr<BufferView> ShaderResourceGroupData::s_nullBufferView;
        const SamplerState ShaderResourceGroupData::s_nullSamplerState{};

        ShaderResourceGroupData::ShaderResourceGroupData() = default;
        ShaderResourceGroupData::~ShaderResourceGroupData() = default;

        ShaderResourceGroupData::ShaderResourceGroupData(const ShaderResourceGroup& shaderResourceGroup)
            : ShaderResourceGroupData(*shaderResourceGroup.GetPool())
        {}

        ShaderResourceGroupData::ShaderResourceGroupData(const ShaderResourceGroupPool& shaderResourceGroupPool)
            : ShaderResourceGroupData(shaderResourceGroupPool.GetLayout())
        {}

        ShaderResourceGroupData::ShaderResourceGroupData(const ShaderResourceGroupLayout* layout)
            : m_shaderResourceGroupLayout(layout)
            , m_constantsData(layout->GetConstantsLayout())
        {
            m_imageViews.resize(layout->GetGroupSizeForImages());
            m_bufferViews.resize(layout->GetGroupSizeForBuffers());
            m_samplers.resize(layout->GetGroupSizeForSamplers());
        }

        const ShaderResourceGroupLayout* ShaderResourceGroupData::GetLayout() const
        {
            return m_shaderResourceGroupLayout.get();
        }

        bool ShaderResourceGroupData::ValidateSetImageView(ShaderInputImageIndex inputIndex, const ImageView* imageView, uint32_t arrayIndex) const
        {
            if (!Validation::IsEnabled())
            {
                return true;
            }
            if (!GetLayout()->ValidateAccess(inputIndex, arrayIndex))
            {
                return false;
            }

            if (imageView)
            {
                const ShaderInputImageDescriptor& shaderInputImage = GetLayout()->GetShaderInput(inputIndex);
                const ImageViewDescriptor& imageViewDescriptor = imageView->GetDescriptor();
                const Image& image = imageView->GetImage();
                const ImageDescriptor& imageDescriptor = image.GetDescriptor();
                const ImageFrameAttachment* frameAttachment = image.GetFrameAttachment();

                // The image must have the correct bind flags for the slot.
                const bool isValidAccess =
                    (shaderInputImage.m_access == ShaderInputImageAccess::Read      && CheckBitsAll(imageDescriptor.m_bindFlags, ImageBindFlags::ShaderRead)) ||
                    (shaderInputImage.m_access == ShaderInputImageAccess::ReadWrite && CheckBitsAll(imageDescriptor.m_bindFlags, ImageBindFlags::ShaderReadWrite));

                if (!isValidAccess)
                {
                    AZ_Error("ShaderResourceGroupData", false,
                        "Image Input '%s[%d]': Invalid 'Read / Write' access. Expected '%s'.",
                        shaderInputImage.m_name.GetCStr(), arrayIndex, GetShaderInputAccessName(shaderInputImage.m_access));
                    return false;
                }

                if (shaderInputImage.m_access == ShaderInputImageAccess::ReadWrite)
                {
                    // An image view assigned to an input with read-write access must be an attachment on the frame scheduler.
                    if (!frameAttachment)
                    {
                        AZ_Error("ShaderResourceGroupData", false,
                            "Image Input '%s[%d]': Image is bound to a ReadWrite shader input, "
                            "but it is not an attachment on the frame scheduler. All GPU-writable resources "
                            "must be declared as attachments in order to provide hazard tracking.",
                            shaderInputImage.m_name.GetCStr(), arrayIndex, GetShaderInputAccessName(shaderInputImage.m_access));
                        return false;
                    }

                    // NOTE: We aren't able to validate the scope attachment here, because shader resource groups aren't directly
                    // associated with a scope. Instead, the CommandListValidator class will check that the access is correct at
                    // command list submission time.
                }

                bool isValidType = true;
                switch (shaderInputImage.m_type)
                {
                case ShaderInputImageType::Unknown:
                    // Unable to validate.
                    break;

                case ShaderInputImageType::Image1DArray:
                case ShaderInputImageType::Image1D:
                    isValidType &= (imageDescriptor.m_dimension == ImageDimension::Image1D);
                    break;

                case ShaderInputImageType::SubpassInput:
                    isValidType &= (imageDescriptor.m_dimension == ImageDimension::Image2D);
                    break;

                case ShaderInputImageType::Image2DArray:
                case ShaderInputImageType::Image2D:
                    isValidType &= (imageDescriptor.m_dimension == ImageDimension::Image2D);
                    isValidType &= (imageDescriptor.m_multisampleState.m_samples == 1);
                    break;

                case ShaderInputImageType::Image2DMultisample:
                case ShaderInputImageType::Image2DMultisampleArray:
                    isValidType &= (imageDescriptor.m_dimension == ImageDimension::Image2D);
                    isValidType &= (imageDescriptor.m_multisampleState.m_samples > 1);
                    break;

                case ShaderInputImageType::Image3D:
                    isValidType &= (imageDescriptor.m_dimension == ImageDimension::Image3D);
                    break;

                case ShaderInputImageType::ImageCube:
                case ShaderInputImageType::ImageCubeArray:
                    isValidType &= (imageDescriptor.m_dimension == ImageDimension::Image2D);
                    isValidType &= (imageViewDescriptor.m_isCubemap != 0);
                    break;

                default:
                    AZ_Assert(false, "Image Input '%s[%d]': Invalid image type!", shaderInputImage.m_name.GetCStr(), arrayIndex);
                    return false;
                }

                if (!isValidType)
                {
                    AZ_Error("ShaderResourceGroupData", false,
                        "Image Input '%s[%d]': Does not match expected type '%s'",
                        shaderInputImage.m_name.GetCStr(), arrayIndex, GetShaderInputTypeName(shaderInputImage.m_type));
                    return false;
                }
            }

            return true;
        }

        bool ShaderResourceGroupData::ValidateSetBufferView(ShaderInputBufferIndex inputIndex, const BufferView* bufferView, uint32_t arrayIndex) const
        {
            if (!Validation::IsEnabled())
            {
                return true;
            }
            if (!GetLayout()->ValidateAccess(inputIndex, arrayIndex))
            {
                return false;
            }

            if (bufferView)
            {
                const ShaderInputBufferDescriptor& shaderInputBuffer = GetLayout()->GetShaderInput(inputIndex);
                const BufferViewDescriptor& bufferViewDescriptor = bufferView->GetDescriptor();
                const Buffer& buffer = bufferView->GetBuffer();
                const BufferDescriptor& bufferDescriptor = buffer.GetDescriptor();
                const BufferFrameAttachment* frameAttachment = buffer.GetFrameAttachment();

                const bool isValidAccess =
                    (shaderInputBuffer.m_access == ShaderInputBufferAccess::Constant  && CheckBitsAll(bufferDescriptor.m_bindFlags, BufferBindFlags::Constant)) ||
                    (shaderInputBuffer.m_access == ShaderInputBufferAccess::Read      && CheckBitsAll(bufferDescriptor.m_bindFlags, BufferBindFlags::ShaderRead)) ||
                    (shaderInputBuffer.m_access == ShaderInputBufferAccess::Read      && CheckBitsAll(bufferDescriptor.m_bindFlags, BufferBindFlags::RayTracingAccelerationStructure)) ||
                    (shaderInputBuffer.m_access == ShaderInputBufferAccess::ReadWrite && CheckBitsAll(bufferDescriptor.m_bindFlags, BufferBindFlags::ShaderReadWrite));

                if (!isValidAccess)
                {
                    AZ_Error("ShaderResourceGroupData", false,
                       "Buffer Input '%s[%d]': Invalid 'Constant / Read / Write' access. Expected '%s'",
                        shaderInputBuffer.m_name.GetCStr(), arrayIndex, GetShaderInputAccessName(shaderInputBuffer.m_access));
                    return false;
                }

                if (shaderInputBuffer.m_access == ShaderInputBufferAccess::ReadWrite && !bufferView->IgnoreFrameAttachmentValidation())
                {
                    // A buffer view assigned to an input with read-write access must be an attachment on the frame scheduler.
                    if (!frameAttachment)
                    {
                        AZ_Error("ShaderResourceGroupData", false,
                            "Buffer Input '%s[%d]': Buffer is bound to a ReadWrite shader input, "
                            "but it is not an attachment on the frame scheduler. All GPU-writable resources "
                            "must be declared as attachments in order to provide hazard tracking.",
                            shaderInputBuffer.m_name.GetCStr(), arrayIndex, GetShaderInputAccessName(shaderInputBuffer.m_access));
                        return false;
                    }

                    // NOTE: We aren't able to validate the scope attachment here, because shader resource groups aren't directly
                    // associated with a scope. Instead, the CommandListValidator class will check that the access is correct at
                    // command list submission time.
                }

                if (shaderInputBuffer.m_type == ShaderInputBufferType::Constant)
                {
                    //For Constant type the stride (full constant buffer) must be larger or equal than the buffer view size (element size x element count).
                    if (!(shaderInputBuffer.m_strideSize >= (bufferViewDescriptor.m_elementSize * bufferViewDescriptor.m_elementCount)))
                    {
                        AZ_Error("ShaderResourceGroupData", false,
                            "Buffer Input '%s[%d]': stride size %d must be larger than or equal to the buffer view size %d",
                            shaderInputBuffer.m_name.GetCStr(), arrayIndex, shaderInputBuffer.m_strideSize,
                            (bufferViewDescriptor.m_elementSize * bufferViewDescriptor.m_elementCount));
                        return false;
                    }
                }
                else
                {
                    // For any other type the buffer view's element size should match the stride.
                    if (shaderInputBuffer.m_strideSize != bufferViewDescriptor.m_elementSize)
                    {
                        // [GFX TODO][ATOM-5735][AZSL] ByteAddressBuffer shader input is setting a stride of 16 instead of 4
                        AZ_Error("ShaderResourceGroupData", false, "Buffer Input '%s[%d]': Does not match expected stride size %d",
                            shaderInputBuffer.m_name.GetCStr(), arrayIndex, bufferViewDescriptor.m_elementSize);
                        return false;
                    }
                }

                bool isValidType = true;
                switch (shaderInputBuffer.m_type)
                {
                case ShaderInputBufferType::Unknown:
                    // Unable to validate.
                    break;

                case ShaderInputBufferType::Constant:
                    // Element format is not relevant for viewing a buffer as a constant buffer, any format would be valid.
                    break;

                case ShaderInputBufferType::Structured:
                    isValidType &= bufferViewDescriptor.m_elementFormat == Format::Unknown;
                    break;

                case ShaderInputBufferType::Typed:
                    isValidType &= bufferViewDescriptor.m_elementFormat != Format::Unknown;
                    break;

                case ShaderInputBufferType::Raw:
                    isValidType &= bufferViewDescriptor.m_elementFormat == Format::R32_UINT;
                    break;

                default:
                    AZ_Assert(false, "Buffer Input '%s[%d]': Invalid buffer type!", shaderInputBuffer.m_name.GetCStr(), arrayIndex);
                    return false;
                }

                if (!isValidType)
                {
                    AZ_Error("ShaderResourceGroupData", false,
                        "Buffer Input '%s[%d]': Does not match expected type '%s'",
                        shaderInputBuffer.m_name.GetCStr(), arrayIndex, GetShaderInputTypeName(shaderInputBuffer.m_type));
                    return false;
                }
            }

            return true;
        }

        ShaderInputBufferIndex ShaderResourceGroupData::FindShaderInputBufferIndex(const Name& name) const
        {
            return m_shaderResourceGroupLayout->FindShaderInputBufferIndex(name);
        }

        ShaderInputImageIndex ShaderResourceGroupData::FindShaderInputImageIndex(const Name& name) const
        {
            return m_shaderResourceGroupLayout->FindShaderInputImageIndex(name);
        }

        ShaderInputSamplerIndex ShaderResourceGroupData::FindShaderInputSamplerIndex(const Name& name) const
        {
            return m_shaderResourceGroupLayout->FindShaderInputSamplerIndex(name);
        }

        ShaderInputConstantIndex ShaderResourceGroupData::FindShaderInputConstantIndex(const Name& name) const
        {
            return m_shaderResourceGroupLayout->FindShaderInputConstantIndex(name);
        }

        bool ShaderResourceGroupData::SetImageView(ShaderInputImageIndex inputIndex, const ImageView* imageView)
        {
            AZStd::array<const ImageView*, 1> imageViews = {{imageView}};
            return SetImageViewArray(inputIndex, imageViews);
        }

        bool ShaderResourceGroupData::SetImageView(ShaderInputImageIndex inputIndex, const ImageView* imageView, uint32_t arrayIndex)
        {
            AZStd::array<const ImageView*, 1> imageViews = {{imageView}};
            return SetImageViewArray(inputIndex, imageViews, arrayIndex);
        }

        bool ShaderResourceGroupData::SetImageViewArray(ShaderInputImageIndex inputIndex, AZStd::array_view<const ImageView*> imageViews)
        {
            return SetImageViewArray(inputIndex, imageViews, 0);
        }

        bool ShaderResourceGroupData::SetImageViewArray(ShaderInputImageIndex inputIndex, AZStd::array_view<const ImageView*> imageViews, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + imageViews.size() - 1)))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                bool isValidAll = true;
                for (size_t i = 0; i < imageViews.size(); ++i)
                {
                    const bool isValid = ValidateSetImageView(inputIndex, imageViews[i], aznumeric_caster(arrayIndex + i));
                    if (isValid)
                    {
                        m_imageViews[interval.m_min + arrayIndex + i] = imageViews[i];
                    }
                    isValidAll &= isValid;
                }
                return isValidAll;
            }
            return false;
        }

        bool ShaderResourceGroupData::SetBufferView(ShaderInputBufferIndex inputIndex, const BufferView* bufferView)
        {
            AZStd::array<const BufferView*, 1> bufferViews = {{bufferView}};
            return SetBufferViewArray(inputIndex, bufferViews, 0);
        }

        bool ShaderResourceGroupData::SetBufferView(ShaderInputBufferIndex inputIndex, const BufferView* bufferView, uint32_t arrayIndex)
        {
            AZStd::array<const BufferView*, 1> bufferViews = {{bufferView}};
            return SetBufferViewArray(inputIndex, bufferViews, arrayIndex);
        }

        bool ShaderResourceGroupData::SetBufferViewArray(ShaderInputBufferIndex inputIndex, AZStd::array_view<const BufferView*> bufferViews)
        {
            return SetBufferViewArray(inputIndex, bufferViews, 0);
        }

        bool ShaderResourceGroupData::SetBufferViewArray(ShaderInputBufferIndex inputIndex, AZStd::array_view<const BufferView*> bufferViews, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + bufferViews.size() - 1)))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                bool isValidAll = true;
                for (size_t i = 0; i < bufferViews.size(); ++i)
                {
                    const bool isValid = ValidateSetBufferView(inputIndex, bufferViews[i], arrayIndex);
                    if (isValid)
                    {
                        m_bufferViews[interval.m_min + arrayIndex + i] = bufferViews[i];
                    }
                    isValidAll &= isValid;
                }
                return isValidAll;
            }
            return false;
        }

        bool ShaderResourceGroupData::SetSampler(ShaderInputSamplerIndex inputIndex, const SamplerState& sampler)
        {
            return SetSamplerArray(inputIndex, AZStd::array_view<SamplerState>(&sampler, 1), 0);
        }

        bool ShaderResourceGroupData::SetSampler(ShaderInputSamplerIndex inputIndex, const SamplerState& sampler, uint32_t arrayIndex)
        {
            return SetSamplerArray(inputIndex, AZStd::array_view<SamplerState>(&sampler, 1), arrayIndex);
        }

        bool ShaderResourceGroupData::SetSamplerArray(ShaderInputSamplerIndex inputIndex, AZStd::array_view<SamplerState> samplers)
        {
            return SetSamplerArray(inputIndex, samplers, 0);
        }

        bool ShaderResourceGroupData::SetSamplerArray(ShaderInputSamplerIndex inputIndex, AZStd::array_view<SamplerState> samplers, uint32_t arrayIndex)
        {
            if (GetLayout()->ValidateAccess(inputIndex, static_cast<uint32_t>(arrayIndex + samplers.size() - 1)))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                for (size_t i = 0; i < samplers.size(); ++i)
                {
                    m_samplers[interval.m_min + arrayIndex + i] = samplers[i];
                }
                return true;
            }
            return false;
        }        

        bool ShaderResourceGroupData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount)
        {
            return SetConstantRaw(inputIndex, bytes, 0, byteCount);
        }

        bool ShaderResourceGroupData::SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount)
        {
            return m_constantsData.SetConstantRaw(inputIndex, bytes, byteOffset, byteCount);
        }

        bool ShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteCount)
        {
            return m_constantsData.SetConstantData(bytes, byteCount);
        }

        bool ShaderResourceGroupData::SetConstantData(const void* bytes, uint32_t byteOffset, uint32_t byteCount)
        {
            return m_constantsData.SetConstantData(bytes, byteOffset, byteCount);
            }
        
        const RHI::ConstPtr<RHI::ImageView>& ShaderResourceGroupData::GetImageView(RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                return m_imageViews[interval.m_min + arrayIndex];
            }
            return s_nullImageView;
        }

        AZStd::array_view<RHI::ConstPtr<RHI::ImageView>> ShaderResourceGroupData::GetImageViewArray(RHI::ShaderInputImageIndex inputIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex, 0))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                return AZStd::array_view<RHI::ConstPtr<RHI::ImageView>>(&m_imageViews[interval.m_min], interval.m_max - interval.m_min);
            }
            return {};
        }

        const RHI::ConstPtr<RHI::BufferView>& ShaderResourceGroupData::GetBufferView(RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                return m_bufferViews[interval.m_min + arrayIndex];
            }
            return s_nullBufferView;
        }

        AZStd::array_view<RHI::ConstPtr<RHI::BufferView>> ShaderResourceGroupData::GetBufferViewArray(RHI::ShaderInputBufferIndex inputIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex, 0))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                return AZStd::array_view<RHI::ConstPtr<RHI::BufferView>>(&m_bufferViews[interval.m_min], interval.m_max - interval.m_min);
            }
            return {};
        }

        const RHI::SamplerState& ShaderResourceGroupData::GetSampler(RHI::ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const
        {
            if (GetLayout()->ValidateAccess(inputIndex, arrayIndex))
            {
                const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
                return m_samplers[interval.m_min + arrayIndex];
            }
            return s_nullSamplerState;
        }

        AZStd::array_view<RHI::SamplerState> ShaderResourceGroupData::GetSamplerArray(RHI::ShaderInputSamplerIndex inputIndex) const
        {
            const Interval interval = GetLayout()->GetGroupInterval(inputIndex);
            return AZStd::array_view<RHI::SamplerState>(&m_samplers[interval.m_min], interval.m_max - interval.m_min);
        }

        AZStd::array_view<uint8_t> ShaderResourceGroupData::GetConstantRaw(ShaderInputConstantIndex inputIndex) const
        {
            return m_constantsData.GetConstantRaw(inputIndex);
        }

        AZStd::array_view<ConstPtr<ImageView>> ShaderResourceGroupData::GetImageGroup() const
        {
            return m_imageViews;
        }

        AZStd::array_view<ConstPtr<BufferView>> ShaderResourceGroupData::GetBufferGroup() const
        {
            return m_bufferViews;
        }

        AZStd::array_view<SamplerState> ShaderResourceGroupData::GetSamplerGroup() const
        {
            return m_samplers;
        }

        AZStd::array_view<uint8_t> ShaderResourceGroupData::GetConstantData() const
        {
            return m_constantsData.GetConstantData();
        }

    } // namespace RHI
} // namespace AZ
