/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Shader/ShaderVariant.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Reflect/Shader/IShaderVariantFinder.h>

#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/PipelineLibrary.h>

#include <AtomCore/Instance/InstanceData.h>
#include <AzCore/IO/SystemFile.h>
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
         * Shader is effectively an 'uber-shader' containing a collection of 'variants'. Variants are
         * designed to be 'variations' on the same core shader technique. To enforce this, every variant
         * in the shader shares the same pipeline layout (i.e. set of shader resource groups).
         *
         * A shader owns a library of pipeline states. When a variant is resolved to a pipeline state, its
         * lifetime is determined by the lifetime of the Shader (unless an explicit reference is taken). If
         * an asset reload event occurs, the pipeline state cache is reset.
         *
         * To use Shader:
         *  1) Construct a ShaderOptionGroup instance using CreateShaderOptionGroup.
         *  2) Configure the group by setting values on shader options.
         *  3) Find the ShaderVariantStableId using the ShaderVariantId generated from the configured ShaderOptionGroup.
         *  4) Acquire the ShaderVariant instance using the ShaderVariantStableId.
         *  5) Configure a pipeline state descriptor on the variant; make local overrides as necessary (e.g. to configure runtime render state).
         *  6) Acquire a RHI::PipelineState instance from the shader using the configured pipeline state descriptor.
         *
         * Remember that the returned RHI::PipelineState instance lifetime is tied to the Shader lifetime.
         * If you need guarantee lifetime, it is safe to take a reference on the returned pipeline state.
         */
        class Shader final
            : public Data::InstanceData
            , public Data::AssetBus::Handler
            , public ShaderVariantFinderNotificationBus::Handler
            , public ShaderReloadNotificationBus::Handler
        {
            friend class ShaderSystem;
        public:
            AZ_INSTANCE_DATA(Shader, "{232D8BD6-3BD4-4842-ABD2-F380BD5B0863}");
            AZ_CLASS_ALLOCATOR(Shader, SystemAllocator, 0);

            /// Returns the shader instance associated with the provided asset.
            static Data::Instance<Shader> FindOrCreate(const Data::Asset<ShaderAsset>& shaderAsset, const Name& supervariantName);

            /// Same as above, but uses the default supervariant 
            static Data::Instance<Shader> FindOrCreate(const Data::Asset<ShaderAsset>& shaderAsset);

            ~Shader();
            AZ_DISABLE_COPY_MOVE(Shader);

            /// returns the SupervariantIndex that corresponds to the given supervariant name given at instantiation.
            SupervariantIndex GetSupervariantIndex() { return m_supervariantIndex; }

            /// Constructs a shader option group suitable to generate a shader variant key for this shader.
            ShaderOptionGroup CreateShaderOptionGroup() const;

            /// Finds the best matching ShaderVariant for the given shaderVariantId,
            /// If the variant is loaded and ready it will return the corresponding ShaderVariant.
            /// If the variant is not yet available it will return the root ShaderVariant.
            /// Callers should listen to ShaderReloadNotificationBus to get notified whenever the exact
            /// variant is loaded and available or if a variant changes, etc.
            /// This function should be your one stop shop to get a ShaderVariant from a ShaderVariantId.
            /// Alternatively: You can call FindVariantStableId() followed by GetVariant(shaderVariantStableId).
            const ShaderVariant& GetVariant(const ShaderVariantId& shaderVariantId);

            /// Finds the best matching shader variant asset and returns its StableId.
            /// In cases where you can't cache the ShaderVariant, and recurrently you may need
            /// the same ShaderVariant at different times, then it can be convenient (and more performant) to call
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
            const ShaderVariant& GetVariant(ShaderVariantStableId shaderVariantStableId);

            /// Convenient function that returns the root variant.
            const ShaderVariant& GetRootVariant();

            /// Returns the pipeline state type generated by variants of this shader.
            RHI::PipelineStateType GetPipelineStateType() const;

            //! Returns the ShaderInputContract which describes which inputs the shader requires
            const ShaderInputContract& GetInputContract() const;

            //! Returns the ShaderOutputContract which describes which outputs the shader requires
            const ShaderOutputContract& GetOutputContract() const;
            
            /// Acquires a pipeline state directly from a descriptor.
            const RHI::PipelineState* AcquirePipelineState(const RHI::PipelineStateDescriptor& descriptor) const;

            /// Finds and returns the shader resource group asset with the requested name. Returns an empty handle if no matching group was found.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& FindShaderResourceGroupLayout(const Name& shaderResourceGroupName) const;

            /// Finds and returns the shader resource group asset associated with the requested binding slot. Returns an empty handle if no matching group was found.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& FindShaderResourceGroupLayout(uint32_t bindingSlot) const;

            /// Finds and returns the shader resource group asset designated as a ShaderVariantKey fallback.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& FindFallbackShaderResourceGroupLayout() const;

            /// Returns the set of shader resource groups referenced by all variants in the shader asset.
            AZStd::array_view<RHI::Ptr<RHI::ShaderResourceGroupLayout>> GetShaderResourceGroupLayouts() const;

            /// Returns a reference to the asset used to initialize this shader.
            const Data::Asset<ShaderAsset>& GetAsset() const;

            //! Returns the DrawListTag that identifies which Pass and View objects will process this shader.
            //! This tag corresponds to the ShaderAsset object's DrawListName.
            RHI::DrawListTag GetDrawListTag() const;

            //! Changes the supervariant of the shader to the specified supervariantIndex.
            //! [GFX TODO][ATOM-15813]: this can be removed when the shader InstanceDatabase can support multiple shader
            //! instances with different supervariants.
            void ChangeSupervariant(SupervariantIndex supervariantIndex);

        private:
            explicit Shader(const SupervariantIndex& supervariantIndex) : m_supervariantIndex(supervariantIndex){};
            Shader() = delete;

            static Data::Instance<Shader> CreateInternal(ShaderAsset& shaderAsset, const AZStd::any* supervariantName);

            RHI::ResultCode Init(ShaderAsset& shaderAsset);

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
            void OnShaderVariantAssetReady(Data::Asset<ShaderVariantAsset> shaderVariantAsset, bool IsError) override;
            ///////////////////////////////////////////////////////////////////
            
            ///////////////////////////////////////////////////////////////////
            // ShaderReloadNotificationBus overrides...
            void OnShaderAssetReinitialized(const Data::Asset<ShaderAsset>& shaderAsset) override;
            // Note we don't need OnShaderVariantReinitialized because the Shader class doesn't do anything with the data inside
            // the ShaderVariant object. The only thing we might want to do is propagate the message upward, but that's unnecessary
            // because the ShaderReloadNotificationBus uses the Shader's AssetId as the ID for all messages including those from the variants.
            // And of course we don't need to handle OnShaderReinitialized because this *is* this Shader.
            ///////////////////////////////////////////////////////////////////

            //! A strong reference to the shader asset.
            Data::Asset<ShaderAsset> m_asset;

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
            ShaderVariant m_rootVariant;

            //! Local cache of ShaderVariants (except for the root variant), searchable by StableId.
            //! Gets populated when GetVariant() is called.
            AZStd::unordered_map<ShaderVariantStableId, ShaderVariant> m_shaderVariants;
            
            //! DrawListTag associated with this shader.
            RHI::DrawListTag m_drawListTag;

            //! PipelineLibrary file name
            char m_pipelineLibraryPath[AZ_MAX_PATH_LEN] = { 0 };
        };
    }
}
