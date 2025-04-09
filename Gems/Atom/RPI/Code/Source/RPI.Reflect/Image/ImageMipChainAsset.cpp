/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>
#include <Atom/RPI.Reflect/Allocators.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {

        AZ_CLASS_ALLOCATOR_IMPL(ImageMipChainAsset, ImageMipChainAssetAllocator)

        static bool ConvertOldVersions(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 1)
            {
                // We need to convert the vectors because we added a custom allocator
                // and the serialization system doens't automatically convert between two different classes
                {
                    auto crc32 = AZ::Crc32("m_imageData");
                    auto* vectorElement = classElement.FindSubElement(crc32);
                    if (vectorElement)
                    {
                        // Get the old data
                        AZStd::vector<uint8_t> oldData;
                        if (vectorElement->GetData(oldData))
                        {
                            // Convert the vector with the new allocator
                            vectorElement->Convert(context, AZ::AzTypeInfo<AZStd::vector<uint8_t, ImageMipChainAsset::Allocator>>::Uuid());
                            // Copy old data to new data
                            AZStd::vector<uint8_t, ImageMipChainAsset::Allocator> newData(oldData.size());
                            ::memcpy(newData.data(), oldData.data(), newData.size());
                            // Set the new data
                            vectorElement->SetData(context, newData);
                        }
                    }
                }
                {
                    auto crc32 = AZ::Crc32("m_subImageDataOffsets");
                    auto* vectorElement = classElement.FindSubElement(crc32);
                    if (vectorElement)
                    {
                        AZStd::vector<AZ::u64> oldData;
                        if (classElement.GetChildData(crc32, oldData))
                        {
                            // Convert the vector with the new allocator
                            vectorElement->Convert(context, AZ::AzTypeInfo<AZStd::vector<AZ::u64, ImageMipChainAsset::Allocator>>::Uuid());
                            for (const auto& element : oldData)
                            {
                                // Convert each vector and re add it. During convertion all sub elements were removed.
                                vectorElement->AddElementWithData<AZ::u64>(context, "element", element);
                            }
                        }
                    }
                }
            }
            return true;
        }

        void ImageMipChainAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                // Need to register the old type with the Serializer so we can read it in order to convert it
                serializeContext->RegisterGenericType<AZStd::vector<uint8_t>>();
                serializeContext->RegisterGenericType<AZStd::vector<u64>>();

                serializeContext->Class<ImageMipChainAsset, Data::AssetData>()
                    ->Version(1, &ConvertOldVersions)
                    ->Field("m_mipLevels", &ImageMipChainAsset::m_mipLevels)
                    ->Field("m_arraySize", &ImageMipChainAsset::m_arraySize)
                    ->Field("m_mipToSubImageOffset", &ImageMipChainAsset::m_mipToSubImageOffset)
                    ->Field("m_subImageLayouts", &ImageMipChainAsset::m_subImageLayouts)
                    ->Field("m_subImageDataOffsets", &ImageMipChainAsset::m_subImageDataOffsets)
                    ->Field("m_imageData", &ImageMipChainAsset::m_imageData)
                    ;
            }
        }

        uint16_t ImageMipChainAsset::GetMipLevelCount() const
        {
            return m_mipLevels;
        }

        uint16_t ImageMipChainAsset::GetArraySize() const
        {
            return m_arraySize;
        }

        size_t ImageMipChainAsset::GetSubImageCount() const
        {
            return m_subImageDatas.size();
        }

        AZStd::span<const uint8_t> ImageMipChainAsset::GetSubImageData(uint32_t mipSlice, uint32_t arraySlice) const
        {
            return GetSubImageData(mipSlice * m_arraySize + arraySlice);
        }

        AZStd::span<const uint8_t> ImageMipChainAsset::GetSubImageData(uint32_t subImageIndex) const
        {
            AZ_Assert(subImageIndex < m_subImageDataOffsets.size() && subImageIndex < m_subImageDatas.size(), "subImageIndex is out of range");

            // The offset vector contains an extra sentinel value.
            const size_t dataSize = m_subImageDataOffsets[subImageIndex + 1] - m_subImageDataOffsets[subImageIndex];

            return AZStd::span<const uint8_t>(reinterpret_cast<const uint8_t*>(m_subImageDatas[subImageIndex].m_data), dataSize);
        }

        const RHI::DeviceImageSubresourceLayout& ImageMipChainAsset::GetSubImageLayout(uint32_t mipSlice) const
        {
            return m_subImageLayouts[mipSlice];
        }

        const ImageMipChainAsset::MipSliceList& ImageMipChainAsset::GetMipSlices() const
        {
            return m_mipSlices;
        }

        size_t ImageMipChainAsset::GetImageDataSize() const
        {
            return m_imageData.size();
        }

        void ImageMipChainAsset::CopyFrom(const ImageMipChainAsset& source)
        {
            m_mipLevels = source.m_mipLevels;
            m_arraySize = source.m_arraySize;
            m_mipToSubImageOffset = source.m_mipToSubImageOffset;
            m_subImageLayouts = source.m_subImageLayouts;
            m_subImageDataOffsets = source.m_subImageDataOffsets;
            m_imageData = source.m_imageData;

            Init();
        }

        void ImageMipChainAsset::Init()
        {
            const size_t subImageCount = m_mipLevels * m_arraySize;

            AZ_Assert(m_status != AssetStatus::Ready, "ImageMipChainAsset has already been initialized!");
            AZ_Assert(m_subImageDataOffsets.size() == subImageCount + 1, "Expected image data offsets vector to be subImageCount + 1");
            AZ_Assert(m_subImageDatas.empty(), "Expected sub-image data to be empty");

            m_subImageDatas.resize(subImageCount);

            for (size_t subImageIndex = 0; subImageIndex < subImageCount; ++subImageIndex)
            {
                const uintptr_t ptrOffset = m_subImageDataOffsets[subImageIndex];
                const uintptr_t ptrBase = reinterpret_cast<uintptr_t>(m_imageData.data());
                m_subImageDatas[subImageIndex].m_data = reinterpret_cast<const void*>(ptrBase + ptrOffset);
            }

            for (uint16_t mipSliceIndex = 0; mipSliceIndex < m_mipLevels; ++mipSliceIndex)
            {
                RHI::StreamingImageMipSlice mipSlice;
                mipSlice.m_subresources = AZStd::span<const RHI::StreamingImageSubresourceData>(&m_subImageDatas[m_arraySize * mipSliceIndex], m_arraySize);
                mipSlice.m_subresourceLayout = m_subImageLayouts[mipSliceIndex];
                m_mipSlices.push_back(mipSlice);
            }
        }

        void ImageMipChainAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        Data::AssetHandler::LoadResult ImageMipChainAssetHandler::LoadAssetData(
            const Data::Asset<Data::AssetData>& asset,
            AZStd::shared_ptr<Data::AssetDataStream> stream,
            const Data::AssetFilterCB& assetLoadFilterCB)
        {
            Data::AssetHandler::LoadResult result = Base::LoadAssetData(asset, stream, assetLoadFilterCB);
            if (result == Data::AssetHandler::LoadResult::LoadComplete)
            {
                asset.GetAs<ImageMipChainAsset>()->Init();
            }
            return result;
        }
    }
}
