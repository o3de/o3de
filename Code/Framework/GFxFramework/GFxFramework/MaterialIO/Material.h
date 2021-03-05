#pragma once

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


#include <AzCore/XML/rapidxml.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <GFxFramework/MaterialIO/IMaterial.h>

namespace AZ 
{
    namespace GFxFramework 
    {
        class Material
            : public IMaterial
        {
        public:
            Material();
      
            ~Material() override = default;
            void SetDataFromMtl(rapidxml::xml_node<char>* materialNode) override;
            
            const AZStd::string& GetName() const override;
            void SetName(const AZStd::string& name) override;
            
            const AZStd::string& GetTexture(TextureMapType type) const override;
            void SetTexture(TextureMapType type, const AZStd::string& texture) override;
            
            bool UseVertexColor() const override;
            void EnableUseVertexColor(bool useVertexColor) override;
            
            int GetMaterialFlags() const override;
            void SetMaterialFlags(int flags) override;

            const AZ::Vector3& GetDiffuseColor() const override;
            const AZ::Vector3& GetSpecularColor() const override;
            const AZ::Vector3& GetEmissiveColor() const override;
            float GetOpacity() const override;
            float GetShininess() const override;

            void SetDiffuseColor(const AZ::Vector3& color) override;
            void SetSpecularColor(const AZ::Vector3& color) override;
            void SetEmissiveColor(const AZ::Vector3& color) override;
            void SetOpacity(float opacity) override;
            void SetShininess(float shininess) override;

            AZ::u32 GetDccMaterialHash() const override;
            void SetDccMaterialHash(AZ::u32 hash) override;

        private:
            AZStd::string m_materialName;
            AZStd::string m_diffuseMap;
            AZStd::string m_specularMap;
            AZStd::string m_normalMap;
            AZStd::string m_empty;//dummy place holder string to kill warning
            int m_flags;
            bool m_useVertexColor;


            AZ::Vector3 m_diffuseColor;
            AZ::Vector3 m_specularColor;
            AZ::Vector3 m_emissiveColor;
            float m_opacity;
            float m_shininess;

            AZ::u32 m_dccMaterialHash;
        };

        class MaterialGroup
            : public IMaterialGroup
        {
        public:
            MaterialGroup();
            ~MaterialGroup() override = default;

            void AddMaterial(const AZStd::shared_ptr<IMaterial>& material) override;
            void RemoveMaterial(const AZStd::string& name) override;
            size_t FindMaterialIndex(const AZStd::string& name) override;
            size_t GetMaterialCount() override;
            AZStd::shared_ptr<IMaterial> GetMaterial(size_t index) override;
            AZStd::shared_ptr<const IMaterial> GetMaterial(size_t index) const override;
            const AZStd::string& GetMtlName() override;
            void SetMtlName(const AZStd::string& name) override;
            bool ReadMtlFile(const char* fileName) override;
            bool WriteMtlFile(const char* fileName) override;

        private:
            AZStd::vector<AZStd::shared_ptr<IMaterial>> m_materials;
            AZStd::vector<char> m_mtlBuffer;
            rapidxml::xml_document<char> m_mtlDoc;
            AZStd::string m_materialGroupName;
            bool m_readFromMtl;

            bool AddMaterialNode(const IMaterial& mat);
            bool UpdateMaterialNode(const IMaterial& mat);
            void RemoveMaterialNode(const IMaterial& mat);

            AZ::u32 CalculateDccMaterialHash();

            void CreateMtlFile();
            void UpdateMtlFile();
            rapidxml::xml_node<char>* FindMaterialNode(const IMaterial& mat) const;
            rapidxml::xml_node<char>* CreateMaterialMtlNode(const IMaterial& material);

            rapidxml::xml_node<char>* CreateTextureMtlNode(const char* name, const char* fileName);

        };
    } //GFxFramework
}//AZ