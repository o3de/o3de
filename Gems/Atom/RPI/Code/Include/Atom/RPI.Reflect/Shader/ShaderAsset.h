/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/EBus/Event.h>

#include <Atom/RPI.Public/AssetInitBus.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroupLayout.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantTreeAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderInputContract.h>
#include <Atom/RPI.Reflect/Shader/ShaderCommonTypes.h>
#include <Atom/RPI.Reflect/Shader/ShaderVariantKey.h>
#include <Atom/RPI.Reflect/Shader/IShaderVariantFinder.h>

#include <Atom/RHI/PipelineStateDescriptor.h>

#include <Atom/RHI.Reflect/ShaderStages.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>

#include <limits>

namespace AZ
{
    namespace RPI
    {
        using ShaderResourceGroupLayoutList = AZStd::fixed_vector<RHI::Ptr<RHI::ShaderResourceGroupLayout>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax>;

        class ShaderAsset final
            : public Data::AssetData
            , public ShaderVariantFinderNotificationBus::Handler
            , public Data::AssetBus::Handler
            , public AssetInitBus::Handler
        {
            friend class ShaderAssetCreator;
            friend class ShaderAssetHandler;
            friend class ShaderAssetTester;
        public:
            AZ_RTTI(ShaderAsset, "{892C4FF2-0B56-417D-AF2E-6FF04D6D6EA9}", Data::AssetData);

            static void Reflect(ReflectContext* context);

            static const char* DisplayName;
            static const char* Extension;
            static const char* Group;

            //! The default shader variant (i.e. the one without any options set).
            static const ShaderVariantStableId RootShaderVariantStableId;
            // @subProductType is one of ShaderAssetSubId, or (ShaderAssetSubId::GeneratedHlslSource + 1)+
            static uint32_t MakeAssetProductSubId(uint32_t rhiApiUniqueIndex, uint32_t subProductType);

            ShaderAsset() = default;
            ~ShaderAsset();

            AZ_DISABLE_COPY_MOVE(ShaderAsset);

            //! Returns the name of the shader.
            const Name& GetName() const;

            //! This function should be your one stop shop to get a ShaderVariantAsset.
            //! Finds and returns the best matching ShaderVariantAsset given a ShaderVariantId.
            //! If the ShaderVariantAsset is not fully loaded and ready at the moment, this function
            //! will QueueLoad the ShaderVariantTreeAsset and subsequently will QueueLoad the ShaderVariantAsset.
            //! The called will be notified via the ShaderVariantFinderNotificationBus when the
            //! ShaderVariantAsset is loaded and ready.
            //! In the mean time, if the required variant is not available this function
            //! returns the Root Variant.
            Data::Asset<ShaderVariantAsset> GetVariant(const ShaderVariantId& shaderVariantId);

            //! Finds the best matching shader variant and returns its StableId.
            //! This function first loads and caches the ShaderVariantTreeAsset (if not done before).
            //! If the ShaderVariantTreeAsset is not found (either the AssetProcessor has not generated it yet, or it simply doesn't exist), then
            //! it returns a search result that identifies the root variant.
            //! This function is thread safe.
            ShaderVariantSearchResult FindVariantStableId(const ShaderVariantId& shaderVariantId);

            //! Returns the variant asset associated with the provided StableId.
            //! The user should call FindVariantStableId() first to get a ShaderVariantStableId from a ShaderVariantId,
            //! Or better yet, call GetVariant(ShaderVariantId) for maximum convenience.
            //! If the requested variant is not found, the root variant will be returned AND the requested variant will be queued for loading.
            //! Next time around if the variant has been loaded this function will return it. Alternatively
            //! the caller can register with the ShaderVariantFinderNotificationBus to get the asset as soon as is available.
            //! This function is thread safe.
            Data::Asset<ShaderVariantAsset> GetVariant(ShaderVariantStableId shaderVariantStableId) const;

            Data::Asset<ShaderVariantAsset> GetRootVariant() const;

            //! Finds and returns the shader resource group asset with the requested name. Returns an empty handle if no matching group was found.
            const Data::Asset<ShaderResourceGroupAsset>& FindShaderResourceGroupAsset(const Name& shaderResourceGroupName) const;

            //! Finds and returns the shader resource group asset associated with the requested binding slot. Returns an empty handle if no matching group was found.
            const Data::Asset<ShaderResourceGroupAsset>& FindShaderResourceGroupAsset(uint32_t bindingSlot) const;

            //! Finds and returns the shader resource group asset designated as a ShaderVariantKey fallback.
            const Data::Asset<ShaderResourceGroupAsset>& FindFallbackShaderResourceGroupAsset() const;

            //! Returns the set of shader resource groups referenced by all variants in the shader asset.
            AZStd::array_view<Data::Asset<ShaderResourceGroupAsset>> GetShaderResourceGroupAssets() const;

            //! Returns the pipeline state type generated by variants of this shader.
            RHI::PipelineStateType GetPipelineStateType() const;

            //! Returns the pipeline layout descriptor shared by all variants in the asset.
            const RHI::PipelineLayoutDescriptor* GetPipelineLayoutDescriptor() const;

            //! Returns the shader option group layout used by all variants in the shader asset.
            const ShaderOptionGroupLayout* GetShaderOptionGroupLayout() const;

            //! Returns the shader resource group asset that has per-draw frequency, which is added to every draw packet.
            const Data::Asset<ShaderResourceGroupAsset>& GetDrawSrgAsset() const;

            //! Returns a list of arguments for the specified attribute, or nullopt_t if the attribute is not found. The list can be empty which is still valid.
            AZStd::optional<RHI::ShaderStageAttributeArguments> GetAttribute(const RHI::ShaderStage& shaderStage, const Name& attributeName) const;

            //! Returns the draw list tag name. 
            //! To get the corresponding DrawListTag use DrawListTagRegistry's FindTag() or AcquireTag() (see RHISystemInterface::GetDrawListTagRegistry()).
            //! The DrawListTag is also available in the Shader that corresponds to this ShaderAsset.
            const Name& GetDrawListName() const;

            //! Return the timestamp when the shader asset was built.
            //! This is used to synchronize versions of the ShaderAsset and ShaderVariantTreeAsset, especially during hot-reload.
            AZStd::sys_time_t GetShaderAssetBuildTimestamp() const;

        private:
            ///////////////////////////////////////////////////////////////////
            /// AssetBus overrides
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            void OnAssetReady(Data::Asset<Data::AssetData> asset) override;
            ///////////////////////////////////////////////////////////////////

            void ReinitializeRootShaderVariant(Data::Asset<Data::AssetData> asset);

            ///////////////////////////////////////////////////////////////////
            /// ShaderVariantFinderNotificationBus overrides
            void OnShaderVariantTreeAssetReady(Data::Asset<ShaderVariantTreeAsset> shaderVariantTreeAsset, bool isError) override;
            void OnShaderVariantAssetReady(Data::Asset<ShaderVariantAsset> /*shaderVariantAsset*/, bool /*isError*/) override {};
            ///////////////////////////////////////////////////////////////////

            //! Container of shader data that is specific to an RHI API.
            //! A ShaderAsset can contain shader data for multiple RHI APIs if
            //! the platform support multiple RHIs.
            struct ShaderApiDataContainer
            {
                AZ_TYPE_INFO(ShaderApiDataContainer, "{1CF7F153-8355-4374-89EF-AD0F78B83D95}");
                static void Reflect(AZ::ReflectContext* context);

                //! RHI API Type for this shader data.
                RHI::APIType m_APIType;

                //! The pipeline layout is shared between all variants in the shader.
                RHI::Ptr<RHI::PipelineLayoutDescriptor> m_pipelineLayoutDescriptor;

                Data::Asset<ShaderVariantAsset> m_rootShaderVariantAsset;

                ///////////////////////////////////////////////////////////////////
                //! List of attributes attached to the shader stage entry
                //! In cases where the virtual shader stage maps to one shader entry function, the attributes are of that entry function
                //! In cases where the virtual shader stage maps to multiple entries, the attributes list is a union of all attributes
                RHI::ShaderStageAttributeMapList m_attributeMaps;
            };

            bool PostLoadInit() override;
            void SetReady();

            //! SelectShaderApiData() must be called before most other ShaderAsset functions.
            bool SelectShaderApiData();

            //! Returns the active ShaderApiDataContainer which was selected in SelectShaderApiData().
            ShaderApiDataContainer& GetCurrentShaderApiData();
            const ShaderApiDataContainer& GetCurrentShaderApiData() const;

            Name m_name;

            //! Dictates the type of pipeline state generated by this asset (Draw / Dispatch / etc.).
            //! All shader variants in the asset adhere to this type.
            RHI::PipelineStateType m_pipelineStateType = RHI::PipelineStateType::Count;

            //! Shader resource group assets referenced by this shader asset.
            AZStd::fixed_vector<Data::Asset<ShaderResourceGroupAsset>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> m_shaderResourceGroupAssets;

            //! Defines the layout of the shader options in the asset.
            Ptr<ShaderOptionGroupLayout> m_shaderOptionGroupLayout;

            //! List with shader data per RHI backend.
            AZStd::vector<ShaderApiDataContainer> m_perAPIShaderData;

            static constexpr size_t InvalidAPITypeIndex = std::numeric_limits<size_t>::max();

            //! Index that indicates which ShaderDataContainer to use.
            size_t m_currentAPITypeIndex = InvalidAPITypeIndex;

            Name m_drawListName;

            //! Used for thread safety for FindVariantStableId().
            mutable AZStd::shared_mutex m_variantTreeMutex;

            //! Use to synchronize versions of the ShaderAsset and ShaderVariantTreeAsset, especially during hot-reload.
            AZStd::sys_time_t m_shaderAssetBuildTimestamp = 0; 

            //! Do Not Serialize! We can not know the ShaderVariantTreeAsset by the time this asset is being created.
            //! This is a value that is discovered at run time. It becomes valid when FindVariantStableId is called at least once.
           Data::Asset<ShaderVariantTreeAsset> m_shaderVariantTree;
           bool m_shaderVariantTreeLoadWasRequested = false;
        };

        class ShaderAssetHandler final
            : public AssetHandler<ShaderAsset>
        {
            using Base = AssetHandler<ShaderAsset>;
        public:
            ShaderAssetHandler() = default;

        private:
            Data::AssetHandler::LoadResult LoadAssetData(
                const Data::Asset<Data::AssetData>& asset,
                AZStd::shared_ptr<Data::AssetDataStream> stream,
                const Data::AssetFilterCB& assetLoadFilterCB) override;
        };

        //////////////////////////////////////////////////////////////////////////
        // Deprecated System
        enum class ShaderAssetSubId : uint32_t
        {
            ShaderAsset = 0,
            RootShaderVariantAsset,
            PostPreprocessingPureAzsl, // .azslin
            IaJson,
            OmJson,
            SrgJson,
            OptionsJson,
            BindingdepJson,
            GeneratedHlslSource // This must be last because we use this as a base for adding the RHI::APIType when generating shadersource for multiple RHI APIs.
        };


        //////////////////////////////////////////////////////////////////////////
    } // namespace RPI

} // namespace AZ
