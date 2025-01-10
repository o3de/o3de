/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>
#include <Atom/RPI.Reflect/Configuration.h>

namespace AZ
{
    namespace RPI
    {
        //! Constructs an instance of a BufferAsset
        class ATOM_RPI_REFLECT_API BufferAssetCreator
            : public AssetCreator<BufferAsset>
        {
        public:
            BufferAssetCreator() = default;
            ~BufferAssetCreator() = default;

            //! @brief Begins construction of a new BufferAsset instance. Resets the builder to a fresh state
            //! @param assetId The unique id to use when creating the asset
            void Begin(const Data::AssetId& assetId);

            //! @brief Assigns data to the BufferAsset
            //! @detail The given buffer is copied to the BufferAsset's structure and
            //! will be cleaned up when the BufferAsset is destroyed.
            //! 
            //! @param[in] initialData The byte buffer to be copied to this stream
            //! @param[in] initialDataSize The number of bytes of the initialData buffer to use.
            //!                            Must be smaller than the size described in descriptor.m_byteCount
            //! @param[in] descriptor The descriptor for the buffer including the total size
            void SetBuffer(const void* initialData, const size_t initialDataSize, const RHI::BufferDescriptor& descriptor);

            //! @brief Assigns a given BufferViewDescriptor to the BufferAsset
            //! @param[in] viewDescriptor The descriptor for the view into the buffer
            void SetBufferViewDescriptor(const RHI::BufferViewDescriptor& viewDescriptor);

            //! Assigns the buffer pool, which the runtime buffer will allocate from.
            //! This pool is used for buffer allocation if it's set.
            //! Otherwise the system pool specified by SetUseCommonPool will be used.
            void SetPoolAsset(const Data::Asset<ResourcePoolAsset>& poolAsset);
            
            //! Set the buffer to use one of common pools provided by buffer system.
            //! The common pool is used if pool asset wasn't set.
            void SetUseCommonPool(CommonBufferPoolType poolType);

            //! Set buffer's name. This name is used for buffer's debug name
            void SetBufferName(AZStd::string_view name);
            
            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<BufferAsset>& result);

            //! Clone the given source buffer asset.
            //! @param sourceAsset The source buffer asset to clone.
            //! @param clonedResult The resulting, cloned buffer asset.
            //! @param inOutLastCreatedAssetId The asset id from the model lod asset that owns the cloned buffer asset. The sub id will be increased and
            //!                                used as the asset id for the cloned asset.
            //! @result True in case the asset got cloned successfully, false in case an error happened and the clone process got cancelled.
            static bool Clone(const Data::Asset<BufferAsset>& sourceAsset, Data::Asset<BufferAsset>& clonedResult, Data::AssetId& inOutLastCreatedAssetId);

        private:
            bool ValidateBuffer();
        };
    } // namespace RPI
} // namespace AZ
