/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Processing/ImageAssetProducer.h>
#include <Processing/ImageObjectImpl.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/Utils.h>
#include <Processing/ImageFlags.h>

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>

#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/ImageAsset.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Serialization/Json/JsonUtils.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace ImageProcessingAtom
{
    using namespace AZ;

    namespace
    {
        // Trying to fit as many as mips to 64k memory for one mip chain
        const uint32_t s_mininumMipBlockSize = 64 * 1024;
    }

    ImageAssetProducer::ImageAssetProducer(
        const IImageObjectPtr imageObject,
        AZStd::string_view saveFolder,
        const Data::AssetId& sourceAssetId,
        AZStd::string_view fileName,
        uint8_t numResidentMips,
        uint32_t subId,
        AZStd::set<AZStd::string> tags)
        : m_imageObject(imageObject)
        , m_productFolder(saveFolder)
        , m_sourceAssetId(sourceAssetId)
        , m_fileName(fileName)
        , m_numResidentMips(numResidentMips)
        , m_subId(subId)
        , m_tags(AZStd::move(tags))
    {
        AZ_Assert(imageObject, "Input imageObject can't be empty");
        AZ_Assert(sourceAssetId.IsValid(), "The source asset Id is not valid");
        AZ_Assert(saveFolder.size() != 0, "saveFolder shouldn't be empty");
        AZ_Assert(fileName.size() != 0, "fileName shouldn't be empty");
    }

    AZStd::string ImageAssetProducer::GenerateAssetFullPath(ImageAssetType assetType, uint32_t assetSubId)
    {
        // Note: m_fileName is the filename contains original extension to avoid file name collision if there are files with same name but with different extension.
        // For example, test.jpg's image asset full path will be outputFolder/test.jpg.streamingimage.
        // And test.png's image asset full path will be outputFolder/test.png.streamingimage.
        AZStd::string fileName;
        if (assetType == ImageAssetType::MipChain)
        {
            fileName = AZStd::string::format("%s.%d.%s", m_fileName.c_str(), assetSubId, RPI::ImageMipChainAsset::Extension);
        }
        else if (assetType == ImageAssetType::Image)
        {
            fileName = AZStd::string::format("%s.%s", m_fileName.c_str(), RPI::StreamingImageAsset::Extension);
        }
        else
        {
            AZ_Assert(false, "Unknown image asset type");
        }
        AZStd::string outPath;
        AzFramework::StringFunc::Path::Join(m_productFolder.c_str(), fileName.c_str(), outPath, true, true);
        return outPath;
    }

    const AZStd::vector<AssetBuilderSDK::JobProduct>& ImageAssetProducer::GetJobProducts() const
    {
        return m_jobProducts;
    }

    bool ImageAssetProducer::BuildImageAssets()
    {
        Data::AssetId imageAssetId(m_sourceAssetId.m_guid, m_subId);

        AssetBuilderSDK::JobProduct product;
        RPI::StreamingImageAssetCreator builder;
        builder.Begin(imageAssetId);

        uint32_t imageDepth = m_imageObject->GetDepth(0);
        const bool isVolumeTexture = m_imageObject->HasImageFlags(EIF_Volumetexture) || (imageDepth > 1);
        if (m_imageObject->HasImageFlags(EIF_Cubemap) && isVolumeTexture)
        {
            AZ_Assert(false, "An image can not be a cubemap and a volume texture at the same time!");
            return false;
        }

        int32_t arraySize = m_imageObject->HasImageFlags(EIF_Cubemap) ? 6 : 1;
        uint32_t imageWidth = m_imageObject->GetWidth(0);
        // The current cubemap faces are vertically aligned in the same buffer of image object. So the height should be divided by the array size.
        uint32_t imageHeight = m_imageObject->GetHeight(0) / arraySize;
        RHI::Format format = Utils::PixelFormatToRHIFormat(m_imageObject->GetPixelFormat(), m_imageObject->HasImageFlags(EIF_SRGBRead));
        RHI::ImageBindFlags bindFlag = RHI::ImageBindFlags::ShaderRead;

        RHI::ImageDescriptor imageDesc = isVolumeTexture
            ? RHI::ImageDescriptor::Create3D(bindFlag, imageWidth, imageHeight, imageDepth, format)
            : RHI::ImageDescriptor::Create2DArray(bindFlag, imageWidth, imageHeight, static_cast<uint16_t>(arraySize), format);
        imageDesc.m_mipLevels = static_cast<uint16_t>(m_imageObject->GetMipCount());
        if (m_imageObject->HasImageFlags(EIF_Cubemap))
        {
            imageDesc.m_isCubemap = true;
        }

        builder.SetImageDescriptor(imageDesc);

        // Set ImageViewDescriptor for cubemap. The regular 2d images can use the default one.
        if (m_imageObject->HasImageFlags(EIF_Cubemap))
        {
            builder.SetImageViewDescriptor(RHI::ImageViewDescriptor::CreateCubemap());
        }

        // Build mip chain assets.
        // Start from smallest mips so the mip chain asset for the lowest resolutions may contain more high level mips
        uint32_t lastMip = imageDesc.m_mipLevels;
        uint32_t totalSize = 0;
        AZStd::vector<Data::Asset<RPI::ImageMipChainAsset>> mipChains;
        uint32_t subid = m_subId + 1;
        bool saveProduct = false; // used to skip exporting the tail mip chain to job product

        // Merge m_residentMipCount amount of mips into a single mip chain, and add it to the StreamingImageAsset's tail mip chain
        if (m_numResidentMips != 0)
        {
            const uint32_t boundedResidentMipCount = AZStd::min(static_cast<uint32_t>(m_numResidentMips), lastMip);
            const uint32_t residentStartMip = lastMip - boundedResidentMipCount;

            // Build the StreamingImageAsset's tail mip chain
            Data::Asset<RPI::ImageMipChainAsset> chainAsset;
            const bool isSuccess = BuildMipChainAsset(Data::AssetId(m_sourceAssetId.m_guid, subid), residentStartMip, boundedResidentMipCount, chainAsset, saveProduct);
            if (!isSuccess)
            {
                AZ_Warning("Image Processing", false, "Failed to build the StreamingImageAsset's m_tailMipChain");
                return false;
            }

            mipChains.emplace_back(chainAsset);

            // Set the state for the remaining mips
            saveProduct = true;
            lastMip = residentStartMip;
            subid++;
        }

        // Create MipChainAssets for the remaining mips
        for (int32_t mip = lastMip - 1; mip >= 0; mip--)
        {
            totalSize += m_imageObject->GetMipBufSize(mip);

            // Trying to fit as many as mips to 64k memory for one mip chain
            if (mip == 0 || totalSize + m_imageObject->GetMipBufSize(mip - 1) > s_mininumMipBlockSize)
            {
                // Build a chain
                Data::Asset<RPI::ImageMipChainAsset> chainAsset;
                bool isSuccess = BuildMipChainAsset(Data::AssetId(m_sourceAssetId.m_guid, subid), mip, lastMip - mip, chainAsset, saveProduct);
                saveProduct = true;
                if (!isSuccess)
                {
                    AZ_Warning("Image Processing", false, "Failed to build mip chain asset");
                    return false;
                }

                mipChains.emplace_back(chainAsset);

                subid++;
                lastMip = mip;
                totalSize = 0;
            }
        }

        // Set the builder's state to non-streaming if it only has a single mip chain, which will be stored in the
        // StreamingImage's tail mip chain
        if (mipChains.size() == 1)
        {
            builder.SetFlags(AZ::RPI::StreamingImageFlags::NotStreamable);
        }

        // Add mip chains to the builder from mip level 0 to highest
        for (auto it = mipChains.rbegin(); it != mipChains.rend(); it++)
        {
            builder.AddMipChainAsset(*it->GetAs<RPI::ImageMipChainAsset>());

            // Add all the mip chain assets as dependencies except the tail mip chain since its embedded in the StreamingImageAsset
            if (it->Get() != mipChains.begin()->Get())
            {
                // Note: we don't want to preload the mipchain assets here to reduce loading time and memory footprint.
                // The mipchain assets will be loaded by StreamingImage automatically or loaded when user is accessing the mipmap data via
                // StreamingImageAsset::GetSubImageData() function
                product.m_dependencies.push_back(AssetBuilderSDK::ProductDependency(it->GetId(), AZ::Data::ProductDependencyInfo::CreateFlags(AZ::Data::AssetLoadBehavior::NoLoad)));
            }
        }

        builder.SetAverageColor(m_imageObject->GetAverageColor());

        for (const AZStd::string& tag : m_tags)
        {
            builder.AddTag(AZ::Name{ tag });
        }

        product.m_dependenciesHandled = true; // We've output the dependencies immediately above so it's OK to tell the AP we've handled dependencies

        bool result = false;

        Data::Asset<RPI::StreamingImageAsset> imageAsset;
        if (builder.End(imageAsset))
        {
            AZStd::string destPath = GenerateAssetFullPath(ImageAssetType::Image, imageAsset.GetId().m_subId);
            result = AZ::Utils::SaveObjectToFile(destPath, AZ::DataStream::ST_BINARY, imageAsset.GetData(), imageAsset.GetData()->GetType(), nullptr);
            if (!result)
            {
                AZ_Warning("Image Processing", false, "Failed to save image asset to file %s", destPath.c_str());
            }
            else
            {
                product.m_productAssetType = imageAsset.GetData()->GetType();
                product.m_productSubID = imageAsset.GetId().m_subId;
                product.m_productFileName = destPath;

                auto& imageDescriptor = imageAsset->GetImageDescriptor();
                AZStd::string folder;
                AZStd::string jsonName;
                folder = AZStd::string::format("%s/%s.abdata.json", m_productFolder.c_str(), m_fileName.c_str());

                AssetBuilderSDK::CreateABDataFile(folder,
                    [imageDescriptor](rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
                {
                    writer.Key("dimension");
                    writer.StartArray();
                    writer.Double(imageDescriptor.m_size.m_width);
                    writer.Double(imageDescriptor.m_size.m_height);
                    writer.Double(imageDescriptor.m_size.m_depth);
                    writer.EndArray();
                });

                AssetBuilderSDK::JobProduct jsonProduct(folder);
                jsonProduct.m_productSubID |= product.m_productSubID;
                m_jobProducts.emplace_back(AZStd::move(jsonProduct));

                // The StreamingImageAsset is added to end of product list on purpose.
                // This is in case a new mip chain file is generated, for example when updating the original image's resolution,
                // the mip chain asset hasn't be registered by the AssetCatalog when processing the asset change notification for
                // StreamingImageAsset reload and it leads to an unknown asset error.
                // The Asset system can be modified to solve the problem so the order doesn't matter.
                // The task is tracked in ATOM-242
                m_jobProducts.emplace_back(AZStd::move(product));
            }
        }

        return result;
    }

    bool ImageAssetProducer::BuildMipChainAsset(const Data::AssetId& chainAssetId, uint32_t startMip, uint32_t mipLevels,
        Data::Asset<RPI::ImageMipChainAsset>& outAsset, bool saveAsProduct)
    {
        RPI::ImageMipChainAssetCreator builder;
        uint32_t arraySize = m_imageObject->HasImageFlags(EIF_Cubemap) ? 6 : 1;
        builder.Begin(chainAssetId, static_cast<uint16_t>(mipLevels), static_cast<uint16_t>(arraySize));

        for (uint32_t mip = startMip; mip < startMip + mipLevels; mip++)
        {
            uint8_t* mipBuffer;
            uint32_t pitch;
            m_imageObject->GetImagePointer(mip, mipBuffer, pitch);
            RHI::Format format = Utils::PixelFormatToRHIFormat(m_imageObject->GetPixelFormat(), m_imageObject->HasImageFlags(EIF_SRGBRead));

            const auto mipSize = (m_imageObject->GetDepth(0) == 1)
                ? RHI::Size(m_imageObject->GetWidth(mip), m_imageObject->GetHeight(mip) / arraySize, 1)
                : RHI::Size(m_imageObject->GetWidth(mip), m_imageObject->GetHeight(mip), m_imageObject->GetDepth(mip));
            RHI::DeviceImageSubresourceLayout layout = RHI::GetImageSubresourceLayout(mipSize, format);
            const auto mipSizeInBytes = layout.m_bytesPerImage * mipSize.m_depth;
            builder.BeginMip(layout);

            for (uint32_t arrayIndex = 0; arrayIndex < arraySize; ++arrayIndex)
            {
                builder.AddSubImage(mipBuffer + arrayIndex * mipSizeInBytes, mipSizeInBytes);
            }

            builder.EndMip();
        }

        bool result = false;
        if (builder.End(outAsset))
        {
            AZStd::string destPath = GenerateAssetFullPath(ImageAssetType::MipChain, outAsset.GetId().m_subId);

            if (saveAsProduct)
            {
                result = AZ::Utils::SaveObjectToFile(destPath, AZ::DataStream::ST_BINARY, outAsset.GetData(), outAsset.GetData()->GetType(), nullptr);

                if (result)
                {
                    // Save job product
                    AssetBuilderSDK::JobProduct product;
                    product.m_productAssetType = outAsset.GetData()->GetType();
                    product.m_productSubID = outAsset.GetId().m_subId;
                    product.m_productFileName = destPath;
                    m_jobProducts.emplace_back(AZStd::move(product));
                }
            }
            else
            {
                result = true;
            }
        }

        return result;
    }
} // namespace ImageProcessingAtom
