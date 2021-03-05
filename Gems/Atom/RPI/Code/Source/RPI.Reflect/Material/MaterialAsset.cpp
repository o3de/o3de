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

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace AZ
{
    namespace RPI
    {
        const char* MaterialAsset::DisplayName = "MaterialAsset";
        const char* MaterialAsset::Group = "Material";
        const char* MaterialAsset::Extension = "azmaterial";

        void MaterialAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialAsset, AZ::Data::AssetData>()
                    ->Version(9)
                    ->Field("materialTypeAsset", &MaterialAsset::m_materialTypeAsset)
                    ->Field("propertyValues", &MaterialAsset::m_propertyValues)
                    ;
            }
        }

        MaterialAsset::MaterialAsset()
        {
        }

        MaterialAsset::~MaterialAsset()
        {
            MaterialReloadNotificationBus::Handler::BusDisconnect();
            Data::AssetBus::Handler::BusDisconnect();
        }

        const Data::Asset<MaterialTypeAsset>& MaterialAsset::GetMaterialTypeAsset() const
        {
            return m_materialTypeAsset;
        }

        const ShaderCollection& MaterialAsset::GetShaderCollection() const
        {
            return m_materialTypeAsset->GetShaderCollection();
        }

        const MaterialFunctorList& MaterialAsset::GetMaterialFunctors() const
        {
            return m_materialTypeAsset->GetMaterialFunctors();
        }

        const AZ::Data::Asset<ShaderResourceGroupAsset>& MaterialAsset::GetMaterialSrgAsset() const
        {
            return m_materialTypeAsset->GetMaterialSrgAsset();
        }

        const AZ::Data::Asset<ShaderResourceGroupAsset>& MaterialAsset::GetObjectSrgAsset() const
        {
            return m_materialTypeAsset->GetObjectSrgAsset();
        }

        const MaterialPropertiesLayout* MaterialAsset::GetMaterialPropertiesLayout() const
        {
            return m_materialTypeAsset->GetMaterialPropertiesLayout();
        }

        AZStd::array_view<MaterialPropertyValue> MaterialAsset::GetPropertyValues() const
        {
            return m_propertyValues;
        }

        void MaterialAsset::SetReady()
        {
            m_status = AssetStatus::Ready;

            // If this was created dynamically using MaterialAssetCreator (which is what calls SetReady()),
            // we need to connect to the AssetBus for reloads.
            PostLoadInit();
        }

        bool MaterialAsset::PostLoadInit()
        {
            if (!m_materialTypeAsset.Get())
            {
                // Any MaterialAsset with invalid MaterialTypeAsset is not a successfully-loaded asset.
                return false;
            }
            else
            {
                Data::AssetBus::Handler::BusConnect(m_materialTypeAsset.GetId());
                MaterialReloadNotificationBus::Handler::BusConnect(m_materialTypeAsset.GetId());

                return true;
            }
        }

        void MaterialAsset::OnMaterialTypeAssetReinitialized(const Data::Asset<MaterialTypeAsset>&)
        {
            // MaterialAsset doesn't need to reinitialize any of its own data when MaterialTypeAsset reinitializes,
            // because all it depends on is the MaterialTypeAsset reference, rather than the data inside it.
            // Ultimately it's the Material that cares about these changes, so we just forward any signal we get.
            MaterialReloadNotificationBus::Event(GetId(), &MaterialReloadNotifications::OnMaterialAssetReinitialized, Data::Asset<MaterialAsset>{this, AZ::Data::AssetLoadBehavior::PreLoad});
        }

        void MaterialAsset::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("MaterialAsset::OnAssetReloaded %s", asset.GetHint().c_str());

            Data::Asset<MaterialTypeAsset> newMaterialTypeAsset = { asset.GetAs<MaterialTypeAsset>(), AZ::Data::AssetLoadBehavior::PreLoad };

            if (newMaterialTypeAsset)
            {
                // The order of asset reloads is non-deterministic. If the MaterialAsset reloads before the
                // MaterialTypeAsset, this will make sure the MaterialAsset gets update with latest one.
                // This also covers the case where just the MaterialTypeAsset is reloaded and not the MaterialAsset.
                m_materialTypeAsset = newMaterialTypeAsset;

                // Notify interested parties that this MaterialAsset is changed and may require other data to reinitialize as well
                MaterialReloadNotificationBus::Event(GetId(), &MaterialReloadNotifications::OnMaterialAssetReinitialized, Data::Asset<MaterialAsset>{this, AZ::Data::AssetLoadBehavior::PreLoad});
            }
        }

        Data::AssetHandler::LoadResult MaterialAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            Data::AssetHandler::LoadResult baseResult = Base::LoadAssetData(asset, stream, assetLoadFilterCB);
            bool postLoadResult = asset.GetAs<MaterialAsset>()->PostLoadInit();
            return ((baseResult == Data::AssetHandler::LoadResult::LoadComplete) && postLoadResult) ?
                Data::AssetHandler::LoadResult::LoadComplete :
                Data::AssetHandler::LoadResult::Error;
        }

    } // namespace RPI
} // namespace AZ
