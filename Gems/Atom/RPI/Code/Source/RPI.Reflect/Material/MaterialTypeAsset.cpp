
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
                    ->Version(3)
                    ->Field("ShaderCollection", &MaterialTypeAsset::m_shaderCollection)
                    ->Field("MaterialFunctors", &MaterialTypeAsset::m_materialFunctors)
                    ->Field("MaterialSrgAsset", &MaterialTypeAsset::m_materialSrgAsset)
                    ->Field("ObjectSrgAsset", &MaterialTypeAsset::m_objectSrgAsset)
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

        const AZ::Data::Asset<ShaderResourceGroupAsset>& MaterialTypeAsset::GetMaterialSrgAsset() const
        {
            return m_materialSrgAsset;
        }

        const AZ::Data::Asset<ShaderResourceGroupAsset>& MaterialTypeAsset::GetObjectSrgAsset() const
        {
            return m_objectSrgAsset;
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
            Data::AssetBus::MultiHandler::BusConnect(m_materialSrgAsset.GetId());
            Data::AssetBus::MultiHandler::BusConnect(m_objectSrgAsset.GetId());
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
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->MaterialTypeAsset::OnAssetReloaded %s", this, asset.GetHint().c_str());

            // The order of asset reloads is non-deterministic. If the MaterialTypeAsset reloads before these
            // dependency assets, this will make sure the MaterialTypeAsset gets the latest ones when they reload.
            // Or in some cases a these assets could get updated and reloaded without reloading the MaterialTypeAsset at all.
            TryReplaceAsset(m_materialSrgAsset, asset);
            TryReplaceAsset(m_objectSrgAsset, asset);
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
