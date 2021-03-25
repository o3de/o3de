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

#pragma once

#include <SceneAPI/SceneData/SceneDataConfiguration.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>
#include <AzCore/std/containers/unordered_map.h>

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
                SCENE_DATA_API virtual void SetUseColorMap(bool useColorMap);
                SCENE_DATA_API virtual void SetBaseColor(const AZ::Vector3& baseColor);
                SCENE_DATA_API virtual void SetUseMetallicMap(bool useMetallicMap);
                SCENE_DATA_API virtual void SetMetallicFactor(float metallicFactor);
                SCENE_DATA_API virtual void SetUseRoughnessMap(bool useRoughnessMap);
                SCENE_DATA_API virtual void SetRoughnessFactor(float roughnessFactor);
                SCENE_DATA_API virtual void SetUseEmissiveMap(bool useEmissiveMap);
                SCENE_DATA_API virtual void SetEmissiveIntensity(float emissiveIntensity);
                SCENE_DATA_API virtual void SetUseAOMap(bool useAOMap);
                

                SCENE_DATA_API const AZStd::string& GetTexture(TextureMapType mapType) const override;
                SCENE_DATA_API bool IsNoDraw() const override;

                SCENE_DATA_API const AZ::Vector3& GetDiffuseColor() const override;
                SCENE_DATA_API const AZ::Vector3& GetSpecularColor() const override;
                SCENE_DATA_API const AZ::Vector3& GetEmissiveColor() const override;
                SCENE_DATA_API float GetOpacity() const override;
                SCENE_DATA_API float GetShininess() const override;
                SCENE_DATA_API uint64_t GetUniqueId() const override;
                SCENE_DATA_API bool GetUseColorMap() const override;
                SCENE_DATA_API const AZ::Vector3& GetBaseColor() const override;
                SCENE_DATA_API bool GetUseMetallicMap() const override;
                SCENE_DATA_API float GetMetallicFactor() const override;
                SCENE_DATA_API bool GetUseRoughnessMap() const override;
                SCENE_DATA_API float GetRoughnessFactor() const override;
                SCENE_DATA_API bool GetUseEmissiveMap() const override;
                SCENE_DATA_API float GetEmissiveIntensity() const override;
                SCENE_DATA_API bool GetUseAOMap() const override;

                static void Reflect(ReflectContext* context);

            protected:
                AZStd::unordered_map<TextureMapType, AZStd::string> m_textureMap;
                
                AZ::Vector3 m_diffuseColor;
                AZ::Vector3 m_specularColor;
                AZ::Vector3 m_emissiveColor;
                AZ::Vector3 m_baseColor;
                float m_opacity;
                float m_shininess;
                float m_metallicFactor = 0.0f;
                float m_roughnessFactor = 0.0f;
                float m_emissiveIntensity = 0.0f;

                bool m_useColorMap = false;
                bool m_useMetallicMap = false;
                bool m_useRoughnessMap = false;
                bool m_useEmissiveMap = false;
                bool m_useAOMap = false;

                bool m_isNoDraw;

                const static AZStd::string s_DiffuseMapName;
                const static AZStd::string s_SpecularMapName;
                const static AZStd::string s_BumpMapName;
                const static AZStd::string s_emptyString;

                // A unique id which is used to identify a material in a fbx. 
                // This is the same as the ID in the fbx file's FbxNode
                uint64_t m_uniqueId;

                //! Material name from FbxNode's Object name
                AZStd::string m_materialName;
            };
            } // namespace GraphData
            } // namespace SceneData
            } // namespace AZ
