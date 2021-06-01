
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
#include <Atom/RPI.Reflect/Material/MaterialTypeAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Public/Material/MaterialReloadNotificationBus.h>
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
            UvNamePair::Reflect(context);

            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->RegisterGenericType<MaterialUvNameMap>();

                serializeContext->Class<MaterialTypeAsset, AZ::Data::AssetData>()
                    ->Version(4)
                    ->Field("ShaderCollection", &MaterialTypeAsset::m_shaderCollection)
                    ->Field("MaterialFunctors", &MaterialTypeAsset::m_materialFunctors)
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
        }

        const ShaderCollection& MaterialTypeAsset::GetShaderCollection() const
        {
            return m_shaderCollection;
        }

        const MaterialFunctorList& MaterialTypeAsset::GetMaterialFunctors() const
        {
            return m_materialFunctors;
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> MaterialTypeAsset::GetSrgLayout(
            uint32_t srgBindingSlot, const SupervariantIndex& supervariantIndex) const
        {
            AZ_Assert(m_shaderCollection.size(), "Need at least one shader in the collection");
            const auto& shaderAsset = m_shaderCollection[0].GetShaderAsset();
            return shaderAsset->FindShaderResourceGroupLayout(srgBindingSlot, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> MaterialTypeAsset::GetSrgLayout(
            uint32_t srgBindingSlot, const AZ::Name& supervariantName) const
        {
            AZ_Assert(m_shaderCollection.size(), "Need at least one shader in the collection");
            const auto& shaderAsset = m_shaderCollection[0].GetShaderAsset();
            auto supervariantIndex = shaderAsset->GetSupervariantIndex(supervariantName);
            return shaderAsset->FindShaderResourceGroupLayout(srgBindingSlot, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> MaterialTypeAsset::GetMaterialSrgLayout(
            const SupervariantIndex& supervariantIndex) const
        {
            return GetSrgLayout(RPI::SrgBindingSlot::Material, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> MaterialTypeAsset::GetMaterialSrgLayout(const AZ::Name& supervariantName) const
        {
            return GetSrgLayout(RPI::SrgBindingSlot::Material, supervariantName);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> MaterialTypeAsset::GetMaterialSrgLayout() const
        {
            return GetMaterialSrgLayout(DefaultSupervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> MaterialTypeAsset::GetObjectSrgLayout(
            const SupervariantIndex& supervariantIndex) const
        {
            return GetSrgLayout(RPI::SrgBindingSlot::Object, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> MaterialTypeAsset::GetObjectSrgLayout(const AZ::Name& supervariantName) const
        {
            return GetSrgLayout(RPI::SrgBindingSlot::Object, supervariantName);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout> MaterialTypeAsset::GetObjectSrgLayout() const
        {
            return GetObjectSrgLayout(DefaultSupervariantIndex);
        }

        const MaterialPropertiesLayout* MaterialTypeAsset::GetMaterialPropertiesLayout() const
        {
            return m_materialPropertiesLayout.get();
        }

        AZStd::array_view<MaterialPropertyValue> MaterialTypeAsset::GetDefaultPropertyValues() const
        {
            return m_propertyValues;
        }

        MaterialUvNameMap MaterialTypeAsset::GetUvNameMap() const
        {
            return m_uvNameMap;
        }

        void MaterialTypeAsset::SetReady()
        {
            m_status = AssetStatus::Ready;
        }

        bool MaterialTypeAsset::PostLoadInit()
        {
            for (const auto& shaderItem : m_shaderCollection)
            {
                Data::AssetBus::MultiHandler::BusConnect(shaderItem.GetShaderAsset().GetId());
            }

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

        void MaterialTypeAsset::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("MaterialTypeAsset::OnAssetReloaded %s", asset.GetHint().c_str());

            // The order of asset reloads is non-deterministic. If the MaterialTypeAsset reloads before these
            // dependency assets, this will make sure the MaterialTypeAsset gets the latest ones when they reload.
            // Or in some cases a these assets could get updated and reloaded without reloading the MaterialTypeAsset at all.
            for (auto& shaderItem : m_shaderCollection)
            {
                TryReplaceAsset(shaderItem.m_shaderAsset, asset);
            }

            // Notify interested parties that this MaterialTypeAsset is changed and may require other data to reinitialize as well
            MaterialReloadNotificationBus::Event(GetId(), &MaterialReloadNotifications::OnMaterialTypeAssetReinitialized, Data::Asset<MaterialTypeAsset>{this, AZ::Data::AssetLoadBehavior::PreLoad});
        }

        AZ::Data::AssetHandler::LoadResult MaterialTypeAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            Data::AssetHandler::LoadResult baseResult = Base::LoadAssetData(asset, stream, assetLoadFilterCB);
            bool postLoadResult = asset.GetAs<MaterialTypeAsset>()->PostLoadInit();
            return ((baseResult == Data::AssetHandler::LoadResult::LoadComplete) && postLoadResult) ?
                Data::AssetHandler::LoadResult::LoadComplete :
                Data::AssetHandler::LoadResult::Error;
        }

    }
}
