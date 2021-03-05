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

#include <SceneAPI/SDKWrapper/MaterialWrapper.h>

namespace AZ
{
    namespace SDKMaterial
    {
        MaterialWrapper::MaterialWrapper(fbxsdk::FbxSurfaceMaterial* fbxMaterial)
            :m_fbxMaterial(fbxMaterial)
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            , m_assImpMaterial(nullptr)
#endif
        {
        }
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
        MaterialWrapper::MaterialWrapper(aiMaterial* assImpMaterial)
            :m_fbxMaterial(nullptr)
            , m_assImpMaterial(assImpMaterial)
        {
        }
#endif

        MaterialWrapper::~MaterialWrapper()
        {
            m_fbxMaterial = nullptr;
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            m_assImpMaterial = nullptr;
#endif
        }

        fbxsdk::FbxSurfaceMaterial* MaterialWrapper::GetFbxMaterial()
        {
            return m_fbxMaterial;
        }
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
        aiMaterial* MaterialWrapper::GetAssImpMaterial()
        {
            return m_assImpMaterial;
        }
#endif
        AZStd::string MaterialWrapper::GetName() const
        {
            return AZStd::string();
        }

        uint64_t MaterialWrapper::GetUniqueId() const
        {
            return uint64_t();
        }

        AZ::Vector3 MaterialWrapper::GetDiffuseColor() const
        {
            return {};
        }

        AZ::Vector3 MaterialWrapper::GetSpecularColor() const
        {
            return {};
        }

        AZ::Vector3 MaterialWrapper::GetEmissiveColor() const
        {
            return {};
        }

        float MaterialWrapper::GetOpacity() const
        {
            return {};
        }
        float MaterialWrapper::GetShininess() const
        {
            return {};
        }

        AZStd::string MaterialWrapper::GetTextureFileName([[maybe_unused]]MaterialMapType textureType) const
        {
            return AZStd::string();
        }

    }// namespace SDKMaterial
}//namespace AZ
