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
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Vector3.h>

namespace fbxsdk
{
    class FbxSurfaceMaterial;
}

#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
struct aiMaterial;
#endif

namespace AZ
{
    namespace SDKMaterial
    {
        class MaterialWrapper
        {
        public:
            AZ_RTTI(MaterialWrapper, "{3B83DEE4-FF59-4AB3-B4F1-14EEE22339F3}");
            enum class MaterialMapType
            {
                Diffuse,
                Specular,
                Bump,
                Normal
            };

            MaterialWrapper(fbxsdk::FbxSurfaceMaterial* fbxMaterial);
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            MaterialWrapper(aiMaterial* assImpmaterial);
#endif
            virtual ~MaterialWrapper();

            fbxsdk::FbxSurfaceMaterial* GetFbxMaterial();
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            aiMaterial* GetAssImpMaterial();
#endif

            virtual AZStd::string GetName() const;
            virtual uint64_t GetUniqueId() const;
            virtual AZStd::string GetTextureFileName(MaterialMapType textureType) const;
            virtual AZ::Vector3 GetDiffuseColor() const;
            virtual AZ::Vector3 GetSpecularColor() const;
            virtual AZ::Vector3 GetEmissiveColor() const;
            virtual float GetOpacity() const;
            virtual float GetShininess() const;

        protected:
            fbxsdk::FbxSurfaceMaterial* m_fbxMaterial = nullptr;
#ifdef ASSET_IMPORTER_SDK_SUPPORTED_TRAIT
            aiMaterial* m_assImpMaterial = nullptr;
#endif
        };
    } // namespace SDKMaterial
} // namespace AZ
