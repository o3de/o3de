/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <SceneAPI/SDKWrapper/MaterialWrapper.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace AssImpSDKWrapper
    {
        class AssImpMaterialWrapper : public SDKMaterial::MaterialWrapper
        {
        public:
            AZ_RTTI(AssImpMaterialWrapper, "{66992628-CFCE-441B-8849-9344A49AFAC9}", SDKMaterial::MaterialWrapper);
            AssImpMaterialWrapper(aiMaterial* aiMaterial);
            ~AssImpMaterialWrapper() override = default;
            aiMaterial* GetAssImpMaterial() const;
            AZStd::string GetName() const override;
            AZ::u64 GetUniqueId() const override;
            AZ::Vector3 GetDiffuseColor() const override;
            AZ::Vector3 GetSpecularColor() const override;
            AZ::Vector3 GetEmissiveColor() const override;
            float GetOpacity() const override;
            float GetShininess() const override;
            AZStd::string GetTextureFileName(MaterialMapType textureType) const override;

            AZStd::optional<bool> GetUseColorMap() const;
            AZStd::optional<AZ::Vector3> GetBaseColor() const;
            AZStd::optional<bool> GetUseMetallicMap() const;
            AZStd::optional<float> GetMetallicFactor() const;
            AZStd::optional<bool> GetUseRoughnessMap() const;
            AZStd::optional<float> GetRoughnessFactor() const;
            AZStd::optional<bool> GetUseEmissiveMap() const;
            AZStd::optional<float> GetEmissiveIntensity() const;
            AZStd::optional<bool> GetUseAOMap() const;

        protected:
            aiMaterial* m_assImpMaterial = nullptr;
        };
    } // namespace AssImpSDKWrapper
}// namespace AZ
