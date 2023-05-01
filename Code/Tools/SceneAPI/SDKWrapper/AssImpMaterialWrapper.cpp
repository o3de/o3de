/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <assimp/scene.h>
#include <assimp/material.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Sha1.h>
#include <AzCore/Debug/Trace.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SDKWrapper/AssImpMaterialWrapper.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace AssImpSDKWrapper
    {

        AssImpMaterialWrapper::AssImpMaterialWrapper(aiMaterial* aiMaterial)
            :m_assImpMaterial(aiMaterial)
        {
            AZ_Assert(aiMaterial, "Asset Importer Material cannot be null");
        }

        aiMaterial* AssImpMaterialWrapper::GetAssImpMaterial() const
        {
            return m_assImpMaterial;
        }

        AZStd::string AssImpMaterialWrapper::GetName() const
        {
            return m_assImpMaterial->GetName().C_Str();
        }

        AZ::u64 AssImpMaterialWrapper::GetUniqueId() const
        {
            AZStd::string fingerprintString;
            fingerprintString.append(GetName());
            AZStd::string extraInformation = AZStd::string::format("%i%i%i%i%i%i", m_assImpMaterial->GetTextureCount(aiTextureType_DIFFUSE),
                m_assImpMaterial->GetTextureCount(aiTextureType_SPECULAR), m_assImpMaterial->GetTextureCount(aiTextureType_NORMALS), m_assImpMaterial->GetTextureCount(aiTextureType_SHININESS),
                m_assImpMaterial->GetTextureCount(aiTextureType_AMBIENT), m_assImpMaterial->GetTextureCount(aiTextureType_EMISSIVE));
            fingerprintString.append(extraInformation);
            AZ::Sha1 sha;
            sha.ProcessBytes(AZStd::as_bytes(AZStd::span(fingerprintString)));
            AZ::u32 digest[5]; //sha1 populate a 5 element array of AZ:u32
            sha.GetDigest(digest);
            return (static_cast<AZ::u64>(digest[0]) << 32) | digest[1];
        }

        AZ::Vector3 AssImpMaterialWrapper::GetDiffuseColor() const
        {
            aiColor3D color(1.f, 1.f, 1.f);
            if (m_assImpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn::aiReturn_FAILURE)
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Unable to get diffuse property from material %.*s. Using default.\n", AZ_STRING_ARG(GetName()));
            }
            return AZ::Vector3(color.r, color.g, color.b);
        }

        AZ::Vector3 AssImpMaterialWrapper::GetSpecularColor() const
        {
            aiColor3D color(0.f, 0.f, 0.f);
            if (m_assImpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == aiReturn::aiReturn_FAILURE)
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Unable to get specular property from material %.*s. Using default.\n", AZ_STRING_ARG(GetName()));
            }
            return AZ::Vector3(color.r, color.g, color.b);
        }

        AZ::Vector3 AssImpMaterialWrapper::GetEmissiveColor() const
        {
            aiColor3D color(0.f, 0.f, 0.f);
            if (m_assImpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color) == aiReturn::aiReturn_FAILURE)
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Unable to get emissive property from material %.*s. Using default.\n", AZ_STRING_ARG(GetName()));
            }
            return AZ::Vector3(color.r, color.g, color.b);
        }

        float AssImpMaterialWrapper::GetOpacity() const
        {
            float opacity = 1.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_OPACITY, opacity) == aiReturn::aiReturn_FAILURE)
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Unable to get opacity from material %.*s. Using default.\n", AZ_STRING_ARG(GetName()));
            }
            return opacity;
        }

        float AssImpMaterialWrapper::GetShininess() const
        {
            float shininess = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_SHININESS, shininess) == aiReturn::aiReturn_FAILURE)
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Unable to get shininess from material %.*s. Using default.\n", AZ_STRING_ARG(GetName()));
            }
            return shininess;
        }

        AZStd::optional<bool> AssImpMaterialWrapper::GetUseColorMap() const
        {
            // AssImp stores values as floats, so load as float and convert to bool.
            float useMap = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_USE_COLOR_MAP, useMap) != aiReturn::aiReturn_FAILURE)
            {
                return useMap != 0.0f;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<AZ::Vector3> AssImpMaterialWrapper::GetBaseColor() const
        {
            aiColor3D color(1.f, 1.f, 1.f);
            if (m_assImpMaterial->Get(AI_MATKEY_BASE_COLOR, color) != aiReturn::aiReturn_FAILURE)
            {
                return AZ::Vector3(color.r, color.g, color.b);
            }
            return AZStd::nullopt;
        }

        AZStd::optional<bool> AssImpMaterialWrapper::GetUseMetallicMap() const
        {
            // AssImp stores values as floats, so load as float and convert to bool.
            float useMap = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_USE_METALLIC_MAP, useMap) != aiReturn::aiReturn_FAILURE)
            {
                return useMap != 0.0f;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<float> AssImpMaterialWrapper::GetMetallicFactor() const
        {
            float metallic = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_METALLIC_FACTOR, metallic) != aiReturn::aiReturn_FAILURE)
            {
                return metallic;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<bool> AssImpMaterialWrapper::GetUseRoughnessMap() const
        {
            // AssImp stores values as floats, so load as float and convert to bool.
            float useMap = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_USE_ROUGHNESS_MAP, useMap) != aiReturn::aiReturn_FAILURE)
            {
                return useMap != 0.0f;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<float> AssImpMaterialWrapper::GetRoughnessFactor() const
        {
            float roughness = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) != aiReturn::aiReturn_FAILURE)
            {
                return roughness;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<bool> AssImpMaterialWrapper::GetUseEmissiveMap() const
        {
            // AssImp stores values as floats, so load as float and convert to bool.
            float useMap = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_USE_EMISSIVE_MAP, useMap) != aiReturn::aiReturn_FAILURE)
            {
                return useMap != 0.0f;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<float> AssImpMaterialWrapper::GetEmissiveIntensity() const
        {
            float emissiveIntensity = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissiveIntensity) != aiReturn::aiReturn_FAILURE)
            {
                return emissiveIntensity;
            }
            return AZStd::nullopt;
        }

        AZStd::optional<bool> AssImpMaterialWrapper::GetUseAOMap() const
        {
            // AssImp stores values as floats, so load as float and convert to bool.
            float useMap = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_USE_AO_MAP, useMap) != aiReturn::aiReturn_FAILURE)
            {
                return useMap != 0.0f;
            }
            return AZStd::nullopt;
        }

        AZStd::string AssImpMaterialWrapper::GetTextureFileName(MaterialMapType textureType) const
        {
            /// Engine currently doesn't support multiple textures. Right now we only use first texture.
            unsigned int textureIndex = 0;
            aiString absTexturePath;
            switch (textureType)
            {
            case MaterialMapType::Diffuse:
                if (m_assImpMaterial->GetTextureCount(aiTextureType_DIFFUSE) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(aiTextureType_DIFFUSE, textureIndex, &absTexturePath);
                }
                break;
            case MaterialMapType::Specular:
                if (m_assImpMaterial->GetTextureCount(aiTextureType_SPECULAR) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(aiTextureType_SPECULAR, textureIndex, &absTexturePath);
                }
                break;
            case MaterialMapType::Bump:
                if (m_assImpMaterial->GetTextureCount(aiTextureType_HEIGHT) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(aiTextureType_HEIGHT, textureIndex, &absTexturePath);
                }
                break;
            case MaterialMapType::Normal:
                if (m_assImpMaterial->GetTextureCount(aiTextureType_NORMALS) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(aiTextureType_NORMALS, textureIndex, &absTexturePath);
                }
                else if (m_assImpMaterial->GetTextureCount(aiTextureType_NORMAL_CAMERA) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(aiTextureType_NORMAL_CAMERA, textureIndex, &absTexturePath);
                }
                break;
            case MaterialMapType::Metallic:
                if (m_assImpMaterial->GetTextureCount(aiTextureType_METALNESS) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(aiTextureType_METALNESS, textureIndex, &absTexturePath);
                }
                break;
            case MaterialMapType::Roughness:
                if (m_assImpMaterial->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, textureIndex, &absTexturePath);
                }
                break;
            case MaterialMapType::AmbientOcclusion:
                if (m_assImpMaterial->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(aiTextureType_AMBIENT_OCCLUSION, textureIndex, &absTexturePath);
                }
                break;
            case MaterialMapType::Emissive:
                if (m_assImpMaterial->GetTextureCount(aiTextureType_EMISSION_COLOR) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(aiTextureType_EMISSION_COLOR, textureIndex, &absTexturePath);
                }
                break;
            case MaterialMapType::BaseColor:
                if (m_assImpMaterial->GetTextureCount(aiTextureType_BASE_COLOR) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(aiTextureType_BASE_COLOR, textureIndex, &absTexturePath);
                }
                // If the base color texture isn't available, fall back to using the diffuse texture.
                // Before PBR support was added, the renderer just defaulted to using the diffuse texture
                // in the base color texture property of the material.
                else if (m_assImpMaterial->GetTextureCount(aiTextureType_DIFFUSE) > textureIndex)
                {
                    m_assImpMaterial->GetTexture(aiTextureType_DIFFUSE, textureIndex, &absTexturePath);
                }
                break;
            default:
                AZ_TraceContext("Unknown value", aznumeric_cast<int>(textureType));
                AZ_TracePrintf(SceneAPI::Utilities::WarningWindow, "Unrecognized MaterialMapType retrieved");
                return AZStd::string();
            }
            return AZStd::string(absTexturePath.C_Str());
        }
    } // namespace AssImpSDKWrapper
} // namespace AZ
