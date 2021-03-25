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

struct aiMaterial;


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
                Normal,
                Metallic,
                Roughness,
                AmbientOcclusion,
                Emissive,
                BaseColor
            };

            MaterialWrapper(fbxsdk::FbxSurfaceMaterial* fbxMaterial);
            MaterialWrapper(aiMaterial* assImpmaterial);
            virtual ~MaterialWrapper();

            fbxsdk::FbxSurfaceMaterial* GetFbxMaterial();
            aiMaterial* GetAssImpMaterial();

            virtual AZStd::string GetName() const;
            virtual AZ::u64 GetUniqueId() const;
            virtual AZStd::string GetTextureFileName(MaterialMapType textureType) const;
            virtual AZ::Vector3 GetDiffuseColor() const;
            virtual AZ::Vector3 GetSpecularColor() const;
            virtual AZ::Vector3 GetEmissiveColor() const;
            virtual float GetOpacity() const;
            virtual float GetShininess() const;

        protected:
            fbxsdk::FbxSurfaceMaterial* m_fbxMaterial = nullptr;
            aiMaterial* m_assImpMaterial = nullptr;
        };
    } // namespace SDKMaterial
} // namespace AZ
