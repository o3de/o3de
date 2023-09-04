/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI/ConstantsData.h>
#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceImage.h>
#include <Atom/RHI/ShaderResourceGroupData.h>
#include <AzCore/std/containers/variant.h>

namespace AZ::RHI
{
    class MultiDeviceShaderResourceGroup;
    class MultiDeviceShaderResourceGroupPool;

    //! Shader resource group data is a light abstraction over a flat table of shader resources
    //! and shader constants. It utilizes basic reflection information from the shader resource group layout
    //! to construct the table in the correct format for the platform-specific compile phase. The user
    //! is expected to create instances of this class, fill data, and then push it to an SRG instance.
    //!
    //! The shader resource group (SRG) includes a set of built-in SRG constants in a single internally-managed
    //! constant buffer. This is separate from any custom constant buffers that some SRG layouts may include
    //! as shader resources. SRG constants can be conveniently accessed through a variety of SetConstant.
    //!
    //! This data structure holds strong references to the resource views bound onto it.
    //!
    //! NOTE [Performance Warning]: This data structure allocates memory. If compiling several SRG's in a batch,
    //! prefer to share the data between them (i.e. within a single job).
    //!
    //! NOTE [SRG Constants]: The ConstantsData class is used for efficiently setting/getting the constants values of the SRG.
    class MultiDeviceShaderResourceGroupData
    {
        //! Local abstraction for multi-device views to images and buffers, consisting of an image/buffer and a corresponding descriptor
        using MultiDeviceBufferView = AZStd::pair<const MultiDeviceBuffer*, BufferViewDescriptor>;
        using MultiDeviceImageView = AZStd::pair<const MultiDeviceImage*, ImageViewDescriptor>;

    public:
        //! By default creates an empty data structure. Must be initialized before use.
        MultiDeviceShaderResourceGroupData() = default;
        ~MultiDeviceShaderResourceGroupData() = default;

        //! Creates shader resource group data from a layout.
        explicit MultiDeviceShaderResourceGroupData(
            MultiDevice::DeviceMask deviceMask, const ShaderResourceGroupLayout* shaderResourceGroupLayout);

        //! Creates shader resource group data from a pool (usable on any SRG with the same layout).
        explicit MultiDeviceShaderResourceGroupData(const MultiDeviceShaderResourceGroupPool& shaderResourceGroupPool);

        //! Creates shader resource group data from an SRG instance (usable on any SRG with the same layout).
        explicit MultiDeviceShaderResourceGroupData(const MultiDeviceShaderResourceGroup& shaderResourceGroup);

        AZ_DEFAULT_COPY_MOVE(MultiDeviceShaderResourceGroupData);

        //! Resolves a shader input name to an index using reflection. For performance reasons, this resolve
        //! operation should be performed once at initialization time (or as infrequently as possible).
        //! Assignment of shader inputs is faster when done using the shader input index directly.
        ShaderInputBufferIndex FindShaderInputBufferIndex(const Name& name) const;
        ShaderInputImageIndex FindShaderInputImageIndex(const Name& name) const;
        ShaderInputSamplerIndex FindShaderInputSamplerIndex(const Name& name) const;
        ShaderInputConstantIndex FindShaderInputConstantIndex(const Name& name) const;

        //! Sets one image view for the given shader input index.
        bool SetImageView(ShaderInputImageIndex inputIndex, const MultiDeviceImageView* imageView, uint32_t arrayIndex);

        //! Sets an array of image view for the given shader input index.
        bool SetImageViewArray(
            ShaderInputImageIndex inputIndex, AZStd::span<const MultiDeviceImageView* const> imageViewDescriptors, uint32_t arrayIndex = 0);

        //! Sets an unbounded array of image view for the given shader input index.
        bool SetImageViewUnboundedArray(
            ShaderInputImageUnboundedArrayIndex inputIndex, AZStd::span<const MultiDeviceImageView* const> imageViewDescriptors);

        //! Sets one buffer view for the given shader input index.
        bool SetBufferView(ShaderInputBufferIndex inputIndex, const MultiDeviceBufferView* bufferView, uint32_t arrayIndex = 0);

        //! Sets an array of image view for the given shader input index.
        bool SetBufferViewArray(
            ShaderInputBufferIndex inputIndex, AZStd::span<const MultiDeviceBufferView* const> bufferView, uint32_t arrayIndex = 0);

        //! Sets an unbounded array of buffer view for the given shader input index.
        bool SetBufferViewUnboundedArray(
            ShaderInputBufferUnboundedArrayIndex inputIndex, AZStd::span<const MultiDeviceBufferView* const> bufferView);

        //! Sets one sampler for the given shader input index, using the bindingIndex as the key.
        bool SetSampler(ShaderInputSamplerIndex inputIndex, const SamplerState& sampler, uint32_t arrayIndex = 0);

        //! Sets an array of samplers for the given shader input index.
        bool SetSamplerArray(ShaderInputSamplerIndex inputIndex, AZStd::span<const SamplerState> samplers, uint32_t arrayIndex = 0);

        //! Assigns constant data for the given constant shader input index.
        bool SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount);
        bool SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount);

        //! Assigns a value of type T to the constant shader input.
        template<typename T>
        bool SetConstant(ShaderInputConstantIndex inputIndex, const T& value);

        //! Assigns a specified number of rows from a Matrix
        template<typename T>
        bool SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const T& value, uint32_t rowCount);

        //! Assigns a value of type T to the constant shader input, at an array offset.
        template<typename T>
        bool SetConstant(ShaderInputConstantIndex inputIndex, const T& value, uint32_t arrayIndex);

        //! Assigns an array of type T to the constant shader input.
        template<typename T>
        bool SetConstantArray(ShaderInputConstantIndex inputIndex, AZStd::span<const T> values);

        //! Assigns constant data as a whole.
        //!
        //! CAUTION!
        //! Different platforms might follow different packing rules for the internally-managed SRG constant buffer.
        //! To set manually a constant buffer as a whole please use Constant Buffers in AZSL,
        //! instead of SRG Constants, then use RHI Buffers with constant binding flag and set
        //! the buffer memory following pragma 4 packing rule.
        bool SetConstantData(const void* bytes, uint32_t byteCount);
        bool SetConstantData(const void* bytes, uint32_t byteOffset, uint32_t byteCount);

        //! Returns the device-specific ShaderResourceGroupData for the given index
        const ShaderResourceGroupData& GetDeviceShaderResourceGroupData(int deviceIndex) const
        {
            AZ_Error(
                "MultiDeviceShaderResourceGroupData",
                m_deviceShaderResourceGroupDatas.find(deviceIndex) != m_deviceShaderResourceGroupDatas.end(),
                "No ShaderResourceGroupData found for device index %d\n",
                deviceIndex);

            return m_deviceShaderResourceGroupDatas.at(deviceIndex);
        }

        //! Reset image and buffer views setup for this MultiDeviceShaderResourceGroupData
        //! So it won't hold references for any RHI resources
        void ResetViews();

        //! Returns the shader resource layout for this group.
        const ShaderResourceGroupLayout* GetLayout() const;

        using ResourceType = ShaderResourceGroupData::ResourceType;

        using ResourceTypeMask = ShaderResourceGroupData::ResourceTypeMask;

        //! Reset the update mask
        void ResetUpdateMask();

        //! Enable compilation for a resourceType specified by resourceTypeMask
        void EnableResourceTypeCompilation(ResourceTypeMask resourceTypeMask);

        //! Returns the mask that is suppose to indicate which resource type was updated
        uint32_t GetUpdateMask() const;

        //! Update the indirect buffer view with the indices of all the image views which reside in the global gpu heap.
        void SetBindlessViews(
            ShaderInputBufferIndex indirectResourceBufferIndex,
            const MultiDeviceBufferView* indirectResourceBufferView,
            AZStd::span<const MultiDeviceImageView* const> imageViews,
            uint32_t* outIndices,
            AZStd::span<bool> isViewReadOnly,
            uint32_t arrayIndex = 0);

        //! Update the indirect buffer view with the indices of all the buffer views which reside in the global gpu heap.
        void SetBindlessViews(
            ShaderInputBufferIndex indirectResourceBufferIndex,
            const MultiDeviceBufferView* indirectResourceBufferView,
            AZStd::span<const MultiDeviceBufferView* const> bufferViews,
            uint32_t* outIndices,
            AZStd::span<bool> isViewReadOnly,
            uint32_t arrayIndex = 0);

    private:
        bool ValidateSetImageView(ShaderInputImageIndex inputIndex, const MultiDeviceImageView* imageView, uint32_t arrayIndex) const;
        bool ValidateSetBufferView(ShaderInputBufferIndex inputIndex, const MultiDeviceBufferView* bufferView, uint32_t arrayIndex) const;

        template<typename TShaderInput, typename TShaderInputDescriptor>
        bool ValidateImageViewAccess(TShaderInput inputIndex, const MultiDeviceImageView* imageView, uint32_t arrayIndex) const;
        template<typename TShaderInput, typename TShaderInputDescriptor>
        bool ValidateBufferViewAccess(TShaderInput inputIndex, const MultiDeviceBufferView* bufferView, uint32_t arrayIndex) const;

        //! Device mask denoting on which devices the SRG data is needed
        MultiDevice::DeviceMask m_deviceMask;

        ConstPtr<ShaderResourceGroupLayout> m_shaderResourceGroupLayout;

        //! Mask used to check whether to compile a specific resource type. This mask is managed by RPI and copied over to the RHI every
        //! frame.
        uint32_t m_updateMask = 0;

        //! A map of all device-specific ShaderResourceGroupDatas, indexed by the device index
        AZStd::unordered_map<int, ShaderResourceGroupData> m_deviceShaderResourceGroupDatas;
    };

    template<typename T>
    bool MultiDeviceShaderResourceGroupData::SetConstant(ShaderInputConstantIndex inputIndex, const T& value)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = true;

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstant(inputIndex, value);
        }

        return isValidAll;
    }

    template<typename T>
    bool MultiDeviceShaderResourceGroupData::SetConstant(ShaderInputConstantIndex inputIndex, const T& value, uint32_t arrayIndex)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = true;

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstant(inputIndex, value, arrayIndex);
        }

        return isValidAll;
    }

    template<typename T>
    bool MultiDeviceShaderResourceGroupData::SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const T& value, uint32_t rowCount)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = true;

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstantMatrixRows(inputIndex, value, rowCount);
        }

        return isValidAll;
    }

    template<typename T>
    bool MultiDeviceShaderResourceGroupData::SetConstantArray(ShaderInputConstantIndex inputIndex, AZStd::span<const T> values)
    {
        if (!values.empty())
        {
            EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);
        }

        bool isValidAll = true;

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstantArray(inputIndex, values);
        }

        return isValidAll;
    }

    template<typename TShaderInput, typename TShaderInputDescriptor>
    bool MultiDeviceShaderResourceGroupData::ValidateImageViewAccess(
        TShaderInput inputIndex, const MultiDeviceImageView* imageView, [[maybe_unused]] uint32_t arrayIndex) const
    {
        if (!Validation::IsEnabled())
        {
            return true;
        }

        const TShaderInputDescriptor& shaderInputImage = GetLayout()->GetShaderInput(inputIndex);

        if (!imageView)
        {
            AZ_Error(
                "MultiDeviceShaderResourceGroupData",
                false,
                "Image Array Input '%s[%d]' is null.",
                shaderInputImage.m_name.GetCStr(),
                arrayIndex);
            return false;
        }

        const ImageViewDescriptor& imageViewDescriptor = imageView->second;
        const MultiDeviceImage& image = *(imageView->first);
        const ImageDescriptor& imageDescriptor = image.GetDescriptor();
        const ImageFrameAttachment* frameAttachment = image.GetFrameAttachment();

        // The image must have the correct bind flags for the slot.
        const bool isValidAccess = (shaderInputImage.m_access == ShaderInputImageAccess::Read &&
                                    CheckBitsAll(imageDescriptor.m_bindFlags, ImageBindFlags::ShaderRead)) ||
            (shaderInputImage.m_access == ShaderInputImageAccess::ReadWrite &&
             CheckBitsAll(imageDescriptor.m_bindFlags, ImageBindFlags::ShaderReadWrite));

        if (!isValidAccess)
        {
            AZ_Error(
                "MultiDeviceShaderResourceGroupData",
                false,
                "Image Input '%s[%d]': Invalid 'Read / Write' access. Expected '%s'.",
                shaderInputImage.m_name.GetCStr(),
                arrayIndex,
                GetShaderInputAccessName(shaderInputImage.m_access));
            return false;
        }

        if (shaderInputImage.m_access == ShaderInputImageAccess::ReadWrite)
        {
            // An image view assigned to an input with read-write access must be an attachment on the frame scheduler.
            if (!frameAttachment)
            {
                AZ_Error(
                    "MultiDeviceShaderResourceGroupData",
                    false,
                    "Image Input '%s[%d]': MultiDeviceImage is bound to a ReadWrite shader input, "
                    "but it is not an attachment on the frame scheduler. All GPU-writable resources "
                    "must be declared as attachments in order to provide hazard tracking.",
                    shaderInputImage.m_name.GetCStr(),
                    arrayIndex,
                    GetShaderInputAccessName(shaderInputImage.m_access));
                return false;
            }

            // NOTE: We aren't able to validate the scope attachment here, because shader resource groups aren't directly
            // associated with a scope. Instead, the CommandListValidator class will check that the access is correct at
            // command list submission time.
        }

        auto checkImageType = [&imageDescriptor, &shaderInputImage, arrayIndex](ImageDimension expected)
        {
            if (imageDescriptor.m_dimension != expected)
            {
                AZ_UNUSED(shaderInputImage);
                AZ_UNUSED(arrayIndex);
                AZ_Error(
                    "MultiDeviceShaderResourceGroupData",
                    false,
                    "Image Input '%s[%d]': The image is %dD but the shader expected %dD",
                    shaderInputImage.m_name.GetCStr(),
                    arrayIndex,
                    static_cast<int>(imageDescriptor.m_dimension),
                    static_cast<int>(expected));
                return false;
            }
            return true;
        };

        switch (shaderInputImage.m_type)
        {
        case ShaderInputImageType::Unknown:
            // Unable to validate.
            break;

        case ShaderInputImageType::Image1DArray:
        case ShaderInputImageType::Image1D:
            if (!checkImageType(ImageDimension::Image1D))
            {
                return false;
            }
            break;

        case ShaderInputImageType::SubpassInput:
            if (!checkImageType(ImageDimension::Image2D))
            {
                return false;
            }
            break;

        case ShaderInputImageType::Image2DArray:
        case ShaderInputImageType::Image2D:
            if (!checkImageType(ImageDimension::Image2D))
            {
                return false;
            }
            if (imageDescriptor.m_multisampleState.m_samples != 1)
            {
                AZ_Error(
                    "MultiDeviceShaderResourceGroupData",
                    false,
                    "Image Input '%s[%d]': The image has multisample count %u but the shader expected 1.",
                    shaderInputImage.m_name.GetCStr(),
                    arrayIndex,
                    imageDescriptor.m_multisampleState.m_samples);
                return false;
            }
            break;

        case ShaderInputImageType::Image2DMultisample:
        case ShaderInputImageType::Image2DMultisampleArray:
            if (!checkImageType(ImageDimension::Image2D))
            {
                return false;
            }
            if (imageDescriptor.m_multisampleState.m_samples <= 1)
            {
                AZ_Error(
                    "MultiDeviceShaderResourceGroupData",
                    false,
                    "Image Input '%s[%d]': The image has multisample count %u but the shader expected more than 1.",
                    shaderInputImage.m_name.GetCStr(),
                    arrayIndex,
                    imageDescriptor.m_multisampleState.m_samples);
                return false;
            }
            break;

        case ShaderInputImageType::Image3D:
            if (!checkImageType(ImageDimension::Image3D))
            {
                return false;
            }
            break;

        case ShaderInputImageType::ImageCube:
        case ShaderInputImageType::ImageCubeArray:
            if (!checkImageType(ImageDimension::Image2D))
            {
                return false;
            }
            if (imageViewDescriptor.m_isCubemap == 0)
            {
                AZ_Error(
                    "MultiDeviceShaderResourceGroupData",
                    false,
                    "Image Input '%s[%d]': The shader expected a cubemap.",
                    shaderInputImage.m_name.GetCStr(),
                    arrayIndex);
                return false;
            }
            break;

        default:
            AZ_Assert(false, "Image Input '%s[%d]': Invalid image type!", shaderInputImage.m_name.GetCStr(), arrayIndex);
            return false;
        }

        return true;
    }

    template<typename TShaderInput, typename TShaderInputDescriptor>
    bool MultiDeviceShaderResourceGroupData::ValidateBufferViewAccess(
        TShaderInput inputIndex, const MultiDeviceBufferView* bufferView, [[maybe_unused]] uint32_t arrayIndex) const
    {
        if (!Validation::IsEnabled())
        {
            return true;
        }

        const TShaderInputDescriptor& shaderInputBuffer = GetLayout()->GetShaderInput(inputIndex);
        const BufferViewDescriptor& bufferViewDescriptor = bufferView->second;
        const MultiDeviceBuffer& buffer = *(bufferView->first);
        const BufferDescriptor& bufferDescriptor = buffer.GetDescriptor();
        const BufferFrameAttachment* frameAttachment = buffer.GetFrameAttachment();

        const bool isValidAccess = (shaderInputBuffer.m_access == ShaderInputBufferAccess::Constant &&
                                    CheckBitsAll(bufferDescriptor.m_bindFlags, BufferBindFlags::Constant)) ||
            (shaderInputBuffer.m_access == ShaderInputBufferAccess::Read &&
             CheckBitsAll(bufferDescriptor.m_bindFlags, BufferBindFlags::ShaderRead)) ||
            (shaderInputBuffer.m_access == ShaderInputBufferAccess::Read &&
             CheckBitsAll(bufferDescriptor.m_bindFlags, BufferBindFlags::RayTracingAccelerationStructure)) ||
            (shaderInputBuffer.m_access == ShaderInputBufferAccess::ReadWrite &&
             CheckBitsAll(bufferDescriptor.m_bindFlags, BufferBindFlags::ShaderReadWrite));

        if (!isValidAccess)
        {
            AZ_Error(
                "MultiDeviceShaderResourceGroupData",
                false,
                "Buffer Input '%s[%d]': Invalid 'Constant / Read / Write' access. Expected '%s'",
                shaderInputBuffer.m_name.GetCStr(),
                arrayIndex,
                GetShaderInputAccessName(shaderInputBuffer.m_access));
            return false;
        }

        if (shaderInputBuffer.m_access == ShaderInputBufferAccess::ReadWrite /*&& !bufferView->IgnoreFrameAttachmentValidation()*/)
        {
            // A buffer view assigned to an input with read-write access must be an attachment on the frame scheduler.
            if (!frameAttachment)
            {
                AZ_Error(
                    "MultiDeviceShaderResourceGroupData",
                    false,
                    "Buffer Input '%s[%d]': MultiDeviceBuffer is bound to a ReadWrite shader input, "
                    "but it is not an attachment on the frame scheduler. All GPU-writable resources "
                    "must be declared as attachments in order to provide hazard tracking.",
                    shaderInputBuffer.m_name.GetCStr(),
                    arrayIndex,
                    GetShaderInputAccessName(shaderInputBuffer.m_access));
                return false;
            }

            // NOTE: We aren't able to validate the scope attachment here, because shader resource groups aren't directly
            // associated with a scope. Instead, the CommandListValidator class will check that the access is correct at
            // command list submission time.
        }

        if (shaderInputBuffer.m_type == ShaderInputBufferType::Constant)
        {
            // For Constant type the stride (full constant buffer) must be larger or equal than the buffer view size (element size x
            // element count).
            if (!(shaderInputBuffer.m_strideSize >= (bufferViewDescriptor.m_elementSize * bufferViewDescriptor.m_elementCount)))
            {
                AZ_Error(
                    "MultiDeviceShaderResourceGroupData",
                    false,
                    "Buffer Input '%s[%d]': stride size %d must be larger than or equal to the buffer view size %d",
                    shaderInputBuffer.m_name.GetCStr(),
                    arrayIndex,
                    shaderInputBuffer.m_strideSize,
                    (bufferViewDescriptor.m_elementSize * bufferViewDescriptor.m_elementCount));
                return false;
            }
        }
        else
        {
            // For any other type the buffer view's element size should match the stride.
            if (shaderInputBuffer.m_strideSize != bufferViewDescriptor.m_elementSize)
            {
                AZ_Error(
                    "MultiDeviceShaderResourceGroupData",
                    false,
                    "Buffer Input '%s[%d]': Does not match expected stride size %d",
                    shaderInputBuffer.m_name.GetCStr(),
                    arrayIndex,
                    bufferViewDescriptor.m_elementSize);
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

        case ShaderInputBufferType::AccelerationStructure:
            isValidType &= bufferViewDescriptor.m_elementFormat == Format::R32_UINT;
            break;

        default:
            AZ_Assert(false, "Buffer Input '%s[%d]': Invalid buffer type!", shaderInputBuffer.m_name.GetCStr(), arrayIndex);
            return false;
        }

        if (!isValidType)
        {
            AZ_Error(
                "MultiDeviceShaderResourceGroupData",
                false,
                "Buffer Input '%s[%d]': Does not match expected type '%s'",
                shaderInputBuffer.m_name.GetCStr(),
                arrayIndex,
                GetShaderInputTypeName(shaderInputBuffer.m_type));
            return false;
        }

        return true;
    }
} // namespace AZ::RHI