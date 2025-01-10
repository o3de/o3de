/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Reflect/AssetCreator.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantTreeAsset.h>
#include <Atom/RPI.Edit/Configuration.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>

namespace AZ
{
    namespace RPI
    {
        //! The "builder" pattern class that creates a ShaderVariantTreeAsset.
        class ATOM_RPI_EDIT_API ShaderVariantTreeAssetCreator final
            : public AssetCreator<ShaderVariantTreeAsset>

        {
        public:
            //! This helper method is called early on by the ShaderVariantAssetBuilder to make sure the
            //! StableIds are unique and not-zero (zero is reserved for the root variant).
            static AZ::Outcome<void, AZStd::string> ValidateStableIdsAreUnique(const AZStd::vector<ShaderVariantListSourceData::VariantInfo>& shaderVariantList);

            //! Begins construction of the shader variant tree asset.
            //! @param assetId The "initial" assetId that the resulting ShaderVariantTreeAsset will get.
            //!        "initial" was quoted because in the end the asset processor will assign another assetId
            //!        because on the UUID of the source asset (a *.shadervariantlist file) and the product subid
            //!        that gets assign when returning the Job Response.
            void Begin(const AZ::Data::AssetId& assetId);

            void SetShaderOptionGroupLayout(const RPI::ShaderOptionGroupLayout& shaderOptionGroupLayout);

            //! None of the VariantInfos may have StableId == 0. StableId 0 is reserved for the root variant, which is implicit.
            //! The ShaderVariantAssetBuilder calls ValidateStableIdsAreUnique() in advance to validate this requirement.
            void SetVariantInfos(const AZStd::vector<ShaderVariantListSourceData::VariantInfo>& variantInfos);

            //! Finalizes and assigns ownership of the asset to result, if successful. 
            //! Otherwise false is returned and result is left untouched.
            bool End(Data::Asset<ShaderVariantTreeAsset>& result);

        private:

            struct ShaderVariantIdWithStableId
            {
                ShaderVariantId m_shaderVariantId;
                ShaderVariantStableId m_stableId;
            };

            bool EndInternal(Data::Asset<ShaderVariantTreeAsset>& result);
            bool BuildTree(const AZStd::vector<ShaderVariantIdWithStableId>& shaderVariantIdsWithStableId);

            const RPI::ShaderOptionGroupLayout* m_shaderOptionGroupLayout;
            AZStd::vector<ShaderVariantListSourceData::VariantInfo> m_variantInfos;
        };
    } // namespace RPI
} // namespace AZ
