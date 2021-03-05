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

#include "MaterialConverterSystemComponent.h"

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AZ
{
    namespace Render
    {
        void MaterialConverterSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<MaterialConverterSystemComponent, Component>()
                    ->Version(2)
                    ->Attribute(Edit::Attributes::SystemComponentTags, AZStd::vector<Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
            }
        }

        void MaterialConverterSystemComponent::Activate()
        {
            RPI::MaterialConverterBus::Handler::BusConnect();
        }

        void MaterialConverterSystemComponent::Deactivate()
        {
            RPI::MaterialConverterBus::Handler::BusDisconnect();
        }

        bool MaterialConverterSystemComponent::ConvertMaterial(const AZ::SceneAPI::DataTypes::IMaterialData& materialData, RPI::MaterialSourceData& sourceData)
        {
            using namespace AZ::RPI;

            // The source data for generating material asset
            sourceData.m_materialType = GetMaterialTypePath();

            auto handleTexture = [&materialData, &sourceData](const char* propertyTextureGroup, SceneAPI::DataTypes::IMaterialData::TextureMapType textureType)
            {
                MaterialSourceData::PropertyMap& properties = sourceData.m_properties[propertyTextureGroup];
                const AZStd::string& texturePath = materialData.GetTexture(textureType);

                // Check to see if the image asset exists. If not, skip this texture map and just disable it.
                bool assetFound = false;
                if (!texturePath.empty())
                {
                    using namespace AzToolsFramework;
                    AZ::Data::AssetInfo sourceInfo;
                    AZStd::string watchFolder;
                    AssetSystemRequestBus::BroadcastResult(assetFound, &AssetSystem::AssetSystemRequest::GetSourceInfoBySourcePath, texturePath.c_str(), sourceInfo, watchFolder);
                }

                if (assetFound)
                {
                    properties["textureMap"].m_value = texturePath;
                }
                else if(!texturePath.empty())
                {
                    AZ_Warning("AtomFeatureCommon", false, "Could not find asset '%s' for '%s'", texturePath.c_str(), propertyTextureGroup);
                }
            };

            handleTexture("specularF0", SceneAPI::DataTypes::IMaterialData::TextureMapType::Specular);
            handleTexture("normal", SceneAPI::DataTypes::IMaterialData::TextureMapType::Normal);
            handleTexture("baseColor", SceneAPI::DataTypes::IMaterialData::TextureMapType::Diffuse);

            auto toColor = [](const AZ::Vector3& v) { return AZ::Color::CreateFromVector3AndFloat(v, 1.0f); };
            sourceData.m_properties["baseColor"]["color"].m_value = toColor(materialData.GetDiffuseColor());

            sourceData.m_properties["opacity"]["factor"].m_value = materialData.GetOpacity();

            return true;
        }

        const char* MaterialConverterSystemComponent::GetMaterialTypePath() const
        {
            return "Materials/Types/StandardPBR.materialtype";
        }
    }
}
