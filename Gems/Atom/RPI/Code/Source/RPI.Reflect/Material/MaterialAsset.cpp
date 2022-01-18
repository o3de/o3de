/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertiesLayout.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RPI.Reflect/Material/MaterialVersionUpdate.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Component/TickBus.h>

namespace AZ
{
    namespace RPI
    {
        const char* MaterialAsset::s_debugTraceName = "MaterialAsset";

        const char* MaterialAsset::DisplayName = "MaterialAsset";
        const char* MaterialAsset::Group = "Material";
        const char* MaterialAsset::Extension = "azmaterial";

        void MaterialAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MaterialAsset, AZ::Data::AssetData>()
                    ->Version(13) // added m_rawPropertyValues
                    ->Field("materialTypeAsset", &MaterialAsset::m_materialTypeAsset)
                    ->Field("materialTypeVersion", &MaterialAsset::m_materialTypeVersion)
                    ->Field("propertyValues", &MaterialAsset::m_propertyValues)
                    ->Field("rawPropertyValues", &MaterialAsset::m_rawPropertyValues)
                    ->Field("isFinalized", &MaterialAsset::m_isFinalized)
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
            AssetInitBus::Handler::BusDisconnect();
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

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialAsset::GetMaterialSrgLayout(const SupervariantIndex& supervariantIndex) const
        {
            return m_materialTypeAsset->GetMaterialSrgLayout(supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialAsset::GetMaterialSrgLayout(const AZ::Name& supervariantName) const
        {
            return m_materialTypeAsset->GetMaterialSrgLayout(supervariantName);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialAsset::GetMaterialSrgLayout() const
        {
            return m_materialTypeAsset->GetMaterialSrgLayout();
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialAsset::GetObjectSrgLayout(const SupervariantIndex& supervariantIndex) const
        {
            return m_materialTypeAsset->GetObjectSrgLayout(supervariantIndex);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialAsset::GetObjectSrgLayout(const AZ::Name& supervariantName) const
        {
            return m_materialTypeAsset->GetObjectSrgLayout(supervariantName);
        }

        const RHI::Ptr<RHI::ShaderResourceGroupLayout>& MaterialAsset::GetObjectSrgLayout() const
        {
            return m_materialTypeAsset->GetObjectSrgLayout();
        }

        const MaterialPropertiesLayout* MaterialAsset::GetMaterialPropertiesLayout() const
        {
            return m_materialTypeAsset->GetMaterialPropertiesLayout();
        }
        
        bool MaterialAsset::IsFinalized() const
        {
            if (m_isFinalized)
            {
                AZ_Assert(GetMaterialPropertiesLayout() && m_propertyValues.size() == GetMaterialPropertiesLayout()->GetPropertyCount(), "MaterialAsset is marked as Finalized but does not have the right number of property values.");
            }

            return m_isFinalized;
        }

        void MaterialAsset::Finalize(AZStd::function<void(const char*)> reportWarning, AZStd::function<void(const char*)> reportError)
        {
            if (IsFinalized())
            {
                return;
            }

            if (!reportWarning)
            {
                reportWarning = []([[maybe_unused]] const char* message)
                {
                    AZ_Warning(s_debugTraceName, false, "%s", message);
                };
            }
            
            if (!reportError)
            {
                reportError = []([[maybe_unused]] const char* message)
                {
                    AZ_Error(s_debugTraceName, false, "%s", message);
                };
            }

            const uint32_t materialTypeVersion = m_materialTypeAsset->GetVersion();
            if (m_materialTypeVersion < materialTypeVersion)
            {
                // It is possible that the material type has had some properties renamed or otherwise updated. If that's the case,
                // and this material is still referencing the old property layout, we need to apply any auto updates to rename those
                // properties before using them to realign the property values.
                ApplyVersionUpdates();
            }
            
            const MaterialPropertiesLayout* propertyLayout = GetMaterialPropertiesLayout();

            AZStd::vector<MaterialPropertyValue> finalizedPropertyValues(m_materialTypeAsset->GetDefaultPropertyValues().begin(), m_materialTypeAsset->GetDefaultPropertyValues().end());

            for (const auto& [name, value] : m_rawPropertyValues)
            {
                const MaterialPropertyIndex propertyIndex = propertyLayout->FindPropertyIndex(name);
                if (propertyIndex.IsValid())
                {
                    const MaterialPropertyDescriptor* propertyDescriptor = propertyLayout->GetPropertyDescriptor(propertyIndex);

                    if (value.Is<AZStd::string>() && propertyDescriptor->GetDataType() == MaterialPropertyDataType::Enum)
                    {
                        AZ::Name enumName = AZ::Name(value.GetValue<AZStd::string>());
                        uint32_t enumValue = propertyDescriptor->GetEnumValue(enumName);
                        if (enumValue == MaterialPropertyDescriptor::InvalidEnumValue)
                        {
                            reportWarning(AZStd::string::format("Material property name \"%s\" has invalid enum value \"%s\".", name.GetCStr(), enumName.GetCStr()).c_str());
                        }
                        else
                        {
                            finalizedPropertyValues[propertyIndex.GetIndex()] = enumValue;
                        }
                    }
                    else if (value.Is<AZStd::string>() && propertyDescriptor->GetDataType() == MaterialPropertyDataType::Image)
                    {
                        // Here we assume that the material asset builder resolved any image source file paths to an ImageAsset reference.
                        // So the only way a string could be present is if it's an empty image path reference, meaning no image should be bound.
                        AZ_Assert(value.GetValue<AZStd::string>().empty(), "Material property '%s' references in image '%s'. Image file paths must be resolved by the material asset builder.");

                        finalizedPropertyValues[propertyIndex.GetIndex()] = Data::Asset<ImageAsset>{};
                    }
                    else
                    {
                        if (ValidateMaterialPropertyDataType(value.GetTypeId(), name, propertyDescriptor, reportError))
                        {
                            finalizedPropertyValues[propertyIndex.GetIndex()] = value;
                        }
                    }
                }
                else
                {
                    reportWarning(AZStd::string::format("Material property name \"%s\" is not found in the material properties layout and will not be used.", name.GetCStr()).c_str());
                }
            }

            m_propertyValues.swap(finalizedPropertyValues);

            m_isFinalized = true;
        }

        const AZStd::vector<MaterialPropertyValue>& MaterialAsset::GetPropertyValues() const
        {
            AZ_Error(s_debugTraceName, IsFinalized(), "MaterialAsset must be finalized before its property values can be accessed");

            return m_propertyValues;
        }
        
        const AZStd::vector<AZStd::pair<Name, MaterialPropertyValue>>& MaterialAsset::GetRawPropertyValues() const
        {
            return m_rawPropertyValues;
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
                AssetInitBus::Handler::BusDisconnect();

                // Any MaterialAsset with invalid MaterialTypeAsset is not a successfully-loaded asset.
                return false;
            }
            else
            {
                Data::AssetBus::Handler::BusConnect(m_materialTypeAsset.GetId());
                MaterialReloadNotificationBus::Handler::BusConnect(m_materialTypeAsset.GetId());
                
                AssetInitBus::Handler::BusDisconnect();

                return true;
            }
        }

        void MaterialAsset::OnMaterialTypeAssetReinitialized(const Data::Asset<MaterialTypeAsset>& materialTypeAsset)
        {
            // When reloads occur, it's possible for old Asset objects to hang around and report reinitialization,
            // so we can reduce unnecessary reinitialization in that case.
            if (materialTypeAsset.Get() == m_materialTypeAsset.Get())
            {
                ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->MaterialAsset::OnMaterialTypeAssetReinitialized %s", this, materialTypeAsset.GetHint().c_str());

                // MaterialAsset doesn't need to reinitialize any of its own data when MaterialTypeAsset reinitializes,
                // because all it depends on is the MaterialTypeAsset reference, rather than the data inside it.
                // Ultimately it's the Material that cares about these changes, so we just forward any signal we get.
                MaterialReloadNotificationBus::Event(GetId(), &MaterialReloadNotifications::OnMaterialAssetReinitialized, Data::Asset<MaterialAsset>{this, AZ::Data::AssetLoadBehavior::PreLoad});
            }
        }
        
        void MaterialAsset::ApplyVersionUpdates()
        {
            if (m_materialTypeVersion == m_materialTypeAsset->GetVersion())
            {
                return;
            }

            [[maybe_unused]] const uint32_t originalVersion = m_materialTypeVersion;

            bool changesWereApplied = false;

            for (const MaterialVersionUpdate& versionUpdate : m_materialTypeAsset->GetMaterialVersionUpdateList())
            {
                if (m_materialTypeVersion < versionUpdate.GetVersion())
                {
                    if (versionUpdate.ApplyVersionUpdates(*this))
                    {
                        changesWereApplied = true;
                        m_materialTypeVersion = versionUpdate.GetVersion();
                    }
                }
            }
            
            if (changesWereApplied)
            {
                AZ_Warning(
                    "MaterialAsset", false,
                    "This material is based on version '%u' of %s, but the material type is now at version '%u'. "
                    "Automatic updates are available. Consider updating the .material source file for '%s'.",
                    originalVersion, m_materialTypeAsset.ToString<AZStd::string>().c_str(), m_materialTypeAsset->GetVersion(),
                    GetId().ToString<AZStd::string>().c_str());
            }

            m_materialTypeVersion = m_materialTypeAsset->GetVersion();
        }

        void MaterialAsset::ReinitializeMaterialTypeAsset(Data::Asset<Data::AssetData> asset)
        {
            Data::Asset<MaterialTypeAsset> newMaterialTypeAsset = Data::static_pointer_cast<MaterialTypeAsset>(asset);

            if (newMaterialTypeAsset)
            {
                // The order of asset reloads is non-deterministic. If the MaterialAsset reloads before the
                // MaterialTypeAsset, this will make sure the MaterialAsset gets update with latest one.
                // This also covers the case where just the MaterialTypeAsset is reloaded and not the MaterialAsset.
                m_materialTypeAsset = newMaterialTypeAsset;

                // If the material asset was not finalized on disk, then we clear the previously finalized property values to force re-finalize.
                // This is necessary in case the property layout changed in some way.
                if (!m_wasPreFinalized)
                {
                    m_isFinalized = false;
                    m_propertyValues.clear();
                }

                // Notify interested parties that this MaterialAsset is changed and may require other data to reinitialize as well
                MaterialReloadNotificationBus::Event(GetId(), &MaterialReloadNotifications::OnMaterialAssetReinitialized, Data::Asset<MaterialAsset>{this, AZ::Data::AssetLoadBehavior::PreLoad});
            }
        }

        void MaterialAsset::OnAssetReloaded(Data::Asset<Data::AssetData> asset)
        {
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->MaterialAsset::OnAssetReloaded %s", this, asset.GetHint().c_str());
            ReinitializeMaterialTypeAsset(asset);
        }

        void MaterialAsset::OnAssetReady(Data::Asset<Data::AssetData> asset)
        {
            // Regarding why we listen to both OnAssetReloaded and OnAssetReady, see explanation in ShaderAsset::OnAssetReady.
            ShaderReloadDebugTracker::ScopedSection reloadSection("{%p}->MaterialAsset::OnAssetReady %s", this, asset.GetHint().c_str());
            ReinitializeMaterialTypeAsset(asset);
        }

        Data::AssetHandler::LoadResult MaterialAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            if (Base::LoadAssetData(asset, stream, assetLoadFilterCB) == Data::AssetHandler::LoadResult::LoadComplete)
            {
                asset.GetAs<MaterialAsset>()->AssetInitBus::Handler::BusConnect();
                asset.GetAs<MaterialAsset>()->m_wasPreFinalized = asset.GetAs<MaterialAsset>()->m_isFinalized;
                return Data::AssetHandler::LoadResult::LoadComplete;
            }

            return Data::AssetHandler::LoadResult::Error;
        }

    } // namespace RPI
} // namespace AZ
