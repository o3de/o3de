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

#include <Atom/RHI.Reflect/ShaderStages.h>
#include <Atom/RHI.Reflect/RenderStates.h>
#include <Atom/RHI.Reflect/ShaderStageFunction.h>

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <Atom/RPI.Reflect/Shader/ShaderInputContract.h>
#include <Atom/RPI.Reflect/Shader/ShaderOutputContract.h>

namespace AZ
{
    namespace RPI
    {
        //! A ShaderVariantAsset contains the shader byte code for each shader stage (Vertex, Fragment, Tessellation, etc) for a given RHI::APIType (dx12, vulkan, metal, etc).
        //! One independent file per RHI::APIType.
        class ShaderVariantAsset final
            : public Data::AssetData
        {
            friend class ShaderVariantAssetHandler;
            friend class ShaderVariantAssetCreator;

        public:
            AZ_RTTI(ShaderVariantAsset, "{9F4D654B-4439-4C61-8DCD-F1C7C5560768}", Data::AssetData);

            static void Reflect(ReflectContext* context);

            static constexpr const char* Extension = "azshadervariant";
            static constexpr const char* DisplayName = "ShaderVariant";
            static constexpr const char* Group = "Shader";

            //! @rhiApiUniqueIndex comes from RHI::Factory::GetAPIUniqueIndex()
            static uint32_t GetAssetSubId(uint32_t rhiApiUniqueIndex, ShaderVariantStableId variantStableId);

            ShaderVariantAsset() = default;
            ~ShaderVariantAsset() = default;

            AZ_DISABLE_COPY_MOVE(ShaderVariantAsset);

            RPI::ShaderVariantStableId GetStableId() const { return m_stableId;  }

            const ShaderVariantId& GetShaderVariantId() const { return m_shaderVariantId; }

            //! Returns the shader stage function associated with the provided stage enum value.
            const RHI::ShaderStageFunction* GetShaderStageFunction(RHI::ShaderStage shaderStage) const;

            //! Returns the ShaderInputContract which describes which inputs the shader requires
            const ShaderInputContract& GetInputContract() const;

            //! Returns the ShaderOuputContract which describes which outputs the shader requires
            const ShaderOutputContract& GetOutputContract() const;

            //! Returns the render states for the draw pipeline. Only used for draw pipelines.
            const RHI::RenderStates& GetRenderStates() const;

            //! Returns whether the variant is fully baked variant (all options are static branches), or false if the
            //! variant uses dynamic branches for some shader options.
            //! If the shader variant is not fully baked, the ShaderVariantKeyFallbackValue must be correctly set when drawing.
            bool IsFullyBaked() const;

            //! Return the timestamp when the associated ShaderAsset was built.
            //! This is used to synchronize versions of the ShaderAsset and ShaderVariantAsset, especially during hot-reload.
            AZStd::sys_time_t GetShaderAssetBuildTimestamp() const;

        private:
            //! Called by asset creators to assign the asset to a ready state.
            void SetReady();
            bool FinalizeAfterLoad();

            //! See AZ::RPI::ShaderVariantListSourceData::VariantInfo::m_stableId for details.
            RPI::ShaderVariantStableId m_stableId;

            ShaderVariantId m_shaderVariantId;
            ShaderInputContract m_inputContract;
            ShaderOutputContract m_outputContract;
            bool m_isFullyBaked = false;

            //! The only reason the render states are part of the ShaderVariantAsset
            //! instead of ShaderAsset is because the m_blendStage.m_targets[i]
            //! get populated by the number of colorAttachementCounts which comes from
            //! the output contract which is a per variant piece of data.
            RHI::RenderStates m_renderStates;

            AZStd::array<RHI::Ptr<RHI::ShaderStageFunction>, RHI::ShaderStageCount> m_functionsByStage;

            //! Used to synchronize versions of the ShaderAsset and ShaderVariantAsset, especially during hot-reload.
            AZStd::sys_time_t m_shaderAssetBuildTimestamp = 0;
        };

        class ShaderVariantAssetHandler final
            : public AssetHandler<ShaderVariantAsset>
        {
            using Base = AssetHandler<ShaderVariantAsset>;
        public:
            ShaderVariantAssetHandler() = default;

        private:
            LoadResult LoadAssetData(const Data::Asset<Data::AssetData>& asset, AZStd::shared_ptr<Data::AssetDataStream> stream, const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
            bool PostLoadInit(const Data::Asset<Data::AssetData>& asset);
        };

    } // namespace RPI

} // namespace AZ
