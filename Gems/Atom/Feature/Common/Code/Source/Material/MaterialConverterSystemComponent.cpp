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
#include <Atom/RPI.Edit/Material/MaterialUtils.h>

namespace AZ
{
    namespace Render
    {
        void MaterialConverterSettings::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MaterialConverterSettings>()
                    // If new settings are added here, be sure to update MaterialConverterSystemComponent::GetFingerprintInfo() as well.
                    ->Version(2)
                    ->Field("Enable", &MaterialConverterSettings::m_enable)
                    ->Field("DefaultMaterial", &MaterialConverterSettings::m_defaultMaterial);
            }
        }

        void MaterialConverterSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<MaterialConverterSystemComponent, Component>()
                    // If changes are made to the material conversion process, update the version number in
                    // MaterialConverterSystemComponent::GetFingerprintInfo(), not this one.
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
        
        AZStd::string MaterialConverterSystemComponent::GetFingerprintInfo() const
        {
            static constexpr int Version = 2; // Bump this version whenever changes are made to the material conversion code to force the AP to reprocess scene files

            AZStd::string fingerprintInfo = AZStd::string::format("[MaterialConverter version=%d enabled=%d", Version, IsEnabled());
             
            if (!IsEnabled())
            {
                fingerprintInfo += AZStd::string::format(" defaultMaterial=%s", GetDefaultMaterialPath().c_str());
            }

            fingerprintInfo += "]";


            return fingerprintInfo;
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

            auto handleTexture = [&materialData, &sourceData](const char* propertyTextureGroup, const char* propertyTextureName, SceneAPI::DataTypes::IMaterialData::TextureMapType textureType)
            {
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
                    sourceData.SetPropertyValue(MaterialPropertyId{propertyTextureGroup, propertyTextureName}, texturePath);
                }
                else if (!texturePath.empty())
                {
                    AZ_Warning("AtomFeatureCommon", false, "Could not find asset '%s' for '%s'", texturePath.c_str(), propertyTextureGroup);
                }
            };

            // If PBR material properties aren't in use, fall back to legacy properties. Don't do that if some PBR material properties are set, though.
            bool anyPBRInUse = false;

            handleTexture("specularF0", "textureMap", SceneAPI::DataTypes::IMaterialData::TextureMapType::Specular);
            handleTexture("normal", "textureMap", SceneAPI::DataTypes::IMaterialData::TextureMapType::Normal);
            AZStd::optional<bool> useColorMap = materialData.GetUseColorMap();
            // If the useColorMap property exists, this is a PBR material and the color should be set to baseColor.
            if (useColorMap.has_value())
            {
                anyPBRInUse = true;
                handleTexture("baseColor", "textureMap", SceneAPI::DataTypes::IMaterialData::TextureMapType::BaseColor);
                sourceData.SetPropertyValue(Name{"baseColor.textureBlendMode"}, AZStd::string("Lerp"));
            }
            else
            {
                // If it doesn't have the useColorMap property, then it's a non-PBR material and the baseColor
                // texture needs to be set to the diffuse texture.
                handleTexture("baseColor", "textureMap", SceneAPI::DataTypes::IMaterialData::TextureMapType::Diffuse);
            }

            auto toColor = [](const AZ::Vector3& v) { return AZ::Color::CreateFromVector3AndFloat(v, 1.0f); };

            AZStd::optional<AZ::Vector3> baseColor = materialData.GetBaseColor();
            if (baseColor.has_value())
            {
                anyPBRInUse = true;
                sourceData.SetPropertyValue(Name{"baseColor.color"}, toColor(baseColor.value()));
            }

            sourceData.SetPropertyValue(Name{"opacity.factor"}, materialData.GetOpacity());
            if (1.0f - materialData.GetOpacity() > AZ::Constants::FloatEpsilon)
            {
                sourceData.SetPropertyValue(Name{ "opacity.mode" }, AZStd::string("Blended"));
            }

            auto applyOptionalPropertiesFunc = [&sourceData, &anyPBRInUse](const auto& propertyGroup, const auto& propertyName, const auto& propertyOptional)
            {
                // Only set PBR settings if they were specifically set in the scene's data.
                // Otherwise, leave them unset so the data driven default properties are used.
                if (propertyOptional.has_value())
                {
                    anyPBRInUse = true;
                    sourceData.SetPropertyValue(MaterialPropertyId{propertyGroup, propertyName}, propertyOptional.value());
                }
            };

            handleTexture("metallic", "textureMap", SceneAPI::DataTypes::IMaterialData::TextureMapType::Metallic);
            applyOptionalPropertiesFunc("metallic", "factor", materialData.GetMetallicFactor());
            applyOptionalPropertiesFunc("metallic", "useTexture", materialData.GetUseMetallicMap());

            handleTexture("roughness", "textureMap", SceneAPI::DataTypes::IMaterialData::TextureMapType::Roughness);
            applyOptionalPropertiesFunc("roughness", "useTexture", materialData.GetUseRoughnessMap());
            // Both PBR material and non-PBR material can have the RoughnessFactor property
            AZStd::optional<float> roughness = materialData.GetRoughnessFactor();
            if (roughness.has_value())
            {
                sourceData.SetPropertyValue(Name{"roughness.factor"}, roughness.value());
            }
            else if (materialData.GetShininess() > AZ::Constants::FloatEpsilon)
            {
                // When the MaterialData provides Shininess instead of Roughness, it is necessary to convert Shininess to Roughness.
                // Normalized Blinn-Phong: D_p(m)=((alpha_p+2)/(2*PI))dot(n, m)^alpha_p, Usually use formula: alpha_p=2*alpha^-2 - 2,
                // alpha = roughness^2
                sourceData.SetPropertyValue(
                    Name{ "roughness.factor" }, aznumeric_cast<float>(pow(2.0f / (materialData.GetShininess() + 2.0f), 0.25)));
            }

            handleTexture("emissive", "textureMap", SceneAPI::DataTypes::IMaterialData::TextureMapType::Emissive);
            sourceData.SetPropertyValue(Name{"emissive.color"}, toColor(materialData.GetEmissiveColor()));
            applyOptionalPropertiesFunc("emissive", "intensity", materialData.GetEmissiveIntensity());
            applyOptionalPropertiesFunc("emissive", "useTexture", materialData.GetUseEmissiveMap());

            handleTexture("occlusion", "diffuseTextureMap", SceneAPI::DataTypes::IMaterialData::TextureMapType::AmbientOcclusion);
            applyOptionalPropertiesFunc("occlusion", "diffuseUseTexture", materialData.GetUseAOMap());

            if (!anyPBRInUse)
            {
                // If it doesn't have the useColorMap property, then it's a non-PBR material and the baseColor
                // texture needs to be set to the diffuse color.
                sourceData.SetPropertyValue(Name{"baseColor.color"}, toColor(materialData.GetDiffuseColor()));
            }
            return true;
        }

        AZStd::string MaterialConverterSystemComponent::GetMaterialTypePath() const
        {
            return AZ::RPI::MaterialUtils::PredictIntermediateMaterialTypeSourcePath("Materials/Types/StandardPBR.materialtype");
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
