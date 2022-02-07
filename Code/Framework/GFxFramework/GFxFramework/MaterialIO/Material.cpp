/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/XML/rapidxml_print.h>
#include <AzCore/Math/Vector3.h>

#include <AzToolsFramework/Debug/TraceContext.h>
#include <GFxFramework/MaterialIO/Material.h>

namespace AZ
{
    namespace GFxFramework
    {
        //These strings form the basis for a default illum or physics shader that can be 
        //used in the case where we are generating new mtl files and not just updating or 
        //reading an mtl file for processing purposes. 
        namespace MaterialExport
        {
             const char* g_subMaterialString = "SubMaterials";
             const char* g_materialString = "Material";
             const char* g_texturesString = "Textures";
             const char* g_textureString = "Texture";
             const char* g_diffuseMapName = "Diffuse";
             const char* g_specularMapName = "Specular";
             const char* g_emissiveMapName = "Emittance";
             const char* g_bumpMapName = "Bumpmap";
             const char* g_shininessString = "Shininess";
             const char* g_opacityString = "Opacity";
             const char* g_genString = "GenMask";
             const char* g_illumShaderName = "Illum";
             const char* g_defaultIllumGenMask = "80000001";
             const char* g_defaultNoDrawGenMask = "0";
             const char* g_defaultShininess = "10";
             const char* g_defaultOpacity = "1";
             const char* g_mapString = "Map";
             const char* g_fileString = "File";
             const char* g_nameString = "Name";
             const char* g_shaderString = "Shader";
             const char* g_noDrawShaderName = "Nodraw";
             const char* g_mtlFlagString = "MtlFlags";
             const char* g_whiteColor = "1,1,1";
             const char* g_blackColor = "0,0,0";

             const char* g_stringGenMask = "StringGenMask";
             const char* g_stringGenMaskOption_VertexColors = "%VERTCOLORS";
             const char* g_stringPhysicsNoDraw = "PhysicsNoDraw";

             const char* g_mtlExtension = ".mtl";
             const char* g_dccMaterialExtension = ".dccmtl";
             const char* g_dccMaterialHashString = "DccMaterialHash";

             const char* g_whiteTexture = "EngineAssets/Textures/white.dds";

             const unsigned g_materialNotFound = unsigned(-1);

             bool ParseVectorAttribute(rapidxml::xml_attribute<char>* attribute, AZ::Vector3& vectorValue)
             {
                 if (attribute)
                 {
                     float x, y, z;
                     const char* value = attribute->value();
                     if (azsscanf(value, "%f,%f,%f", &x, &y, &z) == 3)
                     {
                         vectorValue.Set(x, y, z);
                         return true;
                     }
                 }
                 return false;
             }

             bool ParseFloatAttribute(rapidxml::xml_attribute<char>* attribute, float& floatValue)
             {
                 if (attribute)
                 {
                     const char* value = attribute->value();
                     if (azsscanf(value, "%f", &floatValue) == 1)
                     {
                         return true;
                     }
                 }
                 return false;
             }

             bool ParseUintAttribute(rapidxml::xml_attribute<char>* attribute, AZ::u32& uintValue)
             {
                 if (attribute)
                 {
                     const char* value = attribute->value();
                     if (azsscanf(value, "%u", &uintValue) == 1)
                     {
                         return true;
                     }
                 }
                 return false;
             }

             bool ParseIntAttribute(rapidxml::xml_attribute<char>* attribute, AZ::s32& intValue)
             {
                 if (attribute)
                 {
                     const char* value = attribute->value();
                     if (azsscanf(value, "%d", &intValue) == 1)
                     {
                         return true;
                     }
                 }
                 return false;
             }
        }

        Material::Material() 
            : m_useVertexColor(false)
            , m_flags(0)
            , m_diffuseColor(AZ::Vector3::CreateOne())
            , m_specularColor(AZ::Vector3::CreateZero())
            , m_emissiveColor(AZ::Vector3::CreateZero())
            , m_opacity(1.f)
            , m_shininess(0.f)
            , m_dccMaterialHash(0)
        {
        }

        void Material::SetDataFromMtl(rapidxml::xml_node<char>* materialNode)
        {
            if (materialNode)
            {
                rapidxml::xml_attribute<char>* name = materialNode->first_attribute(MaterialExport::g_nameString);
                rapidxml::xml_attribute<char>* mtlFlags = materialNode->first_attribute(MaterialExport::g_mtlFlagString);
                rapidxml::xml_attribute<char>* stringGenMask = materialNode->first_attribute(MaterialExport::g_stringGenMask);

                rapidxml::xml_node<char>* textureNode = materialNode->first_node(MaterialExport::g_texturesString);

                rapidxml::xml_attribute<char>* diffuseColor = materialNode->first_attribute(MaterialExport::g_diffuseMapName);
                rapidxml::xml_attribute<char>* specularColor = materialNode->first_attribute(MaterialExport::g_specularMapName);
                rapidxml::xml_attribute<char>* emissiveColor = materialNode->first_attribute(MaterialExport::g_emissiveMapName);
                rapidxml::xml_attribute<char>* opacity = materialNode->first_attribute(MaterialExport::g_opacityString);
                rapidxml::xml_attribute<char>* shininess = materialNode->first_attribute(MaterialExport::g_shininessString);
                rapidxml::xml_attribute<char>* hash = materialNode->first_attribute(MaterialExport::g_dccMaterialHashString);

                if (name)
                {
                    m_materialName = AZStd::string(name->value());
                }

                MaterialExport::ParseVectorAttribute(diffuseColor, m_diffuseColor);
                MaterialExport::ParseVectorAttribute(specularColor, m_specularColor);
                MaterialExport::ParseVectorAttribute(emissiveColor, m_emissiveColor);
                MaterialExport::ParseFloatAttribute(opacity, m_opacity);
                MaterialExport::ParseFloatAttribute(shininess, m_shininess);
                MaterialExport::ParseUintAttribute(hash, m_dccMaterialHash);

                if (mtlFlags)
                {
                    MaterialExport::ParseIntAttribute(mtlFlags, m_flags);
                }

                if (stringGenMask)
                {
                    AZStd::string stringMask(stringGenMask->value());
                    if (stringMask.find(MaterialExport::g_stringGenMaskOption_VertexColors) != AZStd::string::npos)
                    {
                        m_useVertexColor = true;
                    }
                    else
                    {
                        m_useVertexColor = false;
                    }
                }

                if (textureNode)
                {
                    rapidxml::xml_node<char>* texture = textureNode->first_node(MaterialExport::g_textureString);
                    while (texture)
                    {
                        rapidxml::xml_attribute<char>* mapType = texture->first_attribute(MaterialExport::g_mapString);
                        rapidxml::xml_attribute<char>* fileName = texture->first_attribute(MaterialExport::g_fileString);

                        if (!mapType || !fileName)
                        {
                            AZ_TracePrintf("Warning", "Detected malformed texture data in MTL file.");
                            texture = texture->next_sibling(MaterialExport::g_texturesString);
                            continue;
                        }

                        if (azstrnicmp(mapType->value(), MaterialExport::g_diffuseMapName,
                           AZStd::GetMin(strlen(mapType->value()), strlen(MaterialExport::g_diffuseMapName))) == 0)
                        {
                            m_diffuseMap = AZStd::string(fileName->value());
                        }

                        if (azstrnicmp(mapType->value(), MaterialExport::g_specularMapName,
                            AZStd::GetMin(strlen(mapType->value()), strlen(MaterialExport::g_specularMapName))) == 0)
                        {
                            m_specularMap = AZStd::string(fileName->value());
                        }

                        if (azstrnicmp(mapType->value(), MaterialExport::g_bumpMapName,
                            AZStd::GetMin(strlen(mapType->value()), strlen(MaterialExport::g_bumpMapName))) == 0)
                        {
                            m_normalMap = AZStd::string(fileName->value());
                        }

                        texture = texture->next_sibling(MaterialExport::g_textureString);
                    }
                }
            }
        }
        const AZStd::string& Material::GetName() const
        {
            return m_materialName;
        }
        void Material::SetName(const AZStd::string& name)
        {
            m_materialName = name;
        }

        const AZStd::string& Material::GetTexture(TextureMapType type) const
        {
            switch (type)
            {
                case TextureMapType::Diffuse:
                    return m_diffuseMap;
                case TextureMapType::Specular:
                    return m_specularMap;
                case TextureMapType::Bump:
                    return m_normalMap;
                default:
                    AZ_Assert(false, "Invalid Texture map requested.");
                    return m_empty;
            }
        }

        void Material::SetTexture(TextureMapType type, const AZStd::string& texture)
        {
            switch (type)
            {
            case TextureMapType::Diffuse:
                m_diffuseMap = texture;
                break;
            case TextureMapType::Specular:
                m_specularMap = texture;
                break;
            case TextureMapType::Bump:
                m_normalMap = texture;
                break;
            default:
                AZ_Assert(false, "Invalid Texture map requested.");
                break;
            }
        }
        bool Material::UseVertexColor() const
        {
            return m_useVertexColor;
        }

        void Material::EnableUseVertexColor(bool useVertexColor)
        {
            m_useVertexColor = useVertexColor;
        }
        
        int Material::GetMaterialFlags() const
        {
            return m_flags;
        }

        void Material::SetMaterialFlags(int flags)
        {
            m_flags = flags;
        }

        const AZ::Vector3& Material::GetDiffuseColor() const
        {
            return m_diffuseColor;
        }

        const AZ::Vector3& Material::GetSpecularColor() const
        {
            return m_specularColor;
        }

        const AZ::Vector3& Material::GetEmissiveColor() const
        {
            return m_emissiveColor;
        }

        float Material::GetOpacity() const
        {
            return m_opacity;
        }

        float Material::GetShininess() const
        {
            return m_shininess;
        }

        void Material::SetDiffuseColor(const AZ::Vector3& color)
        {
            m_diffuseColor = color;
        }

        void Material::SetSpecularColor(const AZ::Vector3& color)
        {
            m_specularColor = color;
        }

        void Material::SetEmissiveColor(const AZ::Vector3& color)
        {
            m_emissiveColor = color;
        }

        void Material::SetOpacity(float opacity)
        {
            m_opacity = opacity;
        }

        void Material::SetShininess(float shininess)
        {
            m_shininess = shininess;
        }

        AZ::u32 Material::GetDccMaterialHash() const
        {
            return m_dccMaterialHash;
        }

        void Material::SetDccMaterialHash(AZ::u32 hash)
        {
            m_dccMaterialHash = hash;
        }

        MaterialGroup::MaterialGroup() 
            : m_readFromMtl(false)
        {
        }

        size_t MaterialGroup::FindMaterialIndex(const AZStd::string& name)
        {
            size_t index = 0;
            for (const auto& mat : m_materials)
            {
                if (mat && mat->GetName().compare(name) == 0)
                {
                    return index;
                }
                ++index;
            }

            return GFxFramework::MaterialExport::g_materialNotFound; 
        }

        size_t MaterialGroup::GetMaterialCount()
        {
            return m_materials.size();
        }

        AZStd::shared_ptr<IMaterial> MaterialGroup::GetMaterial(size_t index)
        {
            if (index < m_materials.size())
            {
                return m_materials[index];
            }
            else
            {
                return nullptr;
            }
        }
        AZStd::shared_ptr<const IMaterial> MaterialGroup::GetMaterial(size_t index) const
        {
            if (index < m_materials.size())
            {
                return m_materials[index];
            }
            else
            {
                return nullptr;
            }
        }

        void MaterialGroup::AddMaterial(const AZStd::shared_ptr<IMaterial>& material)
        {
            //Don't add two materials with the same name. 
            for (const auto& mat : m_materials)
            {
                if (mat && mat->GetName().compare(material->GetName()) == 0)
                {
                    return;
                }
            }
            m_materials.push_back(material);
        }

        void MaterialGroup::RemoveMaterial(const AZStd::string& name)
        {
            for (size_t i = 0; i < m_materials.size(); ++i)
            {
                if (m_materials[i] && m_materials[i]->GetName().compare(name) == 0)
                {
                    if (m_readFromMtl)
                    {
                        RemoveMaterialNode(*(m_materials[i].get()));
                    }
                    m_materials.erase(m_materials.begin() + i);
                    return;
                }
            }
        }

        bool MaterialGroup::ReadMtlFile(const char* fileName)
        {
            IO::SystemFile mtlFile;
            bool fileOpened = mtlFile.Open(fileName, IO::SystemFile::SF_OPEN_READ_ONLY);
            if (!fileOpened || mtlFile.Length() == 0)
            {
                //If the file successfully opened, but the length was 0, report an error
                if (fileOpened)
                {
                    AZ_Error("MaterialIO", false, "Invalid material file %s. File length was 0. Try removing the file or replacing it with a valid material file.", fileName);
                }

                m_readFromMtl = false;
                return false;
            }

            //Read mtl file into a persistent buffer, due to the mechanics of rapidxml this buffer must
            //have the same lifetime as the mtl file if we intend to edit the file. 
            m_mtlBuffer.reserve(mtlFile.Length());
            mtlFile.Read(mtlFile.Length(), m_mtlBuffer.data());
            mtlFile.Close();

            //Apparently in rapidxml if 'parse_no_data_nodes' isn't set it creates both value and data nodes 
            //with the data nodes having precedence such that updating values doesn't work. 
            m_mtlDoc.parse<rapidxml::parse_no_data_nodes>(m_mtlBuffer.data());

            //Parse MTL file for materials and/or submaterials. 
            rapidxml::xml_node<char>* materialNode = m_mtlDoc.first_node(MaterialExport::g_materialString);
            if (!materialNode)
            {
                AZ_Error("MaterialIO", false, "Invalid material file %s. File does not contain a 'Material' node. Try removing the file or replacing it with a valid material file.", fileName);

                m_readFromMtl = false;
                return false;
            }

            rapidxml::xml_node<char>* submaterialNode = materialNode->first_node("SubMaterials");

            if (submaterialNode)
            {
                materialNode = submaterialNode->first_node();
                while (materialNode)
                {
                    AZStd::shared_ptr<IMaterial> mat = AZStd::make_shared<Material>();
                    mat->SetDataFromMtl(materialNode);
                    AddMaterial(mat);
                    materialNode = materialNode->next_sibling();
                }
            }
            else
            {
                AZStd::shared_ptr<IMaterial> mat = AZStd::make_shared<Material>();
                mat->SetDataFromMtl(materialNode);
                AddMaterial(mat);
            }

            m_readFromMtl = true;
            return true;
        }

        bool MaterialGroup::WriteMtlFile(const char* fileName)
        {
            //The MaterialGroup is responsible for updating the mtl XML data and/or creating
            //new default mtl files. This needs to be EBUS-ified eventually. 

            AZ_TraceContext("MTL File Name", fileName);
            if (!m_readFromMtl)
            {
                CreateMtlFile();
            }
          
            if(m_readFromMtl)
            {
                UpdateMtlFile();
            }
            
            IO::SystemFile mtlFile;
            if (!mtlFile.Open(fileName, IO::SystemFile::SF_OPEN_CREATE | IO::SystemFile::SF_OPEN_WRITE_ONLY))
            {
                AZ_TracePrintf("Error", "Unable to write MTL file to disk");
                return false;
            }


            //Write out MTL data from rapidXML then write mtl file to disk. 
            AZStd::vector<char> buffer;
            rapidxml::print(AZStd::back_inserter(buffer), m_mtlDoc);

            mtlFile.Write(buffer.data(), buffer.size());
            mtlFile.Close();

            return true;
        }

        const AZStd::string& MaterialGroup::GetMtlName()
        {
            return m_materialGroupName;
        }

        void MaterialGroup::SetMtlName(const AZStd::string&  name)
        {
            m_materialGroupName = name;
        }

        AZ::u32 MaterialGroup::CalculateDccMaterialHash()
        {
            // Compound material group hash from sub material hashes.
            AZ::Crc32 hash(0u);
            for (const auto& mat : m_materials)
            {
                if (mat)
                {
                    AZ::u32 subMaterialHash = mat->GetDccMaterialHash();
                    hash.Add(&subMaterialHash, sizeof(AZ::u32));
                }
            }
            return hash;
        }

        void MaterialGroup::CreateMtlFile()
        {
            rapidxml::xml_node<char>* rootNode = m_mtlDoc.allocate_node(rapidxml::node_element, MaterialExport::g_materialString);
            
            // MtlFlags
            rapidxml::xml_attribute<char>* attr = m_mtlDoc.allocate_attribute(MaterialExport::g_mtlFlagString,
                m_mtlDoc.allocate_string(AZStd::to_string(EMaterialFlags::MTL_64BIT_SHADERGENMASK | EMaterialFlags::MTL_FLAG_MULTI_SUBMTL).c_str()));
            rootNode->append_attribute(attr);

            // DccMaterialHash
            const AZStd::string hashString = AZStd::string::format("%u", CalculateDccMaterialHash());
            attr = m_mtlDoc.allocate_attribute(MaterialExport::g_dccMaterialHashString, m_mtlDoc.allocate_string(hashString.c_str()));
            rootNode->append_attribute(attr);

            // SubMaterials
            rapidxml::xml_node<char>* subMaterialNode = m_mtlDoc.allocate_node(rapidxml::node_element, MaterialExport::g_subMaterialString);
            rootNode->append_node(subMaterialNode);

            m_mtlDoc.append_node(rootNode);
            for (const auto& mat : m_materials)
            {
                if (mat)
                {
                    subMaterialNode->append_node(CreateMaterialMtlNode(*(mat.get())));
                }
            }
        }

        void MaterialGroup::UpdateMtlFile()
        {
            // Update DCC material hash
            rapidxml::xml_node<char>* rootNode = m_mtlDoc.first_node(MaterialExport::g_materialString);
            if (rootNode)
            {
                rapidxml::xml_attribute<char>* dccMaterialHashAttribute = rootNode->first_attribute(MaterialExport::g_dccMaterialHashString);
                if (dccMaterialHashAttribute)
                {
                    const AZStd::string hashString = AZStd::string::format("%u", CalculateDccMaterialHash());
                    dccMaterialHashAttribute->value(m_mtlDoc.allocate_string(hashString.c_str()));
                }

                //update or add materials
                for (auto& mat : m_materials)
                {
                    if (mat)
                    {
                        bool updated = UpdateMaterialNode(*(mat.get()));
                        if (!updated)
                        {
                            AddMaterialNode(*(mat.get()));
                        }
                    }
                }
            }
        }

        bool MaterialGroup::AddMaterialNode(const IMaterial& mat)
        {
            rapidxml::xml_node<char>* materialNode = m_mtlDoc.first_node(MaterialExport::g_materialString);

            rapidxml::xml_node<char>* submaterialNode = nullptr;

            if (!materialNode)
            {
                AZ_Assert(false, "Attempted to add material to invalid xml document.");
                return false;
            }

            submaterialNode = materialNode->first_node("SubMaterials");

            if (submaterialNode)
            {
               submaterialNode->append_node(CreateMaterialMtlNode(mat));
            }
            else 
            {
               materialNode->append_node(CreateMaterialMtlNode(mat));
            }

            return true;
        }

        struct TextureData
        {
            AZStd::string fileName;
            AZStd::string exportName;
            bool updated;
        };

        bool MaterialGroup::UpdateMaterialNode(const IMaterial& mat)
        {
            rapidxml::xml_node<char>* node = FindMaterialNode(mat);

            if (!node)
            {
                return false;
            }

            //Set appropriate attributes
            {
                rapidxml::xml_attribute<char>* flagAttribute = node->first_attribute(MaterialExport::g_mtlFlagString);
                if (flagAttribute)
                {
                    int flag = atoi(flagAttribute->value());
                    flagAttribute->value(m_mtlDoc.allocate_string(AZStd::to_string(flag).c_str()));
                }
            }

            // Update DCC material hash
            {
                rapidxml::xml_attribute<char>* dccMaterialHashAttribute = node->first_attribute(MaterialExport::g_dccMaterialHashString);
                if (dccMaterialHashAttribute)
                {
                    const AZStd::string hashString = AZStd::string::format("%u", mat.GetDccMaterialHash());
                    dccMaterialHashAttribute->value(m_mtlDoc.allocate_string(hashString.c_str()));
                }
            }

            {
                AZStd::string genMask;
                rapidxml::xml_attribute<char>* stringGenMaskAttribute = node->first_attribute(MaterialExport::g_stringGenMask);
                if (stringGenMaskAttribute)
                {
                    genMask = AZStd::string(stringGenMaskAttribute->value());
                }

                if (mat.UseVertexColor() && genMask.find(MaterialExport::g_stringGenMaskOption_VertexColors) == AZStd::string::npos)
                {
                    genMask += MaterialExport::g_stringGenMaskOption_VertexColors;
                }
                else if (!mat.UseVertexColor())
                {
                    size_t index = genMask.find(MaterialExport::g_stringGenMaskOption_VertexColors);
                    if (index != AZStd::string::npos)
                    {
                        genMask = genMask.substr(0, index) + genMask.substr(index + strlen(MaterialExport::g_stringGenMaskOption_VertexColors), AZStd::string::npos);
                    }
                }

                if (stringGenMaskAttribute && genMask.length() > 0)
                {
                    stringGenMaskAttribute->value(m_mtlDoc.allocate_string(genMask.c_str()));
                }
                else if (stringGenMaskAttribute && genMask.length() == 0)
                {
                    node->remove_attribute(stringGenMaskAttribute);
                }
                else if (!stringGenMaskAttribute && genMask.length() > 0)
                {
                    stringGenMaskAttribute = m_mtlDoc.allocate_attribute(MaterialExport::g_stringGenMask, m_mtlDoc.allocate_string(genMask.c_str()));
                    node->append_attribute(stringGenMaskAttribute);
                }
            }

            //Set Texture fields
            AZStd::vector<TextureData> textures;
            TextureData diffuse;
            diffuse.fileName = mat.GetTexture(TextureMapType::Diffuse);
            if (diffuse.fileName.empty())
            {
                // Default to white texture if the material has no textures. This ensures
                // meshes purely using material/vertex colors render properly in-engine.
                diffuse.fileName = MaterialExport::g_whiteTexture;
            }
            diffuse.exportName = MaterialExport::g_diffuseMapName;
            diffuse.updated = false;
            textures.push_back(diffuse);

            TextureData specular;
            specular.fileName = mat.GetTexture(TextureMapType::Specular);
            specular.exportName = MaterialExport::g_specularMapName;
            specular.updated = false;
            textures.push_back(specular);

            TextureData bump;
            bump.fileName = mat.GetTexture(TextureMapType::Bump);
            bump.exportName = MaterialExport::g_bumpMapName;
            bump.updated = false;
            textures.push_back(bump);

            rapidxml::xml_node<char>* texturesNode = node->first_node(MaterialExport::g_texturesString);
            if (texturesNode)
            {
                for (auto& currentTexture : textures)
                {
                    rapidxml::xml_node<char>* textureNode = texturesNode->first_node(MaterialExport::g_textureString);
                    while (textureNode)
                    {
                        rapidxml::xml_attribute<char>* mapType = textureNode->first_attribute(MaterialExport::g_mapString);
                        rapidxml::xml_attribute<char>* fileName = textureNode->first_attribute(MaterialExport::g_fileString);
                            
                        //malformed texture node. 
                        if (!mapType || !fileName)
                        {
                            AZ_TracePrintf("Warning", "Detected malformed texture data in MTL file.");
                            textureNode = textureNode->next_sibling(MaterialExport::g_texturesString);
                            continue;
                        }
                        if (azstrnicmp(mapType->value(), currentTexture.exportName.c_str(),
                            AZStd::GetMin(strlen(mapType->value()), strlen(currentTexture.exportName.c_str()))) == 0)
                        {
                            //We have removed this texture in this case. Remove the texture node.
                            if (currentTexture.fileName.empty())
                            {
                                texturesNode->remove_node(textureNode);
                            }
                            else
                            {
                                fileName->value(m_mtlDoc.allocate_string(currentTexture.fileName.c_str()));
                                currentTexture.updated = true;
                            }
                            break;
                        }
                        textureNode = textureNode->next_sibling(MaterialExport::g_texturesString);
                    }

                    //texture not found in list add it. 
                    if (!currentTexture.updated && !currentTexture.fileName.empty())
                    {
                        textureNode = CreateTextureMtlNode(currentTexture.exportName.c_str(), currentTexture.fileName.c_str());
                        texturesNode->append_node(textureNode);
                    }
                }
            }
            else
            {
                //Add Texture paramaters to the material.
                texturesNode = m_mtlDoc.allocate_node(rapidxml::node_element, MaterialExport::g_texturesString);
                if (texturesNode)
                {
                    for (const auto& currentTexture : textures)
                    {
                        if (!currentTexture.fileName.empty())
                        {
                            rapidxml::xml_node<char>* textureNode = CreateTextureMtlNode(currentTexture.exportName.c_str(), currentTexture.fileName.c_str());
                            texturesNode->append_node(textureNode);
                        }
                    }
                    node->append_node(texturesNode);
                }
            }
  
            return true;
        }

        void MaterialGroup::RemoveMaterialNode(const IMaterial& mat)
        {
            rapidxml::xml_node<char>* node = FindMaterialNode(mat);
            if (node && node->parent())
            {
                node->parent()->remove_node(node);
            }
        }

        rapidxml::xml_node<char>* MaterialGroup::FindMaterialNode(const IMaterial& mat) const
        {
            rapidxml::xml_node<char>* materialNode = m_mtlDoc.first_node(MaterialExport::g_materialString);
            if (materialNode == nullptr)
            {
                return nullptr;
            }
           
            rapidxml::xml_node<char>* submaterialNode = nullptr;
            submaterialNode = materialNode->first_node("SubMaterials");

            if (submaterialNode)
            {
                materialNode = submaterialNode->first_node();
                while (materialNode)
                {
                    rapidxml::xml_attribute<char>* name = materialNode->first_attribute(MaterialExport::g_nameString);
                    if (name && azstrnicmp(name->value(), mat.GetName().c_str(), mat.GetName().size()) == 0)
                    {
                        break;
                    }
                    materialNode = materialNode->next_sibling();
                }
            }
            else
            {
                rapidxml::xml_attribute<char>* name = materialNode->first_attribute(MaterialExport::g_nameString);
                if (name && azstrnicmp(name->value(), mat.GetName().c_str(), mat.GetName().size()) != 0)
                {
                    return nullptr;
                }
            }

            return materialNode;
        }

        rapidxml::xml_node<char>* MaterialGroup::CreateMaterialMtlNode(const IMaterial& material)
        {
            rapidxml::xml_node<char>* materialNode =  m_mtlDoc.allocate_node(rapidxml::node_element, MaterialExport::g_materialString);
            rapidxml::xml_attribute<char>* attr = m_mtlDoc.allocate_attribute(MaterialExport::g_nameString, m_mtlDoc.allocate_string(material.GetName().c_str()));
            materialNode->append_attribute(attr);

            int materialFlags = material.GetMaterialFlags();

            if (materialFlags & (EMaterialFlags::MTL_FLAG_NODRAW | EMaterialFlags::MTL_FLAG_NODRAW_TOUCHBENDING))
            {
                materialFlags |= EMaterialFlags::MTL_FLAG_PURE_CHILD;
                attr = m_mtlDoc.allocate_attribute(MaterialExport::g_mtlFlagString,
                    m_mtlDoc.allocate_string(AZStd::to_string(materialFlags).c_str()));
                materialNode->append_attribute(attr);
                attr = m_mtlDoc.allocate_attribute(MaterialExport::g_shaderString, MaterialExport::g_noDrawShaderName);
                materialNode->append_attribute(attr);
                attr = m_mtlDoc.allocate_attribute(MaterialExport::g_genString, MaterialExport::g_defaultNoDrawGenMask);
                materialNode->append_attribute(attr);
            }
            else
            {
                materialFlags |= (EMaterialFlags::MTL_64BIT_SHADERGENMASK | EMaterialFlags::MTL_FLAG_PURE_CHILD);
                attr = m_mtlDoc.allocate_attribute(MaterialExport::g_mtlFlagString,
                    m_mtlDoc.allocate_string(AZStd::to_string(materialFlags).c_str()));
                materialNode->append_attribute(attr);
                attr = m_mtlDoc.allocate_attribute(MaterialExport::g_shaderString, MaterialExport::g_illumShaderName);
                materialNode->append_attribute(attr);
                attr = m_mtlDoc.allocate_attribute(MaterialExport::g_genString, MaterialExport::g_defaultIllumGenMask);
                materialNode->append_attribute(attr);
            }

            const AZ::Vector3& diffuseColor = material.GetDiffuseColor();
            const AZ::Vector3& specularColor = material.GetSpecularColor();
            const AZ::Vector3& emissiveColor = material.GetEmissiveColor();
            const AZStd::string diffuseColorString = AZStd::string::format("%f,%f,%f", (float)diffuseColor.GetX(), (float)diffuseColor.GetY(), (float)diffuseColor.GetZ());
            const AZStd::string specularColorString = AZStd::string::format("%f,%f,%f", (float)specularColor.GetX(), (float)specularColor.GetY(), (float)specularColor.GetZ());
            const AZStd::string emissiveColorString = AZStd::string::format("%f,%f,%f", (float)emissiveColor.GetX(), (float)emissiveColor.GetY(), (float)emissiveColor.GetZ());
            const AZStd::string opacityString = AZStd::string::format("%f", material.GetOpacity());
            const AZStd::string shininessString = AZStd::string::format("%f", material.GetShininess());
            const AZStd::string hashString = AZStd::string::format("%u", material.GetDccMaterialHash());

            attr = m_mtlDoc.allocate_attribute(MaterialExport::g_diffuseMapName, m_mtlDoc.allocate_string(diffuseColorString.c_str()));
            materialNode->append_attribute(attr);
            attr = m_mtlDoc.allocate_attribute(MaterialExport::g_specularMapName, m_mtlDoc.allocate_string(specularColorString.c_str()));
            materialNode->append_attribute(attr);
            attr = m_mtlDoc.allocate_attribute(MaterialExport::g_emissiveMapName, m_mtlDoc.allocate_string(emissiveColorString.c_str()));
            materialNode->append_attribute(attr);
            attr = m_mtlDoc.allocate_attribute(MaterialExport::g_opacityString, m_mtlDoc.allocate_string(opacityString.c_str()));
            materialNode->append_attribute(attr);
            attr = m_mtlDoc.allocate_attribute(MaterialExport::g_shininessString, m_mtlDoc.allocate_string(shininessString.c_str()));
            materialNode->append_attribute(attr);
            attr = m_mtlDoc.allocate_attribute(MaterialExport::g_dccMaterialHashString, m_mtlDoc.allocate_string(hashString.c_str()));
            materialNode->append_attribute(attr);

            AZStd::string genMask;
            if (material.UseVertexColor())
            {
                genMask += MaterialExport::g_stringGenMaskOption_VertexColors;
            }

            if (genMask.length() > 0)
            {
                attr = m_mtlDoc.allocate_attribute(MaterialExport::g_stringGenMask, m_mtlDoc.allocate_string(genMask.c_str()));
                materialNode->append_attribute(attr);
            }

            //Set Texture fields
            AZStd::vector<TextureData> textures;
            TextureData diffuse;
            diffuse.fileName = material.GetTexture(TextureMapType::Diffuse);
            if (diffuse.fileName.empty())
            {
                // Default to white texture if the material has no textures. This ensures
                // meshes purely using material/vertex colors render properly in-engine.
                diffuse.fileName = MaterialExport::g_whiteTexture;
            }
            diffuse.exportName = MaterialExport::g_diffuseMapName;
            diffuse.updated = false;
            textures.push_back(diffuse);

            TextureData specular;
            specular.fileName = material.GetTexture(TextureMapType::Specular);
            specular.exportName = MaterialExport::g_specularMapName;
            specular.updated = false;
            textures.push_back(specular);

            TextureData bump;
            bump.fileName = material.GetTexture(TextureMapType::Bump);
            bump.exportName = MaterialExport::g_bumpMapName;
            bump.updated = false;
            textures.push_back(bump);

            //Add Texture paramaters to the material.
            rapidxml::xml_node<char>* texturesNode = m_mtlDoc.allocate_node(rapidxml::node_element, MaterialExport::g_texturesString);
            if (texturesNode)
            {
                for (const auto& currentTexture : textures)
                {
                    if (!currentTexture.fileName.empty())
                    {
                        rapidxml::xml_node<char>* textureNode = CreateTextureMtlNode(currentTexture.exportName.c_str(), currentTexture.fileName.c_str());
                        texturesNode->append_node(textureNode);
                    }
                }
                materialNode->append_node(texturesNode);
            }

            return materialNode;
        }

        rapidxml::xml_node<char>* MaterialGroup::CreateTextureMtlNode(const char* name, const char* fileName)
        {
            rapidxml::xml_node<char>* textureNode = m_mtlDoc.allocate_node(rapidxml::node_element, MaterialExport::g_textureString);
            rapidxml::xml_attribute<char>* attr = m_mtlDoc.allocate_attribute(MaterialExport::g_mapString, m_mtlDoc.allocate_string(name));
            textureNode->append_attribute(attr);
            attr = m_mtlDoc.allocate_attribute(MaterialExport::g_fileString, m_mtlDoc.allocate_string(fileName));
            textureNode->append_attribute(attr);

            return textureNode;
        }


    }//GFxFramework
}//AZ
