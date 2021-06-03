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

#include <Atom/RPI.Public/Shader/ShaderVariant2.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset2.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Reflect/Shader/IShaderVariantFinder2.h>

#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/PipelineLibrary.h>

#include <AtomCore/Instance/InstanceData.h>

#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace RHI
    {
        class PipelineStateCache;
    }

    namespace RPI
    {
        /**
         * Shader2 is effectively an 'uber-shader' containing a collection of 'variants'. Variants are
         * designed to be 'variations' on the same core shader technique. To enforce this, every variant
         * in the shader shares the same pipeline layout (i.e. set of shader resource groups).
         *
         * A shader owns a library of pipeline states. When a variant is resolved to a pipeline state, its
         * lifetime is determined by the lifetime of the Shader2 (unless an explicit reference is taken). If
         * an asset reload event occurs, the pipeline state cache is reset.
         *
         * To use Shader2:
         *  1) Construct a ShaderOptionGroup instance using CreateShaderOptionGroup.
         *  2) Configure the group by setting values on shader options.
         *  3) Find the ShaderVariantStableId using the ShaderVariantId generated from the configured ShaderOptionGroup.
         *  4) Acquire the ShaderVariant2 instance using the ShaderVariantStableId.
         *  5) Configure a pipeline state descriptor on the variant; make local overrides as necessary (e.g. to configure runtime render state).
         *  6) Acquire a RHI::PipelineState instance from the shader using the configured pipeline state descriptor.
         *
         * Remember that the returned RHI::PipelineState instance lifetime is tied to the Shader2 lifetime.
         * If you need guarantee lifetime, it is safe to take a reference on the returned pipeline state.
         */
        class Shader2 final
            : public Data::InstanceData
            , public Data::AssetBus::Handler
            , public ShaderVariantFinderNotificationBus2::Handler
        {
            friend class ShaderSystem;
        public:
            AZ_INSTANCE_DATA(Shader2, "{232D8BD6-3BD4-4842-ABD2-F380BD5B0863}");
            AZ_CLASS_ALLOCATOR(Shader2, SystemAllocator, 0);

            /// Returns the shader instance associated with the provided asset.
            static Data::Instance<Shader2> FindOrCreate(const Data::Asset<ShaderAsset2>& shaderAsset, const Name& supervariantName);

            ~Shader2();
            AZ_DISABLE_COPY_MOVE(Shader2);

            /// Constructs a shader option group suitable to generate a shader variant key for this shader.
            ShaderOptionGroup CreateShaderOptionGroup() const;

            /// Finds the best matching ShaderVariant2 for the given shaderVariantId,
            /// If the variant is loaded and ready it will return the corresponding ShaderVariant2.
            /// If the variant is not yet available it will return the root ShaderVariant2.
            /// Callers should listen to ShaderReloadNotificationBus to get notified whenever the exact
            /// variant is loaded and available or if a variant changes, etc.
            /// This function should be your one stop shop to get a ShaderVariant2 from a ShaderVariantId.
            /// Alternatively: You can call FindVariantStableId() followed by GetVariant(shaderVariantStableId).
            const ShaderVariant2& GetVariant(const ShaderVariantId& shaderVariantId);

            /// Finds the best matching shader variant asset and returns its StableId.
            /// In cases where you can't cache the ShaderVariant2, and recurrently you may need
            /// the same ShaderVariant2 at different times, then it can be convenient (and more performant) to call
            /// this method to cache the ShaderVariantStableId and call GetVariant(ShaderVariantStableId)
            /// when needed.
            /// If the asset is not immediately found in the file system, it will return the StableId
            /// of the root variant.
            /// Callers should listen to ShaderReloadNotificationBus to get notified whenever the exact
            /// variant is loaded and available or if a variant changes, etc.
            ShaderVariantSearchResult FindVariantStableId(const ShaderVariantId& shaderVariantId) const;

            /// Returns the variant associated with the provided StableId.
            /// You should call FindVariantStableId() which caches the variant, later
            /// when this function is called the variant is fetched from a local map.
            /// If the variant is not found, the root variant is returned.
            /// "Alternatively: a more convenient approach is to call GetVariant(ShaderVariantId) which does both, the find and the get."
            const ShaderVariant2& GetVariant(ShaderVariantStableId shaderVariantStableId);

            /// Convenient function that returns the root variant.
            const ShaderVariant2& GetRootVariant();

            /// Returns the pipeline state type generated by variants of this shader.
            RHI::PipelineStateType GetPipelineStateType() const;

            //! Returns the ShaderInputContract which describes which inputs the shader requires
            const ShaderInputContract& GetInputContract() const;

            //! Returns the ShaderOutputContract which describes which outputs the shader requires
            const ShaderOutputContract& GetOutputContract() const;
            
            /// Acquires a pipeline state directly from a descriptor.
            const RHI::PipelineState* AcquirePipelineState(const RHI::PipelineStateDescriptor& descriptor) const;

            /// Finds and returns the shader resource group asset with the requested name. Returns an empty handle if no matching group was found.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout> FindShaderResourceGroupLayout(const Name& shaderResourceGroupName) const;

            /// Finds and returns the shader resource group asset associated with the requested binding slot. Returns an empty handle if no matching group was found.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout> FindShaderResourceGroupLayout(uint32_t bindingSlot) const;

            /// Finds and returns the shader resource group asset designated as a ShaderVariantKey fallback.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout> FindFallbackShaderResourceGroupLayout() const;

            /// Returns the set of shader resource groups referenced by all variants in the shader asset.
            AZStd::array_view<RHI::Ptr<RHI::ShaderResourceGroupLayout>> GetShaderResourceGroupLayouts() const;

            /// Returns a reference to the asset used to initialize this shader.
            const Data::Asset<ShaderAsset2>& GetAsset() const;

            //! Returns the DrawListTag that identifies which Pass and View objects will process this shader.
            //! This tag corresponds to the ShaderAsset2 object's DrawListName.
            RHI::DrawListTag GetDrawListTag() const;

        private:
            Shader2() = default;

            static Data::Instance<Shader2> CreateInternal(ShaderAsset2& shaderAsset);

            bool SelectSupervariant(const Name& supervariantName);

            RHI::ResultCode Init(ShaderAsset2& shaderAsset);

            void Shutdown();

            ConstPtr<RHI::PipelineLibraryData> LoadPipelineLibrary() const;
            void SavePipelineLibrary() const;

            ///////////////////////////////////////////////////////////////////
            /// AssetBus overrides
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;
            ///////////////////////////////////////////////////////////////////

            ///////////////////////////////////////////////////////////////////
            /// ShaderVariantFinderNotificationBus overrides
            void OnShaderVariantTreeAssetReady(Data::Asset<ShaderVariantTreeAsset> /*shaderVariantTreeAsset*/, bool /*isError*/) override {};
            void OnShaderVariantAssetReady(Data::Asset<ShaderVariantAsset2> shaderVariantAsset, bool IsError) override;
            ///////////////////////////////////////////////////////////////////

            //! Returns the path to the pipeline library cache file.
            AZStd::string GetPipelineLibraryPath() const;

            //! A strong reference to the shader asset.
            Data::Asset<ShaderAsset2> m_asset;

            //! Selects current supervariant to be used.
            //! This value is defined at instantiation.
            SupervariantIndex m_supervariantIndex;

            //! The pipeline state type required by this shader.
            RHI::PipelineStateType m_pipelineStateType = RHI::PipelineStateType::Draw;

            //! A cached pointer to the pipeline state cache owned by RHISystem.
            RHI::PipelineStateCache* m_pipelineStateCache = nullptr;

            //! A handle to the pipeline library in the pipeline state cache.
            RHI::PipelineLibraryHandle m_pipelineLibraryHandle;

            //! Used for thread safety for FindVariantStableId() and GetVariant().
            AZStd::shared_mutex m_variantCacheMutex;

            //! The root variant always exist.
            ShaderVariant2 m_rootVariant;

            //! Local cache of ShaderVariants (except for the root variant), searchable by StableId.
            //! Gets populated when GetVariant() is called.
            AZStd::unordered_map<ShaderVariantStableId, ShaderVariant2> m_shaderVariants;
            
            //! DrawListTag associated with this shader.
            RHI::DrawListTag m_drawListTag;
        };
    }
}
