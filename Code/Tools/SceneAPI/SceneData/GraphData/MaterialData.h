/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/fixed_string.h>

namespace AZ
{
    class ReflectContext;

    namespace SceneData
    {
        namespace GraphData
        {
            class SCENE_DATA_CLASS MaterialData
                : public AZ::SceneAPI::DataTypes::IMaterialData
            {
            public:
                AZ_RTTI(MaterialData, "{F2EE1768-183B-483E-9778-CB3D3D0DA68A}", AZ::SceneAPI::DataTypes::IMaterialData)
                
                SCENE_DATA_API MaterialData();
                SCENE_DATA_API virtual ~MaterialData() = default;

                SCENE_DATA_API void SetMaterialName(AZStd::string materialName);
                SCENE_DATA_API const AZStd::string& GetMaterialName() const override;

                SCENE_DATA_API virtual void SetTexture(TextureMapType mapType, const char* textureFileName);
                SCENE_DATA_API virtual void SetTexture(TextureMapType mapType, const AZStd::string& textureFileName);
                SCENE_DATA_API virtual void SetTexture(TextureMapType mapType, AZStd::string&& textureFileName);
                SCENE_DATA_API virtual void SetNoDraw(bool isNoDraw);

                SCENE_DATA_API virtual void SetDiffuseColor(const AZ::Vector3& color);
                SCENE_DATA_API virtual void SetSpecularColor(const AZ::Vector3& color);
                SCENE_DATA_API virtual void SetEmissiveColor(const AZ::Vector3& color);
                SCENE_DATA_API virtual void SetOpacity(float opacity);
                SCENE_DATA_API virtual void SetShininess(float shininess);
                SCENE_DATA_API virtual void SetUniqueId(uint64_t uid);
                SCENE_DATA_API virtual void SetUseColorMap(AZStd::optional<bool> useColorMap);
                SCENE_DATA_API virtual void SetBaseColor(const AZStd::optional<AZ::Vector3>& baseColor);
                SCENE_DATA_API virtual void SetUseMetallicMap(AZStd::optional<bool> useMetallicMap);
                SCENE_DATA_API virtual void SetMetallicFactor(AZStd::optional<float> metallicFactor);
                SCENE_DATA_API virtual void SetUseRoughnessMap(AZStd::optional<bool> useRoughnessMap);
                SCENE_DATA_API virtual void SetRoughnessFactor(AZStd::optional<float> roughnessFactor);
                SCENE_DATA_API virtual void SetUseEmissiveMap(AZStd::optional<bool> useEmissiveMap);
                SCENE_DATA_API virtual void SetEmissiveIntensity(AZStd::optional<float> emissiveIntensity);
                SCENE_DATA_API virtual void SetUseAOMap(AZStd::optional<bool> useAOMap);
                

                SCENE_DATA_API const AZStd::string& GetTexture(TextureMapType mapType) const override;
                SCENE_DATA_API bool IsNoDraw() const override;

                SCENE_DATA_API const AZ::Vector3& GetDiffuseColor() const override;
                SCENE_DATA_API const AZ::Vector3& GetSpecularColor() const override;
                SCENE_DATA_API const AZ::Vector3& GetEmissiveColor() const override;
                SCENE_DATA_API float GetOpacity() const override;
                SCENE_DATA_API float GetShininess() const override;
                SCENE_DATA_API uint64_t GetUniqueId() const override;
                SCENE_DATA_API AZStd::optional<bool> GetUseColorMap() const override;
                SCENE_DATA_API AZStd::optional<AZ::Vector3> GetBaseColor() const override;
                SCENE_DATA_API AZStd::optional<bool> GetUseMetallicMap() const override;
                SCENE_DATA_API AZStd::optional<float> GetMetallicFactor() const override;
                SCENE_DATA_API AZStd::optional<bool> GetUseRoughnessMap() const override;
                SCENE_DATA_API AZStd::optional<float> GetRoughnessFactor() const override;
                SCENE_DATA_API AZStd::optional<bool> GetUseEmissiveMap() const override;
                SCENE_DATA_API AZStd::optional<float> GetEmissiveIntensity() const override;
                SCENE_DATA_API AZStd::optional<bool> GetUseAOMap() const override;

                static void Reflect(ReflectContext* context);

            protected:
                AZStd::unordered_map<TextureMapType, AZStd::string> m_textureMap;
                
                AZ::Vector3 m_diffuseColor;
                AZ::Vector3 m_specularColor;
                AZ::Vector3 m_emissiveColor;
                AZStd::optional<AZ::Vector3> m_baseColor;
                float m_opacity;
                float m_shininess;
                AZStd::optional<float> m_metallicFactor = AZStd::nullopt;
                AZStd::optional<float> m_roughnessFactor = AZStd::nullopt;
                AZStd::optional<float> m_emissiveIntensity = AZStd::nullopt;

                AZStd::optional<bool> m_useColorMap = AZStd::nullopt;
                AZStd::optional<bool> m_useMetallicMap = AZStd::nullopt;
                AZStd::optional<bool> m_useRoughnessMap = AZStd::nullopt;
                AZStd::optional<bool> m_useEmissiveMap = AZStd::nullopt;
                AZStd::optional<bool> m_useAOMap = AZStd::nullopt;

                bool m_isNoDraw;

                const AZStd::string m_emptyString;

                // A unique id which is used to identify a material in a fbx. 
                // This is the same as the ID in the fbx file's FbxNode
                uint64_t m_uniqueId;

                //! Material name from FbxNode's Object name
                AZStd::string m_materialName;
            };
            } // namespace GraphData
            } // namespace SceneData
            } // namespace AZ
