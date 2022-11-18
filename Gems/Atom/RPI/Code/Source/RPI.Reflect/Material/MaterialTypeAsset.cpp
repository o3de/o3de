
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        const char* MaterialTypeAsset::DisplayName = "MaterialTypeAsset";
        const char* MaterialTypeAsset::Group = "Material";
        const char* MaterialTypeAsset::Extension = "azmaterialtype";

        void UvNamePair::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<UvNamePair>()
                    ->Version(1)
                    ->Field("ShaderInput", &UvNamePair::m_shaderInput)
                    ->Field("UvName", &UvNamePair::m_uvName)
                    ;
            }
        }

        void MaterialTypeAsset::Reflect(ReflectContext* context)
        {
            MaterialVersionUpdates::Reflect(context);
            UvNamePair::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<MaterialUvNameMap>();

                serializeContext->Class<MaterialTypeAsset, AZ::Data::AssetData>()
                    ->Version(5) // Material version update
                    ->Field("Version", &MaterialTypeAsset::m_version)
                    ->Field("VersionUpdates", &MaterialTypeAsset::m_materialVersionUpdates)
                    ->Field("ShaderCollection", &MaterialTypeAsset::m_shaderCollection)
                    ->Field("MaterialFunctors", &MaterialTypeAsset::m_materialFunctors)
                    ->Field("MaterialSrgShaderIndex", &MaterialTypeAsset::m_materialSrgShaderIndex)
                    ->Field("ObjectSrgShaderIndex", &MaterialTypeAsset::m_objectSrgShaderIndex)
                    ->Field("MaterialPropertiesLayout", &MaterialTypeAsset::m_materialPropertiesLayout)
                    ->Field("DefaultPropertyValues", &MaterialTypeAsset::m_propertyValues)
                    ->Field("UvNameMap", &MaterialTypeAsset::m_uvNameMap)
                    ;
            }

            ShaderCollection::Reflect(context);
        }

        MaterialTypeAsset::~MaterialTypeAsset()
        {
            Data::AssetBus::MultiHandler::BusDisconnect();
            AssetInitBus::Handler::BusDisconnect();
        }

        const ShaderCollection& MaterialTypeAsset::GetShaderCollection() const
        {
            return m_shaderCollection;
        }

        const MaterialFunctorList& MaterialTypeAsset::GetMaterialFunctors() const
        {
            return m_materialFunctors;
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetSrgLayout(
            uint32_t shaderIndex, const SupervariantIndex& supervariantIndex, uint32_t srgBindingSlot) const
        {
            const bool validShaderIndex = (m_shaderCollection.size() > shaderIndex);
            if (!validShaderIndex)
            {
                return RHI::NullSrgLayout;
            }
            const auto& shaderAsset = m_shaderCollection[shaderIndex].GetShaderAsset();
            return shaderAsset->FindShaderResourceGroupLayout(srgBindingSlot, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetSrgLayout(
            uint32_t shaderIndex, const AZ::Name& supervariantName, uint32_t srgBindingSlot) const
        {
            const bool validShaderIndex = (m_shaderCollection.size() > shaderIndex);
            if (!validShaderIndex)
            {
                return RHI::NullSrgLayout;
            }
            const auto& shaderAsset = m_shaderCollection[shaderIndex].GetShaderAsset();
            auto supervariantIndex = shaderAsset->GetSupervariantIndex(supervariantName);
            return shaderAsset->FindShaderResourceGroupLayout(srgBindingSlot, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetMaterialSrgLayout(
            const SupervariantIndex& supervariantIndex) const
        {
            return GetSrgLayout(m_materialSrgShaderIndex, supervariantIndex, RPI::SrgBindingSlot::Material);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetMaterialSrgLayout(const AZ::Name& supervariantName) const
        {
            return GetSrgLayout(m_materialSrgShaderIndex, supervariantName, RPI::SrgBindingSlot::Material);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetMaterialSrgLayout() const
        {
            return GetMaterialSrgLayout(DefaultSupervariantIndex);
        }

        const Data::Asset<ShaderAsset>& MaterialTypeAsset::GetShaderAssetForMaterialSrg() const
        {
            AZ_Assert(m_shaderCollection.size() > m_materialSrgShaderIndex, "shaderIndex %u is invalid because there are only %zu shaders in the collection", m_materialSrgShaderIndex,
                m_shaderCollection.size());
            return m_shaderCollection[m_materialSrgShaderIndex].GetShaderAsset();
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetObjectSrgLayout(
            const SupervariantIndex& supervariantIndex) const
        {
            return GetSrgLayout(m_objectSrgShaderIndex, supervariantIndex, RPI::SrgBindingSlot::Object);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetObjectSrgLayout(const AZ::Name& supervariantName) const
        {
            return GetSrgLayout(m_objectSrgShaderIndex, supervariantName, RPI::SrgBindingSlot::Object);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetObjectSrgLayout() const
        {
            return GetObjectSrgLayout(DefaultSupervariantIndex);
        }

        const Data::Asset<ShaderAsset>& MaterialTypeAsset::GetShaderAssetForObjectSrg() const
        {
            AZ_Assert(m_shaderCollection.size() > m_objectSrgShaderIndex,
                "shaderIndex %u is invalid because there are only %zu shaders in the collection", m_objectSrgShaderIndex,
                m_shaderCollection.size());
            return m_shaderCollection[m_objectSrgShaderIndex].GetShaderAsset();
        }

        const MaterialPropertiesLayout* MaterialTypeAsset::GetMaterialPropertiesLayout() const
        {
            return m_materialPropertiesLayout.get();
        }

        AZStd::span<const MaterialPropertyValue> MaterialTypeAsset::GetDefaultPropertyValues() const
        {
            return m_propertyValues;
        }

        MaterialUvNameMap MaterialTypeAsset::GetUvNameMap() const
        {
            return m_uvNameMap;
        }

        uint32_t MaterialTypeAsset::GetVersion() const
        {
            return m_version;
        }


        bool MaterialTypeAsset::ApplyPropertyRenames(AZ::Name& propertyId) const
        {
            return m_materialVersionUpdates.ApplyPropertyRenames(propertyId);
        }

        void MaterialTypeAsset::SetReady()
        {
            m_status = AssetStatus::Ready;

            // If this was created dynamically using MaterialTypeAssetCreator (which is what calls SetReady()),
            // we need to connect to the AssetBus for reloads.
            PostLoadInit();
        }

        bool MaterialTypeAsset::PostLoadInit()
        {
            for (const auto& shaderItem : m_shaderCollection)
            {
                Data::AssetBus::MultiHandler::BusConnect(shaderItem.GetShaderAsset().GetId());
            }
            AssetInitBus::Handler::BusDisconnect();

            return true;
        }

        template<typename AssetDataT>
        void TryReplaceAsset(Data::Asset<AssetDataT>& assetToReplace, const Data::Asset<Data::AssetData>& newAsset)
        {
            if (assetToReplace.GetId() == newAsset.GetId())
            {
                assetToReplace = newAsset;
            }
        }
        
        void MaterialTypeAsset::ReinitializeAsset(Data::Asset<Data::AssetData> asset)
        {
            // The order of asset reloads is non-deterministic. If the MaterialTypeAsset reloads before these
            // dependency assets, this will make sure the MaterialTypeAsset gets the latest ones when they reload.
            // Or in some cases a these assets could get updated and reloaded without reloading the MaterialTypeAsset at all.
            for (auto& shaderItem : m_shaderCollection)
            {
                TryReplaceAsset(shaderItem.m_shaderAsset, asset);
            }
        }

        void MaterialTypeAsset::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->MaterialTypeAsset::OnAssetReloaded %s", this, asset.GetHint().c_str());
            ReinitializeAsset(asset);
        }
        
        void MaterialTypeAsset::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            // Regarding why we listen to both OnAssetReloaded and OnAssetReady, see explanation in ShaderAsset::OnAssetReady.
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->MaterialTypeAsset::OnAssetReady %s", this, asset.GetHint().c_str());
            ReinitializeAsset(asset);
        }

        AZ::Data::AssetHandler::LoadResult MaterialTypeAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            if (Base::LoadAssetData(asset, stream, assetLoadFilterCB) == Data::AssetHandler::LoadResult::LoadComplete)
            {
                asset.GetAs<MaterialTypeAsset>()->AssetInitBus::Handler::BusConnect();
                return Data::AssetHandler::LoadResult::LoadComplete;
            }

            return Data::AssetHandler::LoadResult::Error;
        }

    }
}
