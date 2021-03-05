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
#pragma once

#include <Atom/RHI/ConstantsData.h>

namespace AZ
{
    namespace RHI
    {
        class BufferView;
        class ImageView;
        class ShaderResourceGroup;
        class ShaderResourceGroupPool;

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
        class ShaderResourceGroupData
        {
        public:
            //! By default creates an empty data structure. Must be initialized before use.
            ShaderResourceGroupData();
            ~ShaderResourceGroupData();

            //! Creates shader resource group data from a layout.
            explicit ShaderResourceGroupData(const ShaderResourceGroupLayout* shaderResourceGroupLayout);

            //! Creates shader resource group data from a pool (usable on any SRG with the same layout).
            explicit ShaderResourceGroupData(const ShaderResourceGroupPool& shaderResourceGroupPool);

            //! Creates shader resource group data from an SRG instance (usable on any SRG with the same layout).
            explicit ShaderResourceGroupData(const ShaderResourceGroup& shaderResourceGroup);

            AZ_DEFAULT_COPY_MOVE(ShaderResourceGroupData);

            //! Resolves a shader input name to an index using reflection. For performance reasons, this resolve
            //! operation should be performed once at initialization time (or as infrequently as possible).
            //! Assignment of shader inputs is faster when done using the shader input index directly.
            ShaderInputBufferIndex   FindShaderInputBufferIndex(const Name& name) const;
            ShaderInputImageIndex    FindShaderInputImageIndex(const Name& name) const;
            ShaderInputSamplerIndex  FindShaderInputSamplerIndex(const Name& name) const;
            ShaderInputConstantIndex FindShaderInputConstantIndex(const Name& name) const;

            //! Sets one image view for the given shader input index.
            bool SetImageView(ShaderInputImageIndex inputIndex, const ImageView* imageView);
            bool SetImageView(ShaderInputImageIndex inputIndex, const ImageView* imageView, uint32_t arrayIndex);

            //! Sets an array of image view for the given shader input index.
            bool SetImageViewArray(ShaderInputImageIndex inputIndex, AZStd::array_view<const ImageView*> imageViews);
            bool SetImageViewArray(ShaderInputImageIndex inputIndex, AZStd::array_view<const ImageView*> imageViews, uint32_t arrayIndex);

            //! Sets one buffer view for the given shader input index.
            bool SetBufferView(ShaderInputBufferIndex inputIndex, const BufferView* bufferView);
            bool SetBufferView(ShaderInputBufferIndex inputIndex, const BufferView* bufferView, uint32_t arrayIndex);

            //! Sets an array of image view for the given shader input index.
            bool SetBufferViewArray(ShaderInputBufferIndex inputIndex, AZStd::array_view<const BufferView*> bufferViews);
            bool SetBufferViewArray(ShaderInputBufferIndex inputIndex, AZStd::array_view<const BufferView*> bufferViews, uint32_t arrayIndex);

            //! Sets one sampler for the given shader input index, using the bindingIndex as the key.
            bool SetSampler(ShaderInputSamplerIndex inputIndex, const SamplerState& sampler);
            bool SetSampler(ShaderInputSamplerIndex inputIndex, const SamplerState& sampler, uint32_t arrayIndex);

            //! Sets an array of samplers for the given shader input index.
            bool SetSamplerArray(ShaderInputSamplerIndex inputIndex, AZStd::array_view<SamplerState> samplers);
            bool SetSamplerArray(ShaderInputSamplerIndex inputIndex, AZStd::array_view<SamplerState> samplers, uint32_t arrayIndex);

            //! Assigns constant data for the given constant shader input index.
            bool SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount);
            bool SetConstantRaw(ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount);

            //! Assigns a value of type T to the constant shader input.
            template <typename T>
            bool SetConstant(ShaderInputConstantIndex inputIndex, const T& value);

            //! Assigns a specified number of rows from a Matrix
            template <typename T>
            bool SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const T& value, uint32_t rowCount);
                        
            //! Assigns a value of type T to the constant shader input, at an array offset.
            template <typename T>
            bool SetConstant(ShaderInputConstantIndex inputIndex, const T& value, uint32_t arrayIndex);

            //! Assigns an array of type T to the constant shader input.
            template <typename T>
            bool SetConstantArray(ShaderInputConstantIndex inputIndex, AZStd::array_view<T> values);

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
            const ConstPtr<ImageView>& GetImageView(ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const;

            //! Returns an array of image views associated with the given image shader input index.
            AZStd::array_view<ConstPtr<ImageView>> GetImageViewArray(ShaderInputImageIndex inputIndex) const;

            //! Returns a single buffer view associated with the buffer shader input index and array offset.
            const ConstPtr<BufferView>& GetBufferView(ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const;

            //! Returns an array of buffer views associated with the given buffer shader input index.
            AZStd::array_view<ConstPtr<BufferView>> GetBufferViewArray(ShaderInputBufferIndex inputIndex) const;

            //! Returns a single sampler associated with the sampler shader input index and array offset.
            const SamplerState& GetSampler(ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const;

            //! Returns an array of samplers associated with the sampler shader input index.
            AZStd::array_view<SamplerState> GetSamplerArray(ShaderInputSamplerIndex inputIndex) const;

            //! Returns constant data for the given shader input index as a template type.
            //! The stride of T must match the size of the constant input region. The number
            //! of elements in the returned array is the number of evenly divisible elements.
            //! If the strides do not match, an empty array is returned.
            template <typename T>
            AZStd::array_view<T> GetConstantArray(ShaderInputConstantIndex inputIndex) const;

            //! Returns the constant data as type 'T' returned by value. The size of the constant region
            //! must match the size of T exactly. Otherwise, an empty instance is returned.
            template <typename T>
            T GetConstant(ShaderInputConstantIndex inputIndex) const;

            //! Treats the constant input as an array of type T, returning the element by value at the
            //! specified array index. The size of the constant region must equally partition into an
            //! array of type T. Otherwise, an empty instance is returned.
            template <typename T>
            T GetConstant(ShaderInputConstantIndex inputIndex, uint32_t arrayIndex) const;

            //! Returns constant data for the given shader input index as an array of bytes.
            AZStd::array_view<uint8_t> GetConstantRaw(ShaderInputConstantIndex inputIndex) const;

            //! Returns a {Buffer, Image, Sampler} shader resource group. Each resource type has its own separate group.
            //!  - The size of this group matches the size provided by ShaderResourceGroupLayout::GetGroupSizeFor{Buffer, Image, Sampler}.
            //!  - Use ShaderResourceGroupLayout::GetGroupInterval to retrieve a [min, max) interval into the array.
            AZStd::array_view<ConstPtr<ImageView>> GetImageGroup() const;
            AZStd::array_view<ConstPtr<BufferView>> GetBufferGroup() const;
            AZStd::array_view<SamplerState> GetSamplerGroup() const;

            //! Returns the opaque constant data populated by calls to SetConstant and SetConstantData.
            //! 
            //! CAUTION!
            //! Different platforms might follow different packing rules for the internally-managed SRG constant buffer.
            AZStd::array_view<uint8_t> GetConstantData() const;

            //! Returns the shader resource layout for this group.
            const ShaderResourceGroupLayout* GetLayout() const;

        private:
            static const ConstPtr<ImageView> s_nullImageView;
            static const ConstPtr<BufferView> s_nullBufferView;
            static const SamplerState s_nullSamplerState;

            bool ValidateSetImageView(ShaderInputImageIndex inputIndex, const ImageView* imageView, uint32_t arrayIndex) const;
            bool ValidateSetBufferView(ShaderInputBufferIndex inputIndex, const BufferView* bufferView, uint32_t arrayIndex) const;

            ConstPtr<ShaderResourceGroupLayout> m_shaderResourceGroupLayout;

            //! The backing data store of bound resources for the shader resource group.
            AZStd::vector<ConstPtr<ImageView>> m_imageViews;
            AZStd::vector<ConstPtr<BufferView>> m_bufferViews;
            AZStd::vector<SamplerState> m_samplers;

            //! The backing data store of constants for the shader resource group.
            ConstantsData m_constantsData;
        };

        template <typename T>
        bool ShaderResourceGroupData::SetConstant(ShaderInputConstantIndex inputIndex, const T& value)
        {
            return m_constantsData.SetConstant(inputIndex, value);
        }

        template <typename T>
        bool ShaderResourceGroupData::SetConstant(ShaderInputConstantIndex inputIndex, const T& value, uint32_t arrayIndex)
        {
            return m_constantsData.SetConstant(inputIndex, value, arrayIndex);
        }

        template <typename T>
        bool ShaderResourceGroupData::SetConstantArray(ShaderInputConstantIndex inputIndex, AZStd::array_view<T> values)
        {
            return m_constantsData.SetConstantArray(inputIndex, values);
        }

        template <typename T>
        AZStd::array_view<T> ShaderResourceGroupData::GetConstantArray(ShaderInputConstantIndex inputIndex) const
        {
            return m_constantsData.GetConstantArray<T>(inputIndex);
        }

        template <typename T>
        T ShaderResourceGroupData::GetConstant(ShaderInputConstantIndex inputIndex) const
        {
            return m_constantsData.GetConstant<T>(inputIndex);
        }

        template <typename T>
        T ShaderResourceGroupData::GetConstant(ShaderInputConstantIndex inputIndex, uint32_t arrayIndex) const
        {
            return m_constantsData.GetConstant<T>(inputIndex, arrayIndex);
        }

        template <typename T>
        bool ShaderResourceGroupData::SetConstantMatrixRows(ShaderInputConstantIndex inputIndex, const T& value, uint32_t rowCount)
        {
            return m_constantsData.SetConstantMatrixRows(inputIndex, value, rowCount);
        }

    } // namespace RHI
} // namespace AZ
