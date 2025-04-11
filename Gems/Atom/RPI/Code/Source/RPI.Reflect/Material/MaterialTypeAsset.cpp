
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
#include <AzCore/Asset/AssetSerializer.h>

namespace AZ
{
    namespace RPI
    {
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

                serializeContext->Class<MaterialPipelinePayload>()
                    ->Version(1)
                    ->Field("MaterialPropertiesLayout", &MaterialPipelinePayload::m_materialPropertiesLayout)
                    ->Field("DefaultPropertyValues", &MaterialPipelinePayload::m_defaultPropertyValues)
                    ->Field("ShaderCollection", &MaterialPipelinePayload::m_shaderCollection)
                    ->Field("MaterialFunctors", &MaterialPipelinePayload::m_materialFunctors)
                    ;

                serializeContext->Class<MaterialTypeAsset, AZ::Data::AssetData>()
                    ->Version(9) // Material pipeline functor support
                    ->Field("Version", &MaterialTypeAsset::m_version)
                    ->Field("VersionUpdates", &MaterialTypeAsset::m_materialVersionUpdates)
                    ->Field("GeneralShaderCollection", &MaterialTypeAsset::m_generalShaderCollection)
                    ->Field("MaterialFunctors", &MaterialTypeAsset::m_materialFunctors)
                    ->Field("ShaderWithMaterialSrg", &MaterialTypeAsset::m_shaderWithMaterialSrg)
                    ->Field("ShaderWithObjectSrg", &MaterialTypeAsset::m_shaderWithObjectSrg)
                    ->Field("MaterialPropertiesLayout", &MaterialTypeAsset::m_materialPropertiesLayout)
                    ->Field("DefaultPropertyValues", &MaterialTypeAsset::m_propertyValues)
                    ->Field("MaterialPipelinePayloads", &MaterialTypeAsset::m_materialPipelinePayloads)
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

        bool MaterialTypeAsset::InitializeNonSerializedData()
        {
            if (m_isNonSerializedDataInitialized)
            {
                return true;
            }
            if (!m_generalShaderCollection.InitializeShaderOptionGroups())
            {
                return false;
            }
            for (auto& materialPipelinePair : m_materialPipelinePayloads)
            {
                if (!materialPipelinePair.second.m_shaderCollection.InitializeShaderOptionGroups())
                {
                    return false;
                }
            }
            m_isNonSerializedDataInitialized = true;
            return true;
        }

        const ShaderCollection& MaterialTypeAsset::GetGeneralShaderCollection() const
        {
            return m_generalShaderCollection;
        }

        const MaterialFunctorList& MaterialTypeAsset::GetMaterialFunctors() const
        {
            return m_materialFunctors;
        }

        const MaterialTypeAsset::MaterialPipelineMap& MaterialTypeAsset::GetMaterialPipelinePayloads() const
        {
            return m_materialPipelinePayloads;
        }

        void MaterialTypeAsset::ForAllShaderItems(AZStd::function<bool(const Name& materialPipelineName, ShaderCollection::Item& shaderItem, uint32_t shaderIndex)> callback)
        {
            for (int shaderIndex = 0; shaderIndex < m_generalShaderCollection.size(); ++shaderIndex)
            {
                ShaderCollection::Item& shaderItem = m_generalShaderCollection[shaderIndex];
                callback(MaterialPipelineNone, shaderItem, shaderIndex);
            }

            for (auto& [materialPipelineName, materialPipeline] : m_materialPipelinePayloads)
            {
                for (int shaderIndex = 0; shaderIndex < materialPipeline.m_shaderCollection.size(); ++shaderIndex)
                {
                    ShaderCollection::Item& shaderItem = materialPipeline.m_shaderCollection[shaderIndex];
                    callback(materialPipelineName, shaderItem, shaderIndex);
                }
            }
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetMaterialSrgLayout(
            const SupervariantIndex& supervariantIndex) const
        {
            if (!m_shaderWithMaterialSrg)
            {
                return RHI::NullSrgLayout;
            }

            return m_shaderWithMaterialSrg->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Material, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetMaterialSrgLayout(const AZ::Name& supervariantName) const
        {
            if (!m_shaderWithMaterialSrg)
            {
                return RHI::NullSrgLayout;
            }

            auto supervariantIndex = m_shaderWithMaterialSrg->GetSupervariantIndex(supervariantName);
            return m_shaderWithMaterialSrg->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Material, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetMaterialSrgLayout() const
        {
            return GetMaterialSrgLayout(DefaultSupervariantIndex);
        }

        const Data::Asset<ShaderAsset>& MaterialTypeAsset::GetShaderAssetForMaterialSrg() const
        {
            return m_shaderWithMaterialSrg;
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetObjectSrgLayout(
            const SupervariantIndex& supervariantIndex) const
        {
            if (!m_shaderWithObjectSrg)
            {
                return RHI::NullSrgLayout;
            }

            return m_shaderWithObjectSrg->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Object, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetObjectSrgLayout(const AZ::Name& supervariantName) const
        {
            if (!m_shaderWithObjectSrg)
            {
                return RHI::NullSrgLayout;
            }

            auto supervariantIndex = m_shaderWithObjectSrg->GetSupervariantIndex(supervariantName);
            return m_shaderWithObjectSrg->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Object, supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialTypeAsset::GetObjectSrgLayout() const
        {
            return GetObjectSrgLayout(DefaultSupervariantIndex);
        }

        const Data::Asset<ShaderAsset>& MaterialTypeAsset::GetShaderAssetForObjectSrg() const
        {
            return m_shaderWithObjectSrg;
        }

        const MaterialPropertiesLayout* MaterialTypeAsset::GetMaterialPropertiesLayout() const
        {
            return m_materialPropertiesLayout.get();
        }

        const AZStd::vector<MaterialPropertyValue>& MaterialTypeAsset::GetDefaultPropertyValues() const
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
            // Attempt to initialize non-serialized data. The referenced shader assets in the ShaderCollection
            // may not be ready right now, but in the future the system will retry when said assets
            // are ready.
            InitializeNonSerializedData();

            for (const auto& shaderItem : m_generalShaderCollection)
            {
                Data::AssetBus::MultiHandler::BusConnect(shaderItem.GetShaderAsset().GetId());
            }

            for (const auto& materialPipelinePair : m_materialPipelinePayloads)
            {
                for (const auto& shaderItem : materialPipelinePair.second.m_shaderCollection)
                {
                    Data::AssetBus::MultiHandler::BusConnect(shaderItem.GetShaderAsset().GetId());
                }
            }

            AssetInitBus::Handler::BusDisconnect();

            return true;
        }
        
        void MaterialTypeAsset::ReinitializeAsset(Data::Asset<Data::AssetData> asset)
        {
            // The order of asset reloads is non-deterministic. If the MaterialTypeAsset reloads before these
            // dependency assets, this will make sure the MaterialTypeAsset gets the latest ones when they reload.
            // Or in some cases a these assets could get updated and reloaded without reloading the MaterialTypeAsset at all.
            m_generalShaderCollection.TryReplaceShaderAsset(asset);

            for (auto& materialPipelinePair : m_materialPipelinePayloads)
            {
                materialPipelinePair.second.m_shaderCollection.TryReplaceShaderAsset(asset);
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
