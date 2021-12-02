/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneData/GraphData/MaterialData.h>

namespace AZ
{
    namespace SceneData
    {
        namespace GraphData
        {
            namespace DataTypes = AZ::SceneAPI::DataTypes;

            MaterialData::MaterialData()
                : m_isNoDraw(false)
                , m_diffuseColor(AZ::Vector3::CreateOne())
                , m_specularColor(AZ::Vector3::CreateZero())
                , m_emissiveColor(AZ::Vector3::CreateZero())
                , m_opacity(1.f)
                , m_shininess(10.f)
            {
            }

            void MaterialData::SetMaterialName(AZStd::string materialName)
            {
                m_materialName = AZStd::move(materialName);
            }

            const AZStd::string& MaterialData::GetMaterialName() const
            {
                return m_materialName;
            }

            void MaterialData::SetTexture(TextureMapType mapType, const char* textureFileName)
            {
                if (textureFileName)
                {
                    SetTexture(mapType, AZStd::string(textureFileName));
                }
            }

            void MaterialData::SetTexture(TextureMapType mapType, const AZStd::string& textureFileName)
            {
                SetTexture(mapType, AZStd::string(textureFileName));
            }

            void MaterialData::SetTexture(TextureMapType mapType, AZStd::string&& textureFileName)
            {
                if (!textureFileName.empty())
                {
                    m_textureMap[mapType] = AZStd::move(textureFileName);
                }
            }

            const AZStd::string& MaterialData::GetTexture(TextureMapType mapType) const
            {
                auto result = m_textureMap.find(mapType);
                if (result != m_textureMap.end())
                {
                    return result->second;
                }

                return m_emptyString;
            }

            void MaterialData::SetNoDraw(bool isNoDraw)
            {
                m_isNoDraw = isNoDraw;
            }

            bool MaterialData::IsNoDraw() const
            {
                return m_isNoDraw;
            }

            void MaterialData::SetDiffuseColor(const AZ::Vector3& color)
            {
                m_diffuseColor = color;
            }

            void MaterialData::SetUniqueId(uint64_t uid)
            {
                m_uniqueId = uid;
            }

            const AZ::Vector3& MaterialData:: GetDiffuseColor() const
            {
                return m_diffuseColor;
            }
            
            void MaterialData::SetSpecularColor(const AZ::Vector3& color)
            {
                m_specularColor = color;
            }

            const AZ::Vector3& MaterialData::GetSpecularColor() const
            {
                return m_specularColor;
            }

            void MaterialData::SetEmissiveColor(const AZ::Vector3& color)
            {
                m_emissiveColor = color;
            }

            const AZ::Vector3& MaterialData::GetEmissiveColor() const
            {
                return m_emissiveColor;
            }
            
            void MaterialData::SetOpacity(float opacity)
            {
                m_opacity = opacity;
            }

            float MaterialData::GetOpacity() const
            {
                return m_opacity;
            }
            
            void MaterialData::SetShininess(float shininess)
            {
                m_shininess = shininess;
            }

            float MaterialData::GetShininess() const
            {
                return m_shininess;
            }

            void MaterialData::SetUseColorMap(AZStd::optional<bool> useColorMap)
            {
                m_useColorMap = useColorMap;
            }

            AZStd::optional<bool> MaterialData::GetUseColorMap() const
            {
                return m_useColorMap;
            }

            void MaterialData::SetBaseColor(const AZStd::optional<AZ::Vector3>& baseColor)
            {
                m_baseColor = baseColor;
            }

            AZStd::optional<AZ::Vector3> MaterialData::GetBaseColor() const
            {
                return m_baseColor;
            }

            void MaterialData::SetUseMetallicMap(AZStd::optional<bool> useMetallicMap)
            {
                m_useMetallicMap = useMetallicMap;
            }

            AZStd::optional<bool> MaterialData::GetUseMetallicMap() const
            {
                return m_useMetallicMap;
            }

            void MaterialData::SetMetallicFactor(AZStd::optional<float> metallicFactor)
            {
                m_metallicFactor = metallicFactor;
            }

            AZStd::optional<float> MaterialData::GetMetallicFactor() const
            {
                return m_metallicFactor;
            }

            void MaterialData::SetUseRoughnessMap(AZStd::optional<bool> useRoughnessMap)
            {
                m_useRoughnessMap = useRoughnessMap;
            }

            AZStd::optional<bool> MaterialData::GetUseRoughnessMap() const
            {
                return m_useRoughnessMap;
            }

            void MaterialData::SetRoughnessFactor(AZStd::optional<float> roughnessFactor)
            {
                m_roughnessFactor = roughnessFactor;
            }

            AZStd::optional<float> MaterialData::GetRoughnessFactor() const
            {
                return m_roughnessFactor;
            }

            void MaterialData::SetUseEmissiveMap(AZStd::optional<bool> useEmissiveMap)
            {
                m_useEmissiveMap = useEmissiveMap;
            }

            AZStd::optional<bool> MaterialData::GetUseEmissiveMap() const
            {
                return m_useEmissiveMap;
            }

            void MaterialData::SetEmissiveIntensity(AZStd::optional<float> emissiveIntensity)
            {
                m_emissiveIntensity = emissiveIntensity;
            }

            AZStd::optional<float> MaterialData::GetEmissiveIntensity() const
            {
                return m_emissiveIntensity;
            }

            void MaterialData::SetUseAOMap(AZStd::optional<bool> useAOMap)
            {
                m_useAOMap = useAOMap;
            }

            AZStd::optional<bool> MaterialData::GetUseAOMap() const
            {
                return m_useAOMap;
            }

            uint64_t MaterialData::GetUniqueId() const
            {
                return m_uniqueId;
            }

            namespace Helper
            {
                template <typename T>
                T ReturnOptionalValue(AZStd::optional<T> value)
                {
                    if (!value)
                    {
                        return {};
                    }
                    return value.value();
                }
            }

            void MaterialData::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MaterialData>()->Version(4)
                        ->Field("textureMap", &MaterialData::m_textureMap)
                        ->Field("diffuseColor", &MaterialData::m_diffuseColor)
                        ->Field("specularColor", &MaterialData::m_specularColor)
                        ->Field("emissiveColor", &MaterialData::m_emissiveColor)
                        ->Field("opacity", &MaterialData::m_opacity)
                        ->Field("shininess", &MaterialData::m_shininess)
                        ->Field("noDraw", &MaterialData::m_isNoDraw)
                        ->Field("uniqueId", &MaterialData::m_uniqueId)
                        ->Field("useColorMap", &MaterialData::m_useColorMap)
                        ->Field("baseColor", &MaterialData::m_baseColor)
                        ->Field("useMetallicMap", &MaterialData::m_useMetallicMap)
                        ->Field("metallicFactor", &MaterialData::m_metallicFactor)
                        ->Field("useRoughnessMap", &MaterialData::m_useRoughnessMap)
                        ->Field("roughnessFactor", &MaterialData::m_roughnessFactor)
                        ->Field("useEmissiveMap", &MaterialData::m_useEmissiveMap)
                        ->Field("emissiveIntensity", &MaterialData::m_emissiveIntensity)
                        ->Field("useAOMap", &MaterialData::m_useAOMap);

                    EditContext* editContext = serializeContext->GetEditContext();
                    if (editContext)
                    {
                        editContext->Class<MaterialData>("Materials", "Material configuration for the parent.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_diffuseColor, "Diffuse", "Diffuse color component of the material.")
                            ->Attribute(Edit::Attributes::LabelForX, "R")
                            ->Attribute(Edit::Attributes::LabelForY, "G")
                            ->Attribute(Edit::Attributes::LabelForZ, "B")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_specularColor, "Specular", "Specular color component of the material.")
                            ->Attribute(Edit::Attributes::LabelForX, "R")
                            ->Attribute(Edit::Attributes::LabelForY, "G")
                            ->Attribute(Edit::Attributes::LabelForZ, "B")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_emissiveColor, "Emissive", "Emissive color component of the material.")
                            ->Attribute(Edit::Attributes::LabelForX, "R")
                            ->Attribute(Edit::Attributes::LabelForY, "G")
                            ->Attribute(Edit::Attributes::LabelForZ, "B")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_opacity, "Opacity", "Opacity strength of the material, with 0 fully transparent and 1 fully opaque.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_shininess, "Shininess", "The shininess strength of the material.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_isNoDraw, "No draw", "If enabled the mesh with material will not be drawn.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_textureMap, "Texture map", "List of assigned texture slots.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_useColorMap, "Use Color Map", "True to use a color map, false to ignore it.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_baseColor, "Base Color", "The base color of the material.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_useMetallicMap, "Use Metallic Map", "True to use a metallic map, false to ignore it.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_metallicFactor, "Metallic Factor", "How metallic the material is.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_useRoughnessMap, "Use Roughness Map", "True to use a roughness map, false to ignore it.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_roughnessFactor, "Roughness Factor", "How rough the material is.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_useEmissiveMap, "Use Emissive Map", "True to use an emissive map, false to ignore it.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_emissiveIntensity, "Emissive Intensity", "The intensity of the emissiveness of the material.")
                            ->DataElement(AZ::Edit::UIHandlers::Default, &MaterialData::m_useAOMap, "Use Ambient Occlusion Map", "True to use an ambient occlusion map, false to ignore it.");
                    }
                }

                BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->Class<SceneAPI::DataTypes::IMaterialData>()
                        ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene");

                    using namespace Helper;
                    using DataTypes::IMaterialData;

                    behaviorContext->Class<MaterialData>()
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                        ->Attribute(AZ::Script::Attributes::Module, "scene")
                        ->Constant("AmbientOcclusion", BehaviorConstant(TextureMapType::AmbientOcclusion))
                        ->Constant("BaseColor", BehaviorConstant(TextureMapType::BaseColor))
                        ->Constant("Bump", BehaviorConstant(TextureMapType::Bump))
                        ->Constant("Diffuse", BehaviorConstant(TextureMapType::Diffuse))
                        ->Constant("Emissive", BehaviorConstant(TextureMapType::Emissive))
                        ->Constant("Metallic", BehaviorConstant(TextureMapType::Metallic))
                        ->Constant("Normal", BehaviorConstant(TextureMapType::Normal))
                        ->Constant("Roughness", BehaviorConstant(TextureMapType::Roughness))
                        ->Constant("Specular", BehaviorConstant(TextureMapType::Specular))
                        ->Method("GetTexture", &MaterialData::GetTexture)
                        ->Method("GetMaterialName", &MaterialData::GetMaterialName)
                        ->Method("IsNoDraw", &MaterialData::IsNoDraw)
                        ->Method("GetDiffuseColor", &MaterialData::GetDiffuseColor)
                        ->Method("GetSpecularColor", &MaterialData::GetSpecularColor)
                        ->Method("GetEmissiveColor", &MaterialData::GetEmissiveColor)
                        ->Method("GetOpacity", &MaterialData::GetOpacity)
                        ->Method("GetUniqueId", &MaterialData::GetUniqueId)
                        ->Method("GetShininess", &MaterialData::GetShininess)
                        ->Method("GetUseColorMap", [](const MaterialData& self)
                        {
                            return ReturnOptionalValue(self.GetUseColorMap());
                        })
                        ->Method("GetBaseColor", [](const MaterialData& self)
                        {
                            return ReturnOptionalValue(self.GetBaseColor());
                        })
                        ->Method("GetUseMetallicMap", [](const MaterialData& self)
                        {
                            return ReturnOptionalValue(self.GetUseMetallicMap());
                        })
                        ->Method("GetMetallicFactor", [](const MaterialData& self)
                        {
                            return ReturnOptionalValue(self.GetMetallicFactor());
                        })
                        ->Method("GetUseRoughnessMap", [](const MaterialData& self)
                        {
                            return ReturnOptionalValue(self.GetUseRoughnessMap());
                        })
                        ->Method("GetRoughnessFactor", [](const MaterialData& self)
                        {
                            return ReturnOptionalValue(self.GetRoughnessFactor());
                        })
                        ->Method("GetUseEmissiveMap", [](const MaterialData& self)
                        {
                            return ReturnOptionalValue(self.GetUseEmissiveMap());
                        })
                        ->Method("GetEmissiveIntensity", [](const MaterialData& self)
                        {
                            return ReturnOptionalValue(self.GetEmissiveIntensity());
                        })
                        ->Method("GetUseAOMap", [](const MaterialData& self)
                        {
                            return ReturnOptionalValue(self.GetUseAOMap());
                        });
                }
            }
        } // namespace GraphData
    } // namespace SceneData
} // namespace AZ
