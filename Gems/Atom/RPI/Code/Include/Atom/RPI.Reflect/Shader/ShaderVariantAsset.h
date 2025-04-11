/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/array.h>

#include <Atom/RHI.Reflect/ShaderStageFunction.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>

namespace AZ
{
    namespace RPI
    {
        //! A ShaderVariantAsset contains the shader byte code for each shader stage (Vertex, Fragment, Tessellation, etc) for a given RHI::APIType (dx12, vulkan, metal, etc).
        //! One independent file per RHI::APIType.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API ShaderVariantAsset final
            : public Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class ShaderVariantAssetHandler;
            friend class ShaderVariantAssetCreator;

        public:
            AZ_CLASS_ALLOCATOR(ShaderVariantAsset , SystemAllocator)
            AZ_RTTI(ShaderVariantAsset, "{51BED815-36D8-410E-90F0-1FA9FF765FBA}", Data::AssetData);

            static void Reflect(ReflectContext* context);

            static constexpr const char* Extension = "azshadervariant";
            static constexpr const char* DisplayName = "ShaderVariant";
            static constexpr const char* Group = "Shader";

            static constexpr uint32_t ShaderVariantAssetSubProductType = 1;
            //! @rhiApiUniqueIndex comes from RHI::Factory::GetAPIUniqueIndex()
            //! @subProductType is always 0 for a regular ShaderVariantAsset, for all other debug subProducts created
            //!                by ShaderVariantAssetBuilder this is 1+.
            static uint32_t MakeAssetProductSubId(
                uint32_t rhiApiUniqueIndex, uint32_t supervariantIndex, ShaderVariantStableId variantStableId,
                uint32_t subProductType = 0);

            ShaderVariantAsset() = default;
            ~ShaderVariantAsset() = default;

            AZ_DISABLE_COPY_MOVE(ShaderVariantAsset);

            RPI::ShaderVariantStableId GetStableId() const { return m_stableId;  }

            const ShaderVariantId& GetShaderVariantId() const { return m_shaderVariantId; }
            uint32_t GetSupervariantIndex() const;

            //! Returns the shader stage function associated with the provided stage enum value.
            const RHI::ShaderStageFunction* GetShaderStageFunction(RHI::ShaderStage shaderStage) const;

            //! Returns whether the variant is fully baked variant (all options are static branches), or false if the
            //! variant uses dynamic branches for some shader options.
            //! If the shader variant is not fully baked or fully specialized, the ShaderVariantKeyFallbackValue must be correctly set when drawing.
            bool IsFullyBaked() const;

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


        };

        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API ShaderVariantAssetHandler final
            : public AssetHandler<ShaderVariantAsset>
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            using Base = AssetHandler<ShaderVariantAsset>;
        public:
            ShaderVariantAssetHandler() = default;

        private:
            LoadResult LoadAssetData(const Data::Asset<Data::AssetData>& asset, AZStd::shared_ptr<Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
            bool PostLoadInit(const Data::Asset<Data::AssetData>& asset);
        };

    } // namespace RPI

} // namespace AZ
