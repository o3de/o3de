// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include "GltfPbrMaterial.h"
#include "gltfHelpers.h"

void AddTextureIfExists(const json::object_t &material, std::map<std::string, int> &textureIds, char *texturePath, char *textureName)
{
    int id = GetElementInt(material, texturePath, -1);
    if (id >= 0)
    {
        textureIds[textureName] = id;
    }
}

void ProcessMaterials(const json::object_t &material, PBRMaterialParameters *tfmat, std::map<std::string, int> &textureIds)
{
    // Load material constants
    //
    json::array_t ones = { 1.0, 1.0, 1.0, 1.0 };
    json::array_t zeroes = { 0.0, 0.0, 0.0, 0.0 };
    tfmat->m_doubleSided = GetElementBoolean(material, "doubleSided", false);
    tfmat->m_blending = GetElementString(material, "alphaMode", "OPAQUE") == "BLEND";
    tfmat->m_params.m_emissiveFactor = GetVector(GetElementJsonArray(material, "emissiveFactor", zeroes));

    tfmat->m_defines["DEF_doubleSided"] = std::to_string(tfmat->m_doubleSided ? 1 : 0);
    tfmat->m_defines["DEF_alphaCutoff"] = std::to_string(GetElementFloat(material, "alphaCutoff", 0.5));
    tfmat->m_defines["DEF_alphaMode_" + GetElementString(material, "alphaMode", "OPAQUE")] = std::to_string(1);

    // look for textures and store their IDs in a map 
    //
    AddTextureIfExists(material, textureIds, "normalTexture/index", "normalTexture");
    AddTextureIfExists(material, textureIds, "emissiveTexture/index", "emissiveTexture");
    AddTextureIfExists(material, textureIds, "occlusionTexture/index", "occlusionTexture");

    tfmat->m_defines["ID_normalTexCoord"] = std::to_string(GetElementInt(material, "normalTexture/texCoord", 0));
    tfmat->m_defines["ID_emissiveTexCoord"] = std::to_string(GetElementInt(material, "emissiveTexture/texCoord", 0));
    tfmat->m_defines["ID_occlusionTexCoord"] = std::to_string(GetElementInt(material, "occlusionTexture/texCoord", 0));

    // If using pbrMetallicRoughness
    //
    auto pbrMetallicRoughnessIt = material.find("pbrMetallicRoughness");
    if (pbrMetallicRoughnessIt != material.end())
    {
        const json::object_t &pbrMetallicRoughness = pbrMetallicRoughnessIt->second;

        tfmat->m_pbrType = PBRMaterialParameters::MATERIAL_METALLIC_ROUGHNESS;
        float metallicFactor = GetElementFloat(pbrMetallicRoughness, "metallicFactor", 1.0);
        float roughnessFactor = GetElementFloat(pbrMetallicRoughness, "roughnessFactor", 1.0);
        tfmat->m_params.m_metallicRoughnessValues = XMVectorSet(metallicFactor, roughnessFactor, 0, 0);
        tfmat->m_params.m_baseColorFactor = GetVector(GetElementJsonArray(pbrMetallicRoughness, "baseColorFactor", ones));

        AddTextureIfExists(pbrMetallicRoughness, textureIds, "baseColorTexture/index", "baseColorTexture");
        AddTextureIfExists(pbrMetallicRoughness, textureIds, "metallicRoughnessTexture/index", "metallicRoughnessTexture");
        tfmat->m_defines["ID_baseTexCoord"] = std::to_string(GetElementInt(pbrMetallicRoughness, "baseColorTexture/texCoord", 0));
        tfmat->m_defines["ID_metallicRoughnessTextCoord"] = std::to_string(GetElementInt(pbrMetallicRoughness, "metallicRoughnessTexture/texCoord", 0));
        tfmat->m_defines["MATERIAL_METALLICROUGHNESS"] = "1";
    }
    else
    {
        // If using KHR_materials_pbrSpecularGlossiness
        //
        auto extensionsIt = material.find("extensions");
        if (extensionsIt != material.end())
        {
            const json::object_t &extensions = extensionsIt->second;
            auto KHR_materials_pbrSpecularGlossinessIt = extensions.find("KHR_materials_pbrSpecularGlossiness");
            if (KHR_materials_pbrSpecularGlossinessIt != extensions.end())
            {
                const json::object_t &pbrSpecularGlossiness = KHR_materials_pbrSpecularGlossinessIt->second;

                tfmat->m_pbrType = PBRMaterialParameters::MATERIAL_SPECULAR_GLOSSINESS;
                float glossiness = GetElementFloat(pbrSpecularGlossiness, "glossinessFactor", 1.0);
                tfmat->m_params.m_DiffuseFactor = GetVector(GetElementJsonArray(pbrSpecularGlossiness, "diffuseFactor", ones));
                tfmat->m_params.m_specularGlossinessFactor = XMVectorSetW(GetVector(GetElementJsonArray(pbrSpecularGlossiness, "specularFactor", ones)), glossiness);


                AddTextureIfExists(pbrSpecularGlossiness, textureIds, "diffuseTexture/index", "diffuseTexture");
                AddTextureIfExists(pbrSpecularGlossiness, textureIds, "specularGlossinessTexture/index", "specularGlossinessTexture");
                tfmat->m_defines["ID_diffuseTextCoord"] = std::to_string(GetElementInt(pbrSpecularGlossiness, "diffuseTexture/texCoord", 0));
                tfmat->m_defines["ID_specularGlossinessTextCoord"] = std::to_string(GetElementInt(pbrSpecularGlossiness, "specularGlossinessTexture/texCoord", 0));
                tfmat->m_defines["MATERIAL_SPECULARGLOSSINESS"] = "1";
            }
        }
    }
}

