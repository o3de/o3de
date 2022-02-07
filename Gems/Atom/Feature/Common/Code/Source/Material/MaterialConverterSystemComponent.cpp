/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialConverterSystemComponent.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <Atom/RPI.Reflect/Material/MaterialAsset.h>

namespace AZ
{
    namespace Render
    {
        void MaterialConverterSettings::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MaterialConverterSettings>()
                    ->Version(2)
                    ->Field("Enable", &MaterialConverterSettings::m_enable)
                    ->Field("DefaultMaterial", &MaterialConverterSettings::m_defaultMaterial)
                    ->Field("IncludeMaterialPropertyNames", &MaterialConverterSettings::m_includeMaterialPropertyNames);
            }
        }

        void MaterialConverterSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<MaterialConverterSystemComponent, Component>()
                    ->Version(3)
                    ->Attribute(Edit::Attributes::SystemComponentTags, AZStd::vector<Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
            }

            MaterialConverterSettings::Reflect(context);
        }
        
        void MaterialConverterSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.emplace_back(AZ_CRC_CE("FingerprintModification"));
        }

        void MaterialConverterSystemComponent::Activate()
        {
            if (auto* settingsRegistry = AZ::SettingsRegistry::Get())
            {
                settingsRegistry->GetObject(m_settings, "/O3DE/SceneAPI/MaterialConverter");
            }

            RPI::MaterialConverterBus::Handler::BusConnect();
        }

        void MaterialConverterSystemComponent::Deactivate()
        {
            RPI::MaterialConverterBus::Handler::BusDisconnect();
        }
        
        bool MaterialConverterSystemComponent::IsEnabled() const
        {
            return m_settings.m_enable;
        }

        bool MaterialConverterSystemComponent::ShouldIncludeMaterialPropertyNames() const
        {
            return m_settings.m_includeMaterialPropertyNames;
        }

        bool MaterialConverterSystemComponent::ConvertMaterial(
            const AZ::SceneAPI::DataTypes::IMaterialData& materialData, RPI::MaterialSourceData& sourceData)
        {
            using namespace AZ::RPI;
            
            if (!m_settings.m_enable)
            {
                return false;
            }

            // The source data for generating material asset
            sourceData.m_materialType = GetMaterialTypePath();

            auto handleTexture = [&materialData, &sourceData](
                                     const char* propertyTextureGroup, SceneAPI::DataTypes::IMaterialData::TextureMapType textureType) {
                MaterialSourceData::PropertyMap& properties = sourceData.m_properties[propertyTextureGroup];
                const AZStd::string& texturePath = materialData.GetTexture(textureType);

                // Check to see if the image asset exists. If not, skip this texture map and just disable it.
                bool assetFound = false;
                if (!texturePath.empty())
                {
                    using namespace AzToolsFramework;
                    AZ::Data::AssetInfo sourceInfo;
                    AZStd::string watchFolder;
                    AssetSystemRequestBus::BroadcastResult(
                        assetFound, &AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, texturePath.c_str(), sourceInfo,
                        watchFolder);
                }

                if (assetFound)
                {
                    properties["textureMap"].m_value = texturePath;
                }
                else if (!texturePath.empty())
                {
                    AZ_Warning("AtomFeatureCommon", false, "Could not find asset '%s' for '%s'", texturePath.c_str(), propertyTextureGroup);
                }
            };

            // If PBR material properties aren't in use, fall back to legacy properties. Don't do that if some PBR material properties are set, though.
            bool anyPBRInUse = false;

            handleTexture("specularF0", SceneAPI::DataTypes::IMaterialData::TextureMapType::Specular);
            handleTexture("normal", SceneAPI::DataTypes::IMaterialData::TextureMapType::Normal);
            AZStd::optional<bool> useColorMap = materialData.GetUseColorMap();
            // If the useColorMap property exists, this is a PBR material and the color should be set to baseColor.
            if (useColorMap.has_value())
            {
                anyPBRInUse = true;
                handleTexture("baseColor", SceneAPI::DataTypes::IMaterialData::TextureMapType::BaseColor);
                sourceData.m_properties["baseColor"]["textureBlendMode"].m_value = AZStd::string("Lerp");
            }
            else
            {
                // If it doesn't have the useColorMap property, then it's a non-PBR material and the baseColor
                // texture needs to be set to the diffuse texture.
                handleTexture("baseColor", SceneAPI::DataTypes::IMaterialData::TextureMapType::Diffuse);
            }

            auto toColor = [](const AZ::Vector3& v) { return AZ::Color::CreateFromVector3AndFloat(v, 1.0f); };

            AZStd::optional<AZ::Vector3> baseColor = materialData.GetBaseColor();
            if (baseColor.has_value())
            {
                anyPBRInUse = true;
                sourceData.m_properties["baseColor"]["color"].m_value = toColor(baseColor.value());
            }

            sourceData.m_properties["opacity"]["factor"].m_value = materialData.GetOpacity();

            auto applyOptionalPropertiesFunc = [&sourceData, &anyPBRInUse](const auto& propertyGroup, const auto& propertyName, const auto& propertyOptional)
            {
                // Only set PBR settings if they were specifically set in the scene's data.
                // Otherwise, leave them unset so the data driven default properties are used.
                if (propertyOptional.has_value())
                {
                    anyPBRInUse = true;
                    sourceData.m_properties[propertyGroup][propertyName].m_value = propertyOptional.value();
                }
            };

            handleTexture("metallic", SceneAPI::DataTypes::IMaterialData::TextureMapType::Metallic);
            applyOptionalPropertiesFunc("metallic", "factor", materialData.GetMetallicFactor());
            applyOptionalPropertiesFunc("metallic", "useTexture", materialData.GetUseMetallicMap());

            handleTexture("roughness", SceneAPI::DataTypes::IMaterialData::TextureMapType::Roughness);
            applyOptionalPropertiesFunc("roughness", "factor", materialData.GetRoughnessFactor());
            applyOptionalPropertiesFunc("roughness", "useTexture", materialData.GetUseRoughnessMap());

            handleTexture("emissive", SceneAPI::DataTypes::IMaterialData::TextureMapType::Emissive);
            sourceData.m_properties["emissive"]["color"].m_value = toColor(materialData.GetEmissiveColor());
            applyOptionalPropertiesFunc("emissive", "intensity", materialData.GetEmissiveIntensity());
            applyOptionalPropertiesFunc("emissive", "useTexture", materialData.GetUseEmissiveMap());

            handleTexture("ambientOcclusion", SceneAPI::DataTypes::IMaterialData::TextureMapType::AmbientOcclusion);
            applyOptionalPropertiesFunc("ambientOcclusion", "useTexture", materialData.GetUseAOMap());

            if (!anyPBRInUse)
            {
                // If it doesn't have the useColorMap property, then it's a non-PBR material and the baseColor
                // texture needs to be set to the diffuse color.
                sourceData.m_properties["baseColor"]["color"].m_value = toColor(materialData.GetDiffuseColor());
            }
            return true;
        }

        AZStd::string MaterialConverterSystemComponent::GetMaterialTypePath() const
        {
            return "Materials/Types/StandardPBR.materialtype";
        }

        AZStd::string MaterialConverterSystemComponent::GetDefaultMaterialPath() const
        {
            if (m_settings.m_defaultMaterial.empty())
            {
                AZ_Error("MaterialConverterSystemComponent", m_settings.m_enable,
                    "Material conversion is disabled but a default material not specified in registry /O3DE/SceneAPI/MaterialConverter/DefaultMaterial");
            }

            return m_settings.m_defaultMaterial;
        }
    }
}
