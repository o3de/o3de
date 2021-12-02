/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Decals/DecalTextureArrayFeatureProcessor.h>
#include <AzCore/Debug/EventTrace.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <AzCore/Math/Quaternion.h>
#include <AtomCore/std/containers/array_view.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <AtomCore/Instance/InstanceDatabase.h>
#include <Atom/RPI.Reflect/Asset/AssetUtils.h>

namespace AZ
{
    namespace Render
    {
        namespace
        {
            static AZ::RHI::Size GetTextureSizeFromMaterialAsset(const AZ::RPI::MaterialAsset* materialAsset)
            {
                for (const auto& elem : materialAsset->GetPropertyValues())
                {
                    if (elem.Is<Data::Asset<RPI::ImageAsset>>())
                    {
                        const auto& imageBinding = elem.GetValue<Data::Asset<RPI::ImageAsset>>();
                        const auto& assetId = imageBinding.GetId();
                        if (assetId.IsValid())
                        {
                            return imageBinding->GetImageDescriptor().m_size;
                        }
                    }
                }
                AZ_Assert(false, "GetSizeFromMaterial() unable to find an image in the given material.");
                return {};
            }

            static AZ::Data::Asset<AZ::RPI::MaterialAsset> QueueMaterialAssetLoad(const AZ::Data::AssetId material)
            {
                auto asset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::MaterialAsset>(material, AZ::Data::AssetLoadBehavior::QueueLoad);
                return asset;
            }
        }

        void DecalTextureArrayFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<DecalTextureArrayFeatureProcessor, RPI::FeatureProcessor>()
                    ->Version(0);
            }
        }

        void DecalTextureArrayFeatureProcessor::Activate()
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "DecalBuffer";
            desc.m_bufferSrgName = "m_decals";
            desc.m_elementCountSrgName = "m_decalCount";
            desc.m_elementSize = sizeof(DecalData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

            m_decalBufferHandler = GpuBufferHandler(desc);

            CacheShaderIndices();
        }

        void DecalTextureArrayFeatureProcessor::Deactivate()
        {
            AZ::Data::AssetBus::MultiHandler::BusDisconnect();

            m_decalData.Clear();
            m_decalBufferHandler.Release();
        }

        DecalTextureArrayFeatureProcessor::DecalHandle DecalTextureArrayFeatureProcessor::AcquireDecal()
        {
            const uint16_t id = m_decalData.GetFreeSlotIndex();

            if (id == IndexedDataVector<DecalData>::NoFreeSlot)
            {
                return DecalHandle(DecalHandle::NullIndex);
            }
            else
            {
                m_deviceBufferNeedsUpdate = true;
                DecalData& decalData = m_decalData.GetData(id);
                decalData.m_textureArrayIndex = DecalData::UnusedIndex;
                return DecalHandle(id);
            }
        }

        bool DecalTextureArrayFeatureProcessor::ReleaseDecal(const DecalHandle decal)
        {
            if (decal.IsValid())
            {
                if (m_materialLoadTracker.IsAssetLoading(decal))
                {
                    m_materialLoadTracker.RemoveHandle(decal);
                }

                DecalLocation decalLocation;
                decalLocation.textureArrayIndex = m_decalData.GetData(decal.GetIndex()).m_textureArrayIndex;
                decalLocation.textureIndex = m_decalData.GetData(decal.GetIndex()).m_textureIndex;
                RemoveDecalFromTextureArrays(decalLocation);

                m_decalData.RemoveIndex(decal.GetIndex());
                m_deviceBufferNeedsUpdate = true;
                return true;
            }
            return false;
        }

        DecalTextureArrayFeatureProcessor::DecalHandle DecalTextureArrayFeatureProcessor::CloneDecal(const DecalHandle sourceDecal)
        {
            AZ_Assert(sourceDecal.IsValid(), "Invalid DecalHandle passed to DecalTextureArrayFeatureProcessor::CloneDecal().");

            const DecalHandle decal = AcquireDecal();
            if (decal.IsValid())
            {
                m_decalData.GetData(decal.GetIndex()) = m_decalData.GetData(sourceDecal.GetIndex());
                const auto materialAsset = GetMaterialUsedByDecal(sourceDecal);
                if (materialAsset.IsValid())
                {
                    m_materialToTextureArrayLookupTable.at(materialAsset).m_useCount++;
                }
                else
                {
                    AZ_Warning("DecalTextureArrayFeatureProcessor", false, "CloneDecal called on a decal with no material set.");
                }
                m_deviceBufferNeedsUpdate = true;
            }
            return decal;
        }

        void DecalTextureArrayFeatureProcessor::Simulate(const RPI::FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(AzRender, "DecalTextureArrayFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                m_decalBufferHandler.UpdateBuffer(m_decalData.GetDataVector());
                m_deviceBufferNeedsUpdate = false;
            }
        }

        void DecalTextureArrayFeatureProcessor::Render(const RPI::FeatureProcessor::RenderPacket& packet)
        {
            // Note that decals are rendered as part of the forward shading pipeline. We only need to bind the decal buffers/textures in here.
            AZ_PROFILE_SCOPE(AzRender, "DecalTextureArrayFeatureProcessor: Render");

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_decalBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
                SetPackedTexturesToSrg(view);
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalData(const DecalHandle handle, const DecalData& data)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData(handle.GetIndex()) = data;
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalData().");
            }
        }

        const Data::Instance<RPI::Buffer> DecalTextureArrayFeatureProcessor::GetDecalBuffer() const
        {
            return m_decalBufferHandler.GetBuffer();
        }

        uint32_t DecalTextureArrayFeatureProcessor::GetDecalCount() const
        {
            return m_decalBufferHandler.GetElementCount();
        }

        void DecalTextureArrayFeatureProcessor::SetDecalPosition(const DecalHandle handle, const AZ::Vector3& position)
        {
            if (handle.IsValid())
            {
                AZStd::array<float, 3>& writePos = m_decalData.GetData(handle.GetIndex()).m_position;
                position.StoreToFloat3(writePos.data());
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalPosition().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalOrientation(DecalHandle handle, const AZ::Quaternion& orientation)
        {
            if (handle.IsValid())
            {
                orientation.StoreToFloat4(m_decalData.GetData(handle.GetIndex()).m_quaternion.data());
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalOrientation().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalHalfSize(DecalHandle handle, const Vector3& halfSize)
        {
            if (handle.IsValid())
            {
                halfSize.StoreToFloat3(m_decalData.GetData(handle.GetIndex()).m_halfSize.data());
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalHalfSize().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalAttenuationAngle(const DecalHandle handle, float angleAttenuation)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData(handle.GetIndex()).m_angleAttenuation = angleAttenuation;
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", handle.IsValid(), "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalAttenuationAngle().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalOpacity(const DecalHandle handle, float opacity)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData(handle.GetIndex()).m_opacity = opacity;

                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalOpacity().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalSortKey(DecalHandle handle, uint8_t sortKey)
        {
            if (handle.IsValid())
            {
                m_decalData.GetData(handle.GetIndex()).m_sortKey = sortKey;
                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalSortKey().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalTransform(DecalHandle handle, const AZ::Transform& world)
        {
            SetDecalTransform(handle, world, AZ::Vector3::CreateOne());
        }

        void DecalTextureArrayFeatureProcessor::SetDecalTransform(DecalHandle handle, const AZ::Transform& world,
            const AZ::Vector3& nonUniformScale)
        {
            if (handle.IsValid())
            {
                SetDecalHalfSize(handle, nonUniformScale * world.GetUniformScale());
                SetDecalPosition(handle, world.GetTranslation());
                SetDecalOrientation(handle, world.GetRotation());

                m_deviceBufferNeedsUpdate = true;
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalTransform().");
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalMaterial(const DecalHandle handle, const AZ::Data::AssetId material)
        {
            AZ_PROFILE_SCOPE(AzRender, "DecalTextureArrayFeatureProcessor: SetDecalMaterial");
            if (handle.IsNull())
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Invalid handle passed to DecalTextureArrayFeatureProcessor::SetDecalMaterial().");
                return;
            }

            if (GetMaterialUsedByDecal(handle) == material)
            {
                return;
            }

            const auto decalIndex = handle.GetIndex();

            const bool isValidMaterialBeingUsedCurrently = m_decalData.GetData(decalIndex).m_textureArrayIndex != DecalData::UnusedIndex;
            if (isValidMaterialBeingUsedCurrently)
            {
                RemoveMaterialFromDecal(decalIndex);
            }

            if (!material.IsValid())
            {
                return;
            }

            const auto iter = m_materialToTextureArrayLookupTable.find(material);
            if (iter != m_materialToTextureArrayLookupTable.end())
            {
                // This material is already loaded and registered with this feature processor
                iter->second.m_useCount++;
                SetDecalTextureLocation(handle, iter->second.m_location);
                return;
            }

            // Material not loaded so queue it up for loading.
            QueueMaterialLoadForDecal(material, handle);
        }

        void DecalTextureArrayFeatureProcessor::RemoveMaterialFromDecal(const uint16_t decalIndex)
        {
            auto& decalData = m_decalData.GetData(decalIndex);

            DecalLocation decalLocation;
            decalLocation.textureArrayIndex = decalData.m_textureArrayIndex;
            decalLocation.textureIndex = decalData.m_textureIndex;
            RemoveDecalFromTextureArrays(decalLocation);

            decalData.m_textureArrayIndex = DecalData::UnusedIndex;
            decalData.m_textureIndex = DecalData::UnusedIndex;

            m_deviceBufferNeedsUpdate = true;
        }

        void DecalTextureArrayFeatureProcessor::CacheShaderIndices()
        {
            // The azsl shader should define several texture arrays such as:
            // Texture2DArray<float4> m_decalTextureArrayDiffuse0;
            // Texture2DArray<float4> m_decalTextureArrayDiffuse1;
            // Texture2DArray<float4> m_decalTextureArrayDiffuse2;
            // and
            // Texture2DArray<float2> m_decalTextureArrayNormalMaps0;
            // Texture2DArray<float2> m_decalTextureArrayNormalMaps1;
            // Texture2DArray<float2> m_decalTextureArrayNormalMaps2;
            static constexpr AZStd::array<AZStd::string_view, DecalMapType_Num> ShaderNames = { "m_decalTextureArrayDiffuse",
                                                                                       "m_decalTextureArrayNormalMaps" };

            for (int mapType = 0; mapType < DecalMapType_Num; ++mapType)
            {
                for (int texArrayIdx = 0; texArrayIdx < NumTextureArrays; ++texArrayIdx)
                {
                    const RHI::ShaderResourceGroupLayout* viewSrgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();
                    const AZStd::string baseName = AZStd::string(ShaderNames[mapType]) + AZStd::to_string(texArrayIdx);

                    m_decalTextureArrayIndices[texArrayIdx][mapType] = viewSrgLayout->FindShaderInputImageIndex(Name(baseName.c_str()));
                    AZ_Warning(
                        "DecalTextureArrayFeatureProcessor", m_decalTextureArrayIndices[texArrayIdx][mapType].IsValid(),
                        "Unable to find %s in decal shader.",
                        baseName.c_str());
                }
            }
        }

        AZStd::optional<AZ::Render::DecalTextureArrayFeatureProcessor::DecalLocation> DecalTextureArrayFeatureProcessor::AddMaterialToTextureArrays(const AZ::RPI::MaterialAsset* materialAsset)
        {
            const RHI::Size textureSize = GetTextureSizeFromMaterialAsset(materialAsset);

            int textureArrayIndex = FindTextureArrayWithSize(textureSize);
            const bool wasExistingTextureArrayFoundForGivenSize = textureArrayIndex != -1;
            if (m_textureArrayList.size() == NumTextureArrays && !wasExistingTextureArrayFoundForGivenSize)
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "Unable to add decal with size %u %u. There are no more texture arrays left to accept a decal with this size permutation.", textureSize.m_width, textureSize.m_height);
                return AZStd::nullopt;
            }

            int textureIndex;
            if (!wasExistingTextureArrayFoundForGivenSize)
            {
                DecalTextureArray decalTextureArray;
                textureIndex = decalTextureArray.AddMaterial(materialAsset->GetId());
                textureArrayIndex = m_textureArrayList.push_front(AZStd::make_pair(textureSize, decalTextureArray));
            }
            else
            {
                textureIndex = m_textureArrayList[textureArrayIndex].second.AddMaterial(materialAsset->GetId());
            }

            DecalLocation result;
            result.textureArrayIndex = textureArrayIndex;
            result.textureIndex = textureIndex;
            return result;
        }

        void DecalTextureArrayFeatureProcessor::OnAssetReady(const Data::Asset<Data::AssetData> asset)
        {
            AZ_PROFILE_SCOPE(AzRender, "DecalTextureArrayFeatureProcessor: OnAssetReady");
            const Data::AssetId& assetId = asset->GetId();
            
            const RPI::MaterialAsset* materialAsset = asset.GetAs<AZ::RPI::MaterialAsset>();
            const bool validDecalMaterial = materialAsset && DecalTextureArray::IsValidDecalMaterial(*materialAsset);
            if (validDecalMaterial)
            {
                const auto& decalsThatUseThisMaterial = m_materialLoadTracker.GetHandlesByAsset(asset.GetId());
                const auto& decalLocation = AddMaterialToTextureArrays(materialAsset);

                if (decalLocation)
                {
                    for (const auto& decal : decalsThatUseThisMaterial)
                    {
                        m_materialToTextureArrayLookupTable[assetId].m_useCount++;
                        SetDecalTextureLocation(decal, *decalLocation);
                    }
                    m_materialToTextureArrayLookupTable[assetId].m_location = *decalLocation;
                }
            }
            else
            {
                AZ_Warning("DecalTextureArrayFeatureProcessor", false, "DecalTextureArray::IsValidDecalMaterial() failed, unable to add this material to the decal");
            }

            m_materialLoadTracker.RemoveAllHandlesWithAsset(assetId);
            AZ::Data::AssetBus::MultiHandler::BusDisconnect(assetId);

            if (!m_materialLoadTracker.AreAnyLoadsInFlight())
            {
                PackTexureArrays();
            }
        }

        void DecalTextureArrayFeatureProcessor::SetDecalTextureLocation(const DecalHandle& handle, const DecalLocation location)
        {
            AZ_Assert(handle.IsValid(), "SetDecalTextureLocation called with invalid handle");
            m_decalData.GetData(handle.GetIndex()).m_textureArrayIndex = location.textureArrayIndex;
            m_decalData.GetData(handle.GetIndex()).m_textureIndex = location.textureIndex;
            m_deviceBufferNeedsUpdate = true;
        }

        void DecalTextureArrayFeatureProcessor::SetPackedTexturesToSrg(const RPI::ViewPtr& view)
        {
            int iter = m_textureArrayList.begin();
            while (iter != -1)
            {
                for (int mapType = 0 ; mapType < DecalMapType_Num ; ++mapType)
                {
                    const auto& packedTexture = m_textureArrayList[iter].second.GetPackedTexture(aznumeric_cast<DecalMapType>(mapType));
                    view->GetShaderResourceGroup()->SetImage(m_decalTextureArrayIndices[iter][mapType], packedTexture);
                }
                iter = m_textureArrayList.next(iter);
            }
        }

        int DecalTextureArrayFeatureProcessor::FindTextureArrayWithSize(const RHI::Size& size) const
        {
            int iter = m_textureArrayList.begin();
            while (iter != -1)
            {
                if (m_textureArrayList[iter].first == size)
                {
                    return iter;
                }

                iter = m_textureArrayList.next(iter);
            }
            return -1;
        }

        bool DecalTextureArrayFeatureProcessor::RemoveDecalFromTextureArrays(const DecalLocation decalLocation)
        {
            if (decalLocation.textureArrayIndex != DecalData::UnusedIndex)
            {
                auto& textureArray = m_textureArrayList[decalLocation.textureArrayIndex].second;

                const AZ::Data::AssetId material = textureArray.GetMaterialAssetId(decalLocation.textureIndex);
                auto iter = m_materialToTextureArrayLookupTable.find(material);
                AZ_Assert(iter != m_materialToTextureArrayLookupTable.end(), "Bad state");
                DecalLocationAndUseCount& decalInformation = iter->second;
                decalInformation.m_useCount--;

                if (decalInformation.m_useCount == 0)
                {
                    m_materialToTextureArrayLookupTable.erase(iter);
                    textureArray.RemoveMaterial(decalLocation.textureIndex);
                }

                if (textureArray.NumMaterials() == 0)
                {
                    m_textureArrayList.erase(decalLocation.textureArrayIndex);
                }
            }
            return false;
        }

        void DecalTextureArrayFeatureProcessor::PackTexureArrays()
        {
            int iter = m_textureArrayList.begin();
            while (iter != -1)
            {
                m_textureArrayList[iter].second.Pack();
                iter = m_textureArrayList.next(iter);
            }
        }

        AZ::Data::AssetId DecalTextureArrayFeatureProcessor::GetMaterialUsedByDecal(const DecalHandle handle) const
        {
            AZ::Data::AssetId material;
            if (handle.IsValid())
            {
                const DecalData& decalData = m_decalData.GetData(handle.GetIndex());
                if (decalData.m_textureArrayIndex != DecalData::UnusedIndex)
                {
                    const DecalTextureArray& textureArray = m_textureArrayList[decalData.m_textureArrayIndex].second;
                    material = textureArray.GetMaterialAssetId(decalData.m_textureIndex);
                }
            }
            return material;
        }

        void DecalTextureArrayFeatureProcessor::QueueMaterialLoadForDecal(const AZ::Data::AssetId materialId, const DecalHandle handle)
        {
            const auto materialAsset = QueueMaterialAssetLoad(materialId);

            m_materialLoadTracker.TrackAssetLoad(handle, materialAsset);

            if (materialAsset.IsLoading())
            {
                AZ::Data::AssetBus::MultiHandler::BusConnect(materialId);
            }
            else if (materialAsset.IsReady())
            {
                OnAssetReady(materialAsset);
            }
            else
            {
                AZ_Assert(false, "DecalTextureArrayFeatureProcessor::QueueMaterialLoadForDecal is in an unhandled state.");
            }
        }

    } // namespace Render
} // namespace AZ
