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
#include <Atom/RHI/SingleDeviceShaderResourceGroupData.h>
#include <AzCore/std/containers/variant.h>

namespace AZ::RHI
{
    class MultiDeviceShaderResourceGroup;
    class MultiDeviceShaderResourceGroupPool;

    //! MultiShaderResourceGroupData is a multi-device class holding single-device SingleDeviceShaderResourceGroupData objects,
    //! one for each device referenced in its deviceMask.
    //! All calls and data are forwarded to the single-device variants, while the multi-device data is also kept locally,
    //! including ConstantsData and Samplers.
    //!
    //! This data structure holds strong references to the multi-device resource views bound onto it.
    class MultiDeviceShaderResourceGroupData
    {
    public:
        //! By default creates an empty data structure. Must be initialized before use.
        MultiDeviceShaderResourceGroupData() = default;
        ~MultiDeviceShaderResourceGroupData() = default;

        //! Creates MultiDeviceShaderResourceGroupData from a layout and initializes single-device SingleDeviceShaderResourceGroupData.
        explicit MultiDeviceShaderResourceGroupData(
            MultiDevice::DeviceMask deviceMask, const ShaderResourceGroupLayout* shaderResourceGroupLayout);

        //! Creates MultiDeviceShaderResourceGroupData from a pool (usable on any SRG with the same layout).
        explicit MultiDeviceShaderResourceGroupData(const MultiDeviceShaderResourceGroupPool& shaderResourceGroupPool);

        //! Creates MultiDeviceShaderResourceGroupData from an SRG instance (usable on any SRG with the same layout).
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
        bool SetImageView(ShaderInputImageIndex inputIndex, const MultiDeviceImageView* imageView, uint32_t arrayIndex = 0);

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

        //! Returns a single image view associated with the image shader input index and array offset.
        const ConstPtr<MultiDeviceImageView>& GetImageView(ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const;

        //! Returns a span of image views associated with the given image shader input index.
        AZStd::span<const ConstPtr<MultiDeviceImageView>> GetImageViewArray(ShaderInputImageIndex inputIndex) const;

        //! Returns an unbounded span of image views associated with the given buffer shader input index.
        AZStd::span<const ConstPtr<MultiDeviceImageView>> GetImageViewUnboundedArray(ShaderInputImageUnboundedArrayIndex inputIndex) const;

        //! Returns a single buffer view associated with the buffer shader input index and array offset.
        const ConstPtr<MultiDeviceBufferView>& GetBufferView(ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const;

        //! Returns a span of buffer views associated with the given buffer shader input index.
        AZStd::span<const ConstPtr<MultiDeviceBufferView>> GetBufferViewArray(ShaderInputBufferIndex inputIndex) const;

        //! Returns an unbounded span of buffer views associated with the given buffer shader input index.
        AZStd::span<const ConstPtr<MultiDeviceBufferView>> GetBufferViewUnboundedArray(ShaderInputBufferUnboundedArrayIndex inputIndex) const;

        //! Returns a single sampler associated with the sampler shader input index and array offset.
        const SamplerState& GetSampler(ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const;

        //! Returns a span of samplers associated with the sampler shader input index.
        AZStd::span<const SamplerState> GetSamplerArray(ShaderInputSamplerIndex inputIndex) const;

        //! Returns constant data for the given shader input index as a template type.
        //! The stride of T must match the size of the constant input region. The number
        //! of elements in the returned span is the number of evenly divisible elements.
        //! If the strides do not match, an empty span is returned.
        template <typename T>
        AZStd::span<const T> GetConstantArray(ShaderInputConstantIndex inputIndex) const;

        //! Returns the constant data as type 'T' returned by value. The size of the constant region
        //! must match the size of T exactly. Otherwise, an empty instance is returned.
        template <typename T>
        T GetConstant(ShaderInputConstantIndex inputIndex) const;

        //! Treats the constant input as an array of type T, returning the element by value at the
        //! specified array index. The size of the constant region must equally partition into an
        //! array of type T. Otherwise, an empty instance is returned.
        template <typename T>
        T GetConstant(ShaderInputConstantIndex inputIndex, uint32_t arrayIndex) const;

        //! Returns constant data for the given shader input index as a span of bytes.
        AZStd::span<const uint8_t> GetConstantRaw(ShaderInputConstantIndex inputIndex) const;

        //! Returns a {Buffer, Image, Sampler} shader resource group. Each resource type has its own separate group.
        //!  - The size of this group matches the size provided by ShaderResourceGroupLayout::GetGroupSizeFor{Buffer, Image, Sampler}.
        //!  - Use ShaderResourceGroupLayout::GetGroupInterval to retrieve a [min, max) interval into the span.
        AZStd::span<const ConstPtr<MultiDeviceImageView>> GetImageGroup() const;
        AZStd::span<const ConstPtr<MultiDeviceBufferView>> GetBufferGroup() const;
        AZStd::span<const SamplerState> GetSamplerGroup() const;

        //! Returns the device-specific SingleDeviceShaderResourceGroupData for the given index
        const SingleDeviceShaderResourceGroupData& GetDeviceShaderResourceGroupData(int deviceIndex) const
        {
            AZ_Error(
                "MultiDeviceShaderResourceGroupData",
                m_deviceShaderResourceGroupDatas.find(deviceIndex) != m_deviceShaderResourceGroupDatas.end(),
                "No SingleDeviceShaderResourceGroupData found for device index %d\n",
                deviceIndex);

            return m_deviceShaderResourceGroupDatas.at(deviceIndex);
        }

        //! Reset image and buffer views setup for this MultiDeviceShaderResourceGroupData
        //! So it won't hold references for any RHI resources
        void ResetViews();

        //! Returns the shader resource layout for this group.
        const ShaderResourceGroupLayout* GetLayout() const;

        using ResourceType = SingleDeviceShaderResourceGroupData::ResourceType;

        using ResourceTypeMask = SingleDeviceShaderResourceGroupData::ResourceTypeMask;

        // Structure to hold all the bindless views and the BindlessResourceType related to it
        struct MultiDeviceBindlessResourceViews
        {
            BindlessResourceType m_bindlessResourceType = AZ::RHI::BindlessResourceType::Count;
            AZStd::vector<ConstPtr<MultiDeviceResourceView>> m_bindlessResources;
        };

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

        //! Get the size of the bindless view map
        const uint32_t GetBindlessViewsSize() const;

        //! Return all the bindless views referenced indirectly  via SetBindlessViews api
        const AZStd::unordered_map<AZStd::pair<ShaderInputBufferIndex, uint32_t>, MultiDeviceBindlessResourceViews>& GetBindlessResourceViews() const;

    private:
        static const ConstPtr<MultiDeviceImageView> s_nullImageView;
        static const ConstPtr<MultiDeviceBufferView> s_nullBufferView;
        static const SamplerState s_nullSamplerState;

        //! Device mask denoting on which devices the SRG data is needed
        MultiDevice::DeviceMask m_deviceMask;

        ConstPtr<ShaderResourceGroupLayout> m_shaderResourceGroupLayout;

        //! The backing data store of bound resources for the shader resource group.
        AZStd::vector<ConstPtr<MultiDeviceImageView>> m_imageViews;
        AZStd::vector<ConstPtr<MultiDeviceBufferView>> m_bufferViews;
        AZStd::vector<SamplerState> m_samplers;
        AZStd::vector<ConstPtr<MultiDeviceImageView>> m_imageViewsUnboundedArray;
        AZStd::vector<ConstPtr<MultiDeviceBufferView>> m_bufferViewsUnboundedArray;

        // The map below is used to manage ownership of buffer and image views that aren't bound directly to the shader, but implicitly
        // referenced through indirection constants. The key corresponds to the pair of (buffer input slot, index) where the indirection
        // constants reside (an array of indirection buffers is supported)
        AZStd::unordered_map<AZStd::pair<ShaderInputBufferIndex, uint32_t>, MultiDeviceBindlessResourceViews> m_bindlessResourceViews;

        //! The backing data store of constants used only for the getters, actual storage happens in the single device SRGs.
        ConstantsData m_constantsData;

        //! Mask used to check whether to compile a specific resource type. This mask is managed by RPI and copied over to the RHI every
        //! frame.
        uint32_t m_updateMask = 0;

        //! A map of all device-specific ShaderResourceGroupDatas, indexed by the device index
        AZStd::unordered_map<int, SingleDeviceShaderResourceGroupData> m_deviceShaderResourceGroupDatas;
    };

    template<typename T>
    bool MultiDeviceShaderResourceGroupData::SetConstant(ShaderInputConstantIndex inputIndex, const T& value)
    {
        EnableResourceTypeCompilation(ResourceTypeMask::ConstantDataMask);

        bool isValidAll = m_constantsData.SetConstant(inputIndex, value);

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

        bool isValidAll = m_constantsData.SetConstant(inputIndex, value, arrayIndex);

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

        bool isValidAll = m_constantsData.SetConstantMatrixRows(inputIndex, value, rowCount);

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

        bool isValidAll = m_constantsData.SetConstantArray(inputIndex, values);

        for (auto& [deviceIndex, deviceShaderResourceGroupData] : m_deviceShaderResourceGroupDatas)
        {
            isValidAll &= deviceShaderResourceGroupData.SetConstantArray(inputIndex, values);
        }

        return isValidAll;
    }

    template <typename T>
    AZStd::span<const T> MultiDeviceShaderResourceGroupData::GetConstantArray(ShaderInputConstantIndex inputIndex) const
    {
        return m_constantsData.GetConstantArray<T>(inputIndex);
    }

    template <typename T>
    T MultiDeviceShaderResourceGroupData::GetConstant(ShaderInputConstantIndex inputIndex) const
    {
        return m_constantsData.GetConstant<T>(inputIndex);
    }

    template <typename T>
    T MultiDeviceShaderResourceGroupData::GetConstant(ShaderInputConstantIndex inputIndex, uint32_t arrayIndex) const
    {
        return m_constantsData.GetConstant<T>(inputIndex, arrayIndex);
    }
} // namespace AZ::RHI
