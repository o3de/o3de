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

#include <assimp/scene.h>
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
            :SDKMaterial::MaterialWrapper(aiMaterial)
        {
            AZ_Assert(aiMaterial, "Asset Importer Material cannot be null");
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
            sha.ProcessBytes(fingerprintString.data(), fingerprintString.size());
            AZ::u32 digest[5]; //sha1 populate a 5 element array of AZ:u32
            sha.GetDigest(digest);
            return (static_cast<AZ::u64>(digest[0]) << 32) | digest[1];
        }

        AZ::Vector3 AssImpMaterialWrapper::GetDiffuseColor() const
        {
            aiColor3D color(1.f, 1.f, 1.f);
            if (m_assImpMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn::aiReturn_FAILURE)
            {
                AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Unable to get diffuse property from the material. Using default.");
            }
            return AZ::Vector3(color.r, color.g, color.b);
        }

        AZ::Vector3 AssImpMaterialWrapper::GetSpecularColor() const
        {
            aiColor3D color(0.f, 0.f, 0.f);
            if (m_assImpMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color) == aiReturn::aiReturn_FAILURE)
            {
                AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Unable to get specular property from the material. Using default.");
            }
            return AZ::Vector3(color.r, color.g, color.b);
        }

        AZ::Vector3 AssImpMaterialWrapper::GetEmissiveColor() const
        {
            aiColor3D color(0.f, 0.f, 0.f);
            if (m_assImpMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color) == aiReturn::aiReturn_FAILURE)
            {
                AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Unable to get emissive property from the material. Using default.");
            }
            return AZ::Vector3(color.r, color.g, color.b);
        }

        float AssImpMaterialWrapper::GetOpacity() const
        {
            float opacity = 1.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_OPACITY, opacity) == aiReturn::aiReturn_FAILURE)
            {
                AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Unable to get opacity from the material. Using default.");
            }
            return opacity;
        }

        float AssImpMaterialWrapper::GetShininess() const
        {
            float shininess = 0.0f;
            if (m_assImpMaterial->Get(AI_MATKEY_SHININESS, shininess) == aiReturn::aiReturn_FAILURE)
            {
                AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Unable to get shininess from the material. Using default.");
            }
            return shininess;
        }

        AZStd::string AssImpMaterialWrapper::GetTextureFileName(MaterialMapType textureType) const
        {
            /// Engine currently doesn't support multiple textures. Right now we only use first texture.
            int textureIndex = 0;
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
