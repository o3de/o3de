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

#include <fbxsdk.h>
#include <AzCore/std/string/string.h>
#include <SceneAPI/SDKWrapper/MaterialWrapper.h>

namespace AZ
{
    namespace FbxSDKWrapper
    {
        class FbxMaterialWrapper : public SDKMaterial::MaterialWrapper
        {
        public:
            AZ_RTTI(FbxMaterialWrapper, "{227582F6-93BC-4E44-823E-FB1D631443A7}", SDKMaterial::MaterialWrapper);

            FbxMaterialWrapper(FbxSurfaceMaterial* fbxMaterial);
            ~FbxMaterialWrapper() override = default;

            AZStd::string GetName() const override;
            virtual AZStd::string GetTextureFileName(const char* textureType) const;
            virtual AZStd::string GetTextureFileName(const AZStd::string& textureType) const;
            AZStd::string GetTextureFileName(MaterialMapType textureType) const override;

            AZ::Vector3 GetDiffuseColor() const override;
            AZ::Vector3 GetSpecularColor() const override;
            AZ::Vector3 GetEmissiveColor() const override;
            float GetOpacity() const override;
            float GetShininess() const override;
            uint64_t GetUniqueId() const override;
        };
    } // namespace FbxSDKWrapper
} // namespace AZ
