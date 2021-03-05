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

#include <Atom/RPI.Reflect/Base.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroupPool.h>
#include <Atom/RPI.Reflect/Image/Image.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>

#include <Atom/RHI/ShaderResourceGroup.h>

#include <AtomCore/std/containers/array_view.h>
#include <AtomCore/Instance/InstanceData.h>

namespace AZ
{
    namespace RHI
    {
        struct ShaderDataMappings;
    }

    namespace RPI
    {
        /**
         * This class is an RPI extension to the RHI shader resource group class. It provides support
         * for instantiation from an asset, as well as assignment of RPI resource types.
         *
         * This class supports assignment of both RPI and RHI types. If an RPI resource is bound at
         * a specific location, the class will hold *both* the RPI and RHI references. On the other
         * hand, if an RHI resource is bound, any previously held RPI resource is *cleared*. Therefore,
         * it's possible that querying for an RPI resource will return null while querying the same
         * location for an RHI resource will return a valid entry.
         *
         * If RHI validation is enabled, the class will perform error checking. If a setter method fails
         * an error is emitted and the call returns false without performing the requested operation.
         * Likewise, if a getter method fails, an error is emitted and an empty value or empty array
         * is returned. If validation is disabled, the operation is always performed.
         */
        class ShaderResourceGroup final
            : public AZ::Data::InstanceData
        {
            friend class ShaderSystem;
        public:
            AZ_INSTANCE_DATA(ShaderResourceGroup, "{88B52D0C-9CBF-4B4D-B9E2-180BA602E1EA}");
            AZ_CLASS_ALLOCATOR(ShaderResourceGroup, AZ::SystemAllocator, 0);

            /// Instantiates or returns an existing streaming image instance using its paired asset.
            static Data::Instance<ShaderResourceGroup> FindOrCreate(const Data::Asset<ShaderResourceGroupAsset>& srgAsset);

            /// Instantiates a unique shader resource group instance using its paired asset.
            static Data::Instance<ShaderResourceGroup> Create(const Data::Asset<ShaderResourceGroupAsset>& srgAsset);

            /// Queues a request that the underlying hardware shader resource group be compiled.
            void Compile();

            /// Returns whether the group is currently queued for compilation.
            bool IsQueuedForCompile() const;

            /// Finds the shader input index from the shader input name for each type of resource.
            RHI::ShaderInputBufferIndex    FindShaderInputBufferIndex(const Name& name) const;
            RHI::ShaderInputImageIndex     FindShaderInputImageIndex(const Name& name) const;
            RHI::ShaderInputSamplerIndex   FindShaderInputSamplerIndex(const Name& name) const;
            RHI::ShaderInputConstantIndex  FindShaderInputConstantIndex(const Name& name) const;

            /// Returns the parent shader resource group asset.
            const Data::Asset<ShaderResourceGroupAsset>& GetAsset() const;

            /// Returns the RHI shader resource group layout.
            const RHI::ShaderResourceGroupLayout* GetLayout() const;

            /// Returns the underlying RHI shader resource group.
            RHI::ShaderResourceGroup* GetRHIShaderResourceGroup();

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access of RPI Image types.

            /// Sets the ShaderVariantKey value as constant data. Returns false if this SRG is not designated as fallback.
            bool SetShaderVariantKeyFallbackValue(const ShaderVariantKey& shaderKey);

            /// Returns true if the ShaderResourceGroup has been designated as a ShaderVariantKey fallback
            bool HasShaderVariantKeyFallbackEntry() const;

            /// Sets one RPI image for the given shader input index.
            bool SetImage(RHI::ShaderInputImageIndex inputIndex, const Data::Instance<Image>& image);
            bool SetImage(RHI::ShaderInputImageIndex inputIndex, const Data::Instance<Image>& image, uint32_t arrayIndex);

            /// Sets multiple RPI images for the given shader input index.
            bool SetImageArray(RHI::ShaderInputImageIndex inputIndex, AZStd::array_view<Data::Instance<Image>> images);
            bool SetImageArray(RHI::ShaderInputImageIndex inputIndex, AZStd::array_view<Data::Instance<Image>> images, uint32_t arrayIndex);

            /// Returns a single RPI image associated with the image shader input index and array offset.
            const Data::Instance<Image>& GetImage(RHI::ShaderInputImageIndex inputIndex) const;
            const Data::Instance<Image>& GetImage(RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const;

            /// Returns an array of RPI images associated with the image shader input index.
            AZStd::array_view<Data::Instance<Image>> GetImageArray(RHI::ShaderInputImageIndex inputIndex) const;

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access of RPI Buffer types.

            /// Sets one RPI buffer for the given shader input index.
            bool SetBuffer(RHI::ShaderInputBufferIndex inputIndex, const Data::Instance<Buffer>& buffer);
            bool SetBuffer(RHI::ShaderInputBufferIndex inputIndex, const Data::Instance<Buffer>& buffer, uint32_t arrayIndex);

            /// Sets multiple RPI buffers for the given shader input index.
            bool SetBufferArray(RHI::ShaderInputBufferIndex inputIndex, AZStd::array_view<Data::Instance<Buffer>> buffers);
            bool SetBufferArray(RHI::ShaderInputBufferIndex inputIndex, AZStd::array_view<Data::Instance<Buffer>> buffers, uint32_t arrayIndex);

            /// Returns a single RPI buffer associated with the buffer shader input index and array offset.
            const Data::Instance<Buffer>& GetBuffer(RHI::ShaderInputBufferIndex inputIndex) const;
            const Data::Instance<Buffer>& GetBuffer(RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const;

            /// Returns an array of RPI buffers associated with the buffer shader input index.
            AZStd::array_view<Data::Instance<Buffer>> GetBufferArray(RHI::ShaderInputBufferIndex inputIndex) const;

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access of RHI Image types.

            /// Sets one image view for the given shader input index.
            bool SetImageView(RHI::ShaderInputImageIndex inputIndex, const RHI::ImageView* imageView);
            bool SetImageView(RHI::ShaderInputImageIndex inputIndex, const RHI::ImageView* imageView, uint32_t arrayIndex);

            /// Sets an array of image view for the given shader input index.
            bool SetImageViewArray(RHI::ShaderInputImageIndex inputIndex, AZStd::array_view<const RHI::ImageView*> imageViews);
            bool SetImageViewArray(RHI::ShaderInputImageIndex inputIndex, AZStd::array_view<const RHI::ImageView*> imageViews, uint32_t arrayIndex);

            /// Returns a single image view associated with the image shader input index and array offset.
            const RHI::ConstPtr<RHI::ImageView>& GetImageView(RHI::ShaderInputImageIndex inputIndex) const;
            const RHI::ConstPtr<RHI::ImageView>& GetImageView(RHI::ShaderInputImageIndex inputIndex, uint32_t arrayIndex) const;

            /// Returns an array of image views associated with the given image shader input index.
            AZStd::array_view<RHI::ConstPtr<RHI::ImageView>> GetImageViewArray(RHI::ShaderInputImageIndex inputIndex) const;

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access of RHI Buffer types.

            /// Sets one buffer view for the given shader input index.
            bool SetBufferView(RHI::ShaderInputBufferIndex inputIndex, const RHI::BufferView* bufferView);
            bool SetBufferView(RHI::ShaderInputBufferIndex inputIndex, const RHI::BufferView* bufferView, uint32_t arrayIndex);

            /// Sets an array of buffer view for the given shader input index.
            bool SetBufferViewArray(RHI::ShaderInputBufferIndex inputIndex, AZStd::array_view<const RHI::BufferView*> bufferViews);
            bool SetBufferViewArray(RHI::ShaderInputBufferIndex inputIndex, AZStd::array_view<const RHI::BufferView*> bufferViews, uint32_t arrayIndex);

            /// Returns a single buffer view associated with the buffer shader input index and array offset.
            const RHI::ConstPtr<RHI::BufferView>& GetBufferView(RHI::ShaderInputBufferIndex inputIndex) const;
            const RHI::ConstPtr<RHI::BufferView>& GetBufferView(RHI::ShaderInputBufferIndex inputIndex, uint32_t arrayIndex) const;

            /// Returns an array of buffer views associated with the given buffer shader input index.
            AZStd::array_view<RHI::ConstPtr<RHI::BufferView>> GetBufferViewArray(RHI::ShaderInputBufferIndex inputIndex) const;

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access of RHI Sampler types.

            /// Sets one sampler for the given shader input index, using the bindingIndex as the key.
            bool SetSampler(RHI::ShaderInputSamplerIndex inputIndex, const RHI::SamplerState& sampler);
            bool SetSampler(RHI::ShaderInputSamplerIndex inputIndex, const RHI::SamplerState& sampler, uint32_t arrayIndex);

            /// Sets an array of samplers for the given shader input index.
            bool SetSamplerArray(RHI::ShaderInputSamplerIndex inputIndex, AZStd::array_view<RHI::SamplerState> samplers);
            bool SetSamplerArray(RHI::ShaderInputSamplerIndex inputIndex, AZStd::array_view<RHI::SamplerState> samplers, uint32_t arrayIndex);

            /// Returns a single sampler associated with the sampler shader input index and array offset.
            const RHI::SamplerState& GetSampler(RHI::ShaderInputSamplerIndex inputIndex, uint32_t arrayIndex) const;

            /// Returns an array of samplers associated with the sampler shader input index.
            AZStd::array_view<RHI::SamplerState> GetSamplerArray(RHI::ShaderInputSamplerIndex inputIndex) const;

            //////////////////////////////////////////////////////////////////////////
            // Methods for assignment / access SRG constants.

            /// Assigns constant data for the given constant shader input index.
            bool SetConstantRaw(RHI::ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteCount);
            bool SetConstantRaw(RHI::ShaderInputConstantIndex inputIndex, const void* bytes, uint32_t byteOffset, uint32_t byteCount);

            /// Assigns a value of type T to the constant shader input.
            template <typename T>
            bool SetConstant(RHI::ShaderInputConstantIndex inputIndex, const T& value);

            /// Assigns the specified number of rows from a Matrix
            template <typename T>
            bool SetConstantMatrixRows(RHI::ShaderInputConstantIndex inputIndex, const T& value, uint32_t rowCount);

            /// Assigns a value of type T to the constant shader input, at an array offset.
            template <typename T>
            bool SetConstant(RHI::ShaderInputConstantIndex inputIndex, const T& value, uint32_t arrayIndex);

            /// Assigns an array of type T to the constant shader input.
            template <typename T>
            bool SetConstantArray(RHI::ShaderInputConstantIndex inputIndex, AZStd::array_view<T> values);

            /// Assigns an array of type T to the constant shader input.
            template <typename T, size_t N>
            bool SetConstantArray(RHI::ShaderInputConstantIndex inputIndex, const AZStd::array<T, N>& values);

            /// Assigns an array of shader data mappings of type T
            template<typename T>
            bool ApplyDataMappingArray(const AZStd::vector<T>& mappings);

            /// Assigns a group of shader data mapping arrays
            bool ApplyDataMappings(const RHI::ShaderDataMappings& mappings);

            /**
             * Returns constant data for the given shader input index as a template type.
             * The stride of T must match the size of the constant input region. The number
             * of elements in the returned array is the number of evenly divisible elements.
             * If the strides do not match, an empty array is returned.
             */
            template <typename T>
            AZStd::array_view<T> GetConstantArray(RHI::ShaderInputConstantIndex inputIndex) const;

            /**
             * Returns the constant data as type 'T' returned by value. The size of the constant region
             * must match the size of T exactly. Otherwise, an empty instance is returned.
             */
            template <typename T>
            T GetConstant(RHI::ShaderInputConstantIndex inputIndex) const;

            /**
             * Treats the constant input as an array of type T, returning the element by value at the
             * specified array index. The size of the constant region must equally partition into an
             * array of type T. Otherwise, an empty instance is returned.
             */
            template <typename T>
            T GetConstant(RHI::ShaderInputConstantIndex inputIndex, uint32_t arrayIndex) const;

            /// Returns constant data for the given shader input index as an array of bytes.
            AZStd::array_view<uint8_t> GetConstantRaw(RHI::ShaderInputConstantIndex inputIndex) const;

        private:
            ShaderResourceGroup() = default;

            RHI::ResultCode Init(ShaderResourceGroupAsset& shaderResourceGroupAsset);
            static AZ::Data::Instance<ShaderResourceGroup> CreateInternal(ShaderResourceGroupAsset& srgAsset);

            /// A name to be used in error messages
            static const char* s_traceCategoryName;

            /// Allows us to return const& to a null Image
            static const Data::Instance<Image> s_nullImage;

            /// Allows us to return const& to a null Buffer
            static const Data::Instance<Buffer> s_nullBuffer;

            /// Pool for allocating RHI::ShaderResourceGroup objects
            Data::Instance<ShaderResourceGroupPool> m_pool;

            /// The shader resource group data that is manipulated by this class
            RHI::ShaderResourceGroupData m_data;

            /// The shader resource group that can be submitted to the renderer
            RHI::Ptr<RHI::ShaderResourceGroup> m_shaderResourceGroup;

            /// A reference to the parent template asset used to initialize and manipulate this group.
            AZ::Data::Asset<ShaderResourceGroupAsset> m_asset;

            /// A pointer to the layout inside of m_srgAsset
            const RHI::ShaderResourceGroupLayout* m_layout = nullptr;

            /**
             * The set of images currently bound. The shader resource group maintains these references to
             * keep the hardware resource in memory, manage streaming operations, and support reload operations.
             * However, entries remain null when RHI image views are bound.
             */
            AZStd::vector<Data::Instance<Image>> m_imageGroup;

            /**
             * The set of buffers currently bound. The shader resource group maintains these references to
             * keep the hardware resource in memory, manage streaming operations, and support reload operations.
             * However, entries remain null when RHI buffer views are bound.
             */
            AZStd::vector<Data::Instance<Buffer>> m_bufferGroup;
        };

        template<typename T>
        bool ShaderResourceGroup::ApplyDataMappingArray(const AZStd::vector<T>& mappings)
        {
            bool success = true;
            for (const auto& mapping : mappings)
            {
                auto index = m_layout->FindShaderInputConstantIndex(mapping.m_name);
                success = success && SetConstant(index, mapping.m_value);
            }
            return success;
        }

        template <typename T>
        bool ShaderResourceGroup::SetConstant(RHI::ShaderInputConstantIndex inputIndex, const T& value)
        {
            return m_data.SetConstant(inputIndex, value);
        }
      
        template <typename T>
        bool ShaderResourceGroup::SetConstantMatrixRows(RHI::ShaderInputConstantIndex inputIndex, const T& value, uint32_t rowCount)
        {
            return m_data.SetConstantMatrixRows(inputIndex, value, rowCount);
        }

        template <typename T>
        bool ShaderResourceGroup::SetConstant(RHI::ShaderInputConstantIndex inputIndex, const T& value, uint32_t arrayIndex)
        {
            return m_data.SetConstant(inputIndex, value, arrayIndex);
        }

        template <typename T>
        bool ShaderResourceGroup::SetConstantArray(RHI::ShaderInputConstantIndex inputIndex, AZStd::array_view<T> values)
        {
            return m_data.SetConstantArray(inputIndex, values);
        }

        template <typename T, size_t N>
        bool ShaderResourceGroup::SetConstantArray(RHI::ShaderInputConstantIndex inputIndex, const AZStd::array<T, N>& values)
        {
            return SetConstantArray(inputIndex, AZStd::array_view<T>(values));
        }

        template <typename T>
        AZStd::array_view<T> ShaderResourceGroup::GetConstantArray(RHI::ShaderInputConstantIndex inputIndex) const
        {
            return m_data.GetConstantArray<T>(inputIndex);
        }

        template <typename T>
        T ShaderResourceGroup::GetConstant(RHI::ShaderInputConstantIndex inputIndex) const
        {
            return m_data.GetConstant<T>(inputIndex);
        }
    } // namespace RPI
} // namespace AZ
