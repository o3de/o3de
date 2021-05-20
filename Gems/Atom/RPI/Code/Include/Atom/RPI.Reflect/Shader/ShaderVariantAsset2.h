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

#include <AzCore/std/containers/array.h>

#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

namespace AZ
{
    namespace RPI
    {
        //! A ShaderVariantAsset2 contains the shader byte code for each shader stage (Vertex, Fragment, Tessellation, etc) for a given RHI::APIType (dx12, vulkan, metal, etc).
        //! One independent file per RHI::APIType.
        class ShaderVariantAsset2 final
            : public Data::AssetData
        {
            friend class ShaderVariantAssetHandler2;
            friend class ShaderVariantAssetCreator2;

        public:
            AZ_RTTI(ShaderVariantAsset2, "{51BED815-36D8-410E-90F0-1FA9FF765FBA}", Data::AssetData);

            static void Reflect(ReflectContext* context);

            static constexpr const char* Extension = "azshadervariant2";
            static constexpr const char* DisplayName = "ShaderVariant";
            static constexpr const char* Group = "Shader";

            static constexpr uint32_t ShaderVariantAsset2SubProductType = 1;
            //! @rhiApiUniqueIndex comes from RHI::Factory::GetAPIUniqueIndex()
            //! @subProductType is always 0 for a regular ShaderVariantAsset2, for all other debug subProducts created
            //!                by ShaderVariantAssetBuilder2 this is 1+.
            static uint32_t MakeAssetProductSubId(
                uint32_t rhiApiUniqueIndex, uint32_t supervariantIndex, ShaderVariantStableId variantStableId,
                uint32_t subProductType = ShaderVariantAsset2SubProductType);

            ShaderVariantAsset2() = default;
            ~ShaderVariantAsset2() = default;

            AZ_DISABLE_COPY_MOVE(ShaderVariantAsset2);

            RPI::ShaderVariantStableId GetStableId() const { return m_stableId;  }

            const ShaderVariantId& GetShaderVariantId() const { return m_shaderVariantId; }

            //! Returns the shader stage function associated with the provided stage enum value.
            const RHI::ShaderStageFunction* GetShaderStageFunction(RHI::ShaderStage shaderStage) const;

            //! Returns whether the variant is fully baked variant (all options are static branches), or false if the
            //! variant uses dynamic branches for some shader options.
            //! If the shader variant is not fully baked, the ShaderVariantKeyFallbackValue must be correctly set when drawing.
            bool IsFullyBaked() const;

            //! Return the timestamp when this asset was built, and it must be >= than the timestamp of the main ShaderAsset.
            //! This is used to synchronize versions of the ShaderAsset and ShaderVariantAsset2, especially during hot-reload.
            AZStd::sys_time_t GetBuildTimestamp() const;

            bool IsRootVariant() const { return m_stableId == RPI::RootShaderVariantStableId; } 

        private:
            //! Called by asset creators to assign the asset to a ready state.
            void SetReady();
            bool FinalizeAfterLoad();

            //! See AZ::RPI::ShaderVariantListSourceData::VariantInfo::m_stableId for details.
            RPI::ShaderVariantStableId m_stableId;

            ShaderVariantId m_shaderVariantId;

            bool m_isFullyBaked = false;

            AZStd::array<RHI::Ptr<RHI::ShaderStageFunction>, RHI::ShaderStageCount> m_functionsByStage;

            //! Used to synchronize versions of the ShaderAsset and ShaderVariantAsset2, especially during hot-reload.
            AZStd::sys_time_t m_buildTimestamp = 0;
        };

        class ShaderVariantAssetHandler2 final
            : public AssetHandler<ShaderVariantAsset2>
        {
            using Base = AssetHandler<ShaderVariantAsset2>;
        public:
            ShaderVariantAssetHandler2() = default;

        private:
            LoadResult LoadAssetData(const Data::Asset<Data::AssetData>& asset, AZStd::shared_ptr<Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
            bool PostLoadInit(const Data::Asset<Data::AssetData>& asset);
        };

    } // namespace RPI

} // namespace AZ
