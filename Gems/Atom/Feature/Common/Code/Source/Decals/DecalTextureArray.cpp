/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "DecalTextureArray.h"
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>
#include <Atom/RPI.Public/Image/StreamingImagePool.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAsset.h>
#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <AzCore/Name/NameDictionary.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            static const AZ::Name& GetMapName(const DecalMapType mapType)
            {
                switch (mapType)
                {
                case DecalMapType_Diffuse:
                    return AZ_NAME_LITERAL("baseColor.textureMap");
                case DecalMapType_Normal:
                    return AZ_NAME_LITERAL("normal.textureMap");
                default:
                    return AZ_NAME_LITERAL("");
                }
            }

            static AZ::Data::AssetId GetImagePoolId()
            {
                const Data::Instance<RPI::StreamingImagePool>& imagePool = RPI::ImageSystemInterface::Get()->GetSystemStreamingPool();
                return imagePool->GetAssetId();
            }

            static AZ::Data::Asset<AZ::RPI::MaterialAsset> QueueLoad(const AZ::Data::AssetId id)
            {
                auto asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MaterialAsset>(
                    id, AZ::Data::AssetLoadBehavior::QueueLoad);

                return asset;
            }

            // Extract exactly which texture asset we need to load from the given material and map type (diffuse, normal, etc).
            static AZ::Data::Asset<AZ::RPI::StreamingImageAsset> GetStreamingImageAsset(AZ::RPI::MaterialAsset& materialAsset, const AZ::Name& propertyName)
            {
                if (!materialAsset.IsReady())
                {
                    AZ_Warning("DecalTextureArray", false, "GetStreamingImageAsset() called with material property: %s, was passed a MaterialAsset that was not ready for use", propertyName.GetCStr());
                    return {};
                }

                const AZ::RPI::MaterialPropertiesLayout* materialLayout = materialAsset.GetMaterialPropertiesLayout();
                const AZ::RPI::MaterialPropertyIndex propertyIndex = materialLayout->FindPropertyIndex(propertyName);
                if (propertyIndex.IsNull())
                {
                    AZ_Warning("DecalTextureArray", false, "Unable to find material property with the name: %s", propertyName.GetCStr());
                    return {};
                }

                const auto& propertyValues = materialAsset.GetPropertyValues();
                const AZ::RPI::MaterialPropertyValue& propertyValue = propertyValues[propertyIndex.GetIndex()];
                auto imageAsset = propertyValue.GetValue<Data::Asset<RPI::ImageAsset>>();
                const auto& assetId = imageAsset.GetId();

                if (assetId.IsValid())
                {
                    imageAsset.QueueLoad();
                    // [GFX TODO][ATOM-14271] - DecalTextureArrayFeatureProcessor should use async loading
                    imageAsset.BlockUntilLoadComplete();
                }
                else
                {
                    AZ_Warning("DecalTextureArray", false, "Material property: %s does not have a valid asset Id", propertyName.GetCStr());
                    return {};
                }
                return Data::static_pointer_cast<AZ::RPI::StreamingImageAsset>(imageAsset);
            }

            static AZ::Data::Asset<AZ::RPI::StreamingImageAsset> GetStreamingImageAsset(const AZ::Data::Asset<Data::AssetData> materialAssetData, const AZ::Name& propertyName)
            {
                AZ_Assert(materialAssetData->IsReady(), "GetStreamingImageAsset() called with AssetData that is not ready.");
                AZ::RPI::MaterialAsset* materialAsset = materialAssetData.GetAs<AZ::RPI::MaterialAsset>();
                return GetStreamingImageAsset(*materialAsset, propertyName);
            }
        }

        int DecalTextureArray::FindMaterial(const AZ::Data::AssetId materialAssetId) const
        {
            int iter = m_materials.begin();
            while (iter != -1)
            {
                if (m_materials[iter].m_materialAssetId == materialAssetId)
                {
                    return iter;
                }
                iter = m_materials.next(iter);
            }
            return -1;
        }

        int DecalTextureArray::AddMaterial(const AZ::Data::AssetId materialAssetId)
        {
            AZ_Error("DecalTextureArray", FindMaterial(materialAssetId) == -1, "Adding material when it already exists in the array");
            // Invalidate the existing texture array, as we need to repack it taking into account the new material.
            AZStd::fill(m_textureArrayPacked.begin(), m_textureArrayPacked.end(), nullptr);

            MaterialData materialData;
            materialData.m_materialAssetId = materialAssetId;
            int iter = m_materials.push_front(materialData);
            return iter;
        }

        void DecalTextureArray::RemoveMaterial(const int index)
        {
            m_materials[index] = {};
            m_materials.erase(index);
        }

        AZ::Data::AssetId DecalTextureArray::GetMaterialAssetId(const int index) const
        {
            return m_materials[index].m_materialAssetId;
        }

        RHI::Size DecalTextureArray::GetImageDimensions(const DecalMapType mapType) const
        {
            AZ_Assert(m_materials.size() > 0, "GetImageDimensions() cannot be called until at least one material has been added");
            const int iter = m_materials.begin();
            // All textures in a texture array must have the same size, so just pick the first 
            const MaterialData& firstMaterial = m_materials[iter];
            const auto& baseColorAsset = GetStreamingImageAsset(firstMaterial.m_materialAssetData, GetMapName(mapType));
            return baseColorAsset->GetImageDescriptor().m_size;
        }

        const AZ::Data::Instance<AZ::RPI::StreamingImage>& DecalTextureArray::GetPackedTexture(const DecalMapType mapType) const
        {
            return m_textureArrayPacked[mapType];
        }

        bool DecalTextureArray::IsValidDecalMaterial(AZ::RPI::MaterialAsset& materialAsset)
        {
            return GetStreamingImageAsset(materialAsset, GetMapName(DecalMapType_Diffuse)).IsReady();
        }

        AZ::Data::Asset<AZ::RPI::ImageMipChainAsset> DecalTextureArray::BuildPackedMipChainAsset(const DecalMapType mapType, const size_t numTexturesToCreate)
        {
            RPI::ImageMipChainAssetCreator assetCreator;
            const uint32_t mipLevels = GetNumMipLevels(mapType);

            assetCreator.Begin(Data::AssetId(AZ::Uuid::CreateRandom()), aznumeric_cast<uint16_t>(mipLevels), aznumeric_cast<uint16_t>(numTexturesToCreate));

            for (uint32_t mipLevel = 0; mipLevel < mipLevels; ++mipLevel)
            {
                const auto& layout = GetLayout(mapType, mipLevel);
                assetCreator.BeginMip(layout);

                for (int i = 0; i < m_materials.array_size(); ++i)
                {                   
                    const auto imageData = GetRawImageData(GetMapName(mapType), i, mipLevel);
                    assetCreator.AddSubImage(imageData.data(), imageData.size());
                }

                assetCreator.EndMip();
            }

            Data::Asset<RPI::ImageMipChainAsset> asset;
            assetCreator.End(asset);

            return AZStd::move(asset);
        }

        RHI::ImageDescriptor DecalTextureArray::CreatePackedImageDescriptor(
            const DecalMapType mapType, const uint16_t arraySize, const uint16_t mipLevels) const
        {
            const RHI::Size imageDimensions = GetImageDimensions(mapType);
            RHI::ImageDescriptor imageDescriptor = RHI::ImageDescriptor::Create2DArray(
                RHI::ImageBindFlags::ShaderRead, imageDimensions.m_width, imageDimensions.m_height, arraySize, GetFormat(mapType));
            imageDescriptor.m_mipLevels = mipLevels;
            return imageDescriptor;
        }

        void DecalTextureArray::Pack()
        {
            if (!NeedsPacking())
                return;

            if (!AreAllAssetsReady())
            {
                QueueAssetLoads();
                return;
            }

            const size_t numTexturesToCreate = m_materials.array_size();
            for (int i = 0; i < DecalMapType_Num; ++i)
            {
                const DecalMapType mapType = aznumeric_cast<DecalMapType>(i);
                if (!AreAllTextureMapsPresent(mapType))
                {
                    AZ_Warning("DecalTextureArray", false, "Missing decal texture maps for %s. Please make sure all maps of this type are present.\n", GetMapName(mapType).GetCStr());
                    m_textureArrayPacked[i] = nullptr;
                    continue;
                }

                const auto mipChainAsset = BuildPackedMipChainAsset(mapType, numTexturesToCreate);
                RHI::ImageViewDescriptor imageViewDescriptor;
                imageViewDescriptor.m_isArray = true;

                RPI::StreamingImageAssetCreator assetCreator;
                assetCreator.Begin(Data::AssetId(Uuid::CreateRandom()));
                assetCreator.SetPoolAssetId(GetImagePoolId());
                assetCreator.SetFlags(RPI::StreamingImageFlags::None);
                assetCreator.SetImageDescriptor(
                    CreatePackedImageDescriptor(mapType, aznumeric_cast<uint16_t>(numTexturesToCreate), GetNumMipLevels(mapType)));
                assetCreator.SetImageViewDescriptor(imageViewDescriptor);
                assetCreator.AddMipChainAsset(*mipChainAsset);
                Data::Asset<RPI::StreamingImageAsset> packedAsset;
                const bool createdOk = assetCreator.End(packedAsset);
                AZ_Error("TextureArrayData", createdOk, "Pack() call failed.");
                m_textureArrayPacked[i] = createdOk ? RPI::StreamingImage::FindOrCreate(packedAsset) : nullptr;

            }

            // Free unused memory
            ClearAssets();
        }

        size_t DecalTextureArray::NumMaterials() const
        {
            return m_materials.size();
        }

        void DecalTextureArray::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());

            m_assetsCurrentlyLoading.erase(asset.GetId());
            if (m_assetsCurrentlyLoading.empty())
            {
                Pack();
            }
        }

        uint16_t DecalTextureArray::GetNumMipLevels(const DecalMapType mapType) const
        {
            AZ_Assert(m_materials.size() > 0, "GetNumMipLevels() cannot be called until at least one material has been added");
            // All decals in a texture array must have the same number of mips, so just pick the first 
            const int iter = m_materials.begin();
            const MaterialData& firstMaterial = m_materials[iter];
            const auto& imageAsset = GetStreamingImageAsset(firstMaterial.m_materialAssetData, GetMapName(mapType));
            return imageAsset->GetImageDescriptor().m_mipLevels;
        }

        RHI::DeviceImageSubresourceLayout DecalTextureArray::GetLayout(const DecalMapType mapType, int mip) const
        {
            AZ_Assert(m_materials.size() > 0, "GetLayout() cannot be called unless at least one material has been added");

            const int iter = m_materials.begin();
            const auto& descriptor =
                GetStreamingImageAsset(m_materials[iter].m_materialAssetData, GetMapName(mapType))->GetImageDescriptor();
            RHI::Size mipSize = descriptor.m_size;
            mipSize.m_width >>= mip;
            mipSize.m_height >>= mip;
            return AZ::RHI::GetImageSubresourceLayout(mipSize, descriptor.m_format);
        }

        AZStd::span<const uint8_t> DecalTextureArray::GetRawImageData(const AZ::Name& mapName, int arrayLevel, const int mip) const
        {
            // We always want to provide valid data to the AssetCreator for each texture.
            // If this spot in the array is empty, just provide some random image as filler.
            // (No decals will be indexing this spot anyway)
            const bool isEmptySlot = !m_materials[arrayLevel].m_materialAssetData.GetId().IsValid();
            if (isEmptySlot)
            {
                arrayLevel = m_materials.begin();
            }
            const auto image = GetStreamingImageAsset(m_materials[arrayLevel].m_materialAssetData, mapName);
            if (!image)
            {
                return {};
            }
            AZ_Assert(
                mip < image->GetImageDescriptor().m_mipLevels,
                "It is expected that all decals in a texture array must have the same number of mips which may not be the case here. "
                "Please ensure that all the materials within m_materials are pointing to textures with same mips.");

            const auto srcData = image->GetSubImageData(mip, 0);
            return srcData;
        }

        AZ::RHI::Format DecalTextureArray::GetFormat(const DecalMapType mapType) const
        {
            AZ_Assert(m_materials.size() > 0, "GetFormat() can only be called after at least one material has been added.");
            const int iter = m_materials.begin();
            const auto& baseColorAsset = GetStreamingImageAsset(m_materials[iter].m_materialAssetData, GetMapName(mapType));
            return baseColorAsset->GetImageDescriptor().m_format;
        }

        bool DecalTextureArray::AreAllAssetsReady() const
        {
            int iter = m_materials.begin();
            while (iter != -1)
            {
                if (!IsAssetReady(m_materials[iter]))
                    return false;

                iter = m_materials.next(iter);
            }
            return true;
        }

        bool DecalTextureArray::IsAssetReady(const MaterialData& materialData) const
        {
            const auto& id = materialData.m_materialAssetData.GetId();
            return id.IsValid() && materialData.m_materialAssetData.IsReady();
        }

        bool DecalTextureArray::AreAllTextureMapsPresent(const DecalMapType mapType) const
        {
            int iter = m_materials.begin();
            while (iter != -1)
            {
                if (!IsTextureMapPresentInMaterial(m_materials[iter], mapType))
                {                    
                    return false;
                }
                iter = m_materials.next(iter);
            }
            return true;
        }

        bool DecalTextureArray::IsTextureMapPresentInMaterial(const MaterialData& materialData, const DecalMapType mapType) const
        {
            return GetStreamingImageAsset(materialData.m_materialAssetData, GetMapName(mapType)).IsReady();
        }

        void DecalTextureArray::ClearAssets()
        {
            int iter = m_materials.begin();
            while (iter != -1)
            {
                ClearAsset(m_materials[iter]);
                iter = m_materials.next(iter);
            }
        }

        void DecalTextureArray::ClearAsset(MaterialData& materialData)
        {
            materialData.m_materialAssetData = {};
        }

        void DecalTextureArray::QueueAssetLoad(MaterialData& materialData)
        {
            if (materialData.m_materialAssetData.IsReady())
                return;

            m_assetsCurrentlyLoading.emplace(materialData.m_materialAssetId);
            materialData.m_materialAssetData = QueueLoad(materialData.m_materialAssetId);
            AZ::Data::AssetBus::MultiHandler::BusConnect(materialData.m_materialAssetId);
        }

        void DecalTextureArray::QueueAssetLoads()
        {
            int iter = m_materials.begin();
            while (iter != -1)
            {
                QueueAssetLoad(m_materials[iter]);
                iter = m_materials.next(iter);
            }
        }

        bool DecalTextureArray::NeedsPacking() const
        {
            if (m_materials.size() == 0)
                return false;

            // We pack all diffuse/normal/etc in one go, so just check to see if the diffusemaps need packing
            return m_textureArrayPacked[DecalMapType_Diffuse] == nullptr;
        }

    }
} // namespace AZ
