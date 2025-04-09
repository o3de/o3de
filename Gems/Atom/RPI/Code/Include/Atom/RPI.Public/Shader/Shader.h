/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Shader/ShaderVariant.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>
#include <Atom/RPI.Reflect/Shader/IShaderVariantFinder.h>

#include <Atom/RHI/DrawListTagRegistry.h>
#include <Atom/RHI/PipelineLibrary.h>
#include <Atom/RHI/PipelineState.h>

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
        class ShaderResourceGroup;

        //! Shader is effectively an 'uber-shader' containing a collection of 'variants'. Variants are
        //! designed to be 'variations' on the same core shader technique. To enforce this, every variant
        //! in the shader shares the same pipeline layout (i.e. set of shader resource groups).
        //! 
        //! A shader owns a library of pipeline states. When a variant is resolved to a pipeline state, its
        //! lifetime is determined by the lifetime of the Shader (unless an explicit reference is taken). If
        //! an asset reload event occurs, the pipeline state cache is reset.
        //! 
        //! To use Shader:
        //!  1) Construct a ShaderOptionGroup instance using CreateShaderOptionGroup.
        //!  2) Configure the group by setting values on shader options.
        //!  3) Find the ShaderVariantStableId using the ShaderVariantId generated from the configured ShaderOptionGroup.
        //!  4) Acquire the ShaderVariant instance using the ShaderVariantStableId.
        //!  5) Configure a pipeline state descriptor on the variant; make local overrides as necessary (e.g. to configure runtime render state).
        //!  6) Acquire a RHI::PipelineState instance from the shader using the configured pipeline state descriptor.
        //! 
        //! Remember that the returned RHI::PipelineState instance lifetime is tied to the Shader lifetime.
        //! If you need guarantee lifetime, it is safe to take a reference on the returned pipeline state.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API Shader final
            : public Data::InstanceData
            , public Data::AssetBus::MultiHandler
            , public ShaderVariantFinderNotificationBus::Handler
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class ShaderSystem;
        public:
            AZ_INSTANCE_DATA(Shader, "{232D8BD6-3BD4-4842-ABD2-F380BD5B0863}");
            AZ_CLASS_ALLOCATOR(Shader, SystemAllocator);

            //! Returns the shader instance associated with the provided asset.
            static Data::Instance<Shader> FindOrCreate(const Data::Asset<ShaderAsset>& shaderAsset, const Name& supervariantName);

            //! Same as above, but uses the default supervariant 
            static Data::Instance<Shader> FindOrCreate(const Data::Asset<ShaderAsset>& shaderAsset);

            ~Shader();
            AZ_DISABLE_COPY_MOVE(Shader);

            //! returns the SupervariantIndex that corresponds to the given supervariant name given at instantiation.
            SupervariantIndex GetSupervariantIndex() const { return m_supervariantIndex; }

            //! Constructs a shader option group suitable to generate a shader variant key for this shader.
            ShaderOptionGroup CreateShaderOptionGroup() const;

            //! Finds the best matching ShaderVariant for the given shaderVariantId,
            //! If the variant is loaded and ready it will return the corresponding ShaderVariant.
            //! If the variant is not yet available it will return the root ShaderVariant.
            //! Callers should listen to ShaderReloadNotificationBus to get notified whenever the exact
            //! variant is loaded and available or if a variant changes, etc.
            //! This function should be your one stop shop to get a ShaderVariant from a ShaderVariantId.
            //! Alternatively: You can call FindVariantStableId() followed by GetVariant(shaderVariantStableId).
            const ShaderVariant& GetVariant(const ShaderVariantId& shaderVariantId);

            //! Finds the best matching shader variant asset and returns its StableId.
            //! In cases where you can't cache the ShaderVariant, and recurrently you may need
            //! the same ShaderVariant at different times, then it can be convenient (and more performant) to call
            //! this method to cache the ShaderVariantStableId and call GetVariant(ShaderVariantStableId)
            //! when needed.
            //! If the asset is not immediately found in the file system, it will return the StableId
            //! of the root variant.
            //! Callers should listen to ShaderReloadNotificationBus to get notified whenever the exact
            //! variant is loaded and available or if a variant changes, etc.
            ShaderVariantSearchResult FindVariantStableId(const ShaderVariantId& shaderVariantId) const;

            //! Returns the variant associated with the provided StableId.
            //! You should call FindVariantStableId() which caches the variant, later
            //! when this function is called the variant is fetched from a local map.
            //! If the variant is not found, the root variant is returned.
            //! "Alternatively: a more convenient approach is to call GetVariant(ShaderVariantId) which does both, the find and the get."
            const ShaderVariant& GetVariant(ShaderVariantStableId shaderVariantStableId);

            //! Returns the root variant.
            const ShaderVariant& GetRootVariant();

            //! Returns the closest variant that uses the default shader option values.
            //! This could return the root variant or a fallback variant if there is no variant baked for that combination of option values.
            const ShaderVariant& GetDefaultVariant();

            //! Returns the default shader option values.
            ShaderOptionGroup GetDefaultShaderOptions() const;

            //! Returns the pipeline state type generated by variants of this shader.
            RHI::PipelineStateType GetPipelineStateType() const;

            //! Returns the ShaderInputContract which describes which inputs the shader requires
            const ShaderInputContract& GetInputContract() const;

            //! Returns the ShaderOutputContract which describes which outputs the shader requires
            const ShaderOutputContract& GetOutputContract() const;
            
            //! Acquires a pipeline state directly from a descriptor.
            const RHI::PipelineState* AcquirePipelineState(const RHI::PipelineStateDescriptor& descriptor) const;

            //! Finds and returns the shader resource group asset with the requested name. Returns an empty handle if no matching group was found.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& FindShaderResourceGroupLayout(const Name& shaderResourceGroupName) const;

            //! Finds and returns the shader resource group asset associated with the requested binding slot. Returns an empty handle if no matching group was found.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& FindShaderResourceGroupLayout(uint32_t bindingSlot) const;

            //! Finds and returns the shader resource group asset designated as a ShaderVariantKey fallback.
            const RHI::Ptr<RHI::ShaderResourceGroupLayout>& FindFallbackShaderResourceGroupLayout() const;

            //! Returns the set of shader resource groups referenced by all variants in the shader asset.
            AZStd::span<const RHI::Ptr<RHI::ShaderResourceGroupLayout>> GetShaderResourceGroupLayouts() const;

            //! Creates a DrawSrg that contains the shader variant fallback key.
            //! This SRG must be included in the DrawPacket for any shader that has shader options,
            //! otherwise the CommandList will fail validation for SRG being null.
            //! @param shaderOptions The shader option values will be stored in the SRG's shader variant fallback key (if there is one).
            //! @param compileTheSrg If you need to set other values in the SRG, set this to false, and the call Compile() when you are done.
            //! @return The DrawSrg instance, or null if the shader does not include a DrawSrg.
            Data::Instance<ShaderResourceGroup> CreateDrawSrgForShaderVariant(const ShaderOptionGroup& shaderOptions, bool compileTheSrg);

            //! Creates a DrawSrg that contains the shader variant fallback key, initialized to the default shader options values.
            //! This SRG must be included in the DrawPacket for any shader that has shader options,
            //! otherwise the CommandList will fail validation for SRG being null.
            //! @param compileTheSrg If you need to set other values in the SRG, set this to false, and the call Compile() when you are done.
            //! @return The DrawSrg instance, or null if the shader does not include a DrawSrg.
            Data::Instance<ShaderResourceGroup> CreateDefaultDrawSrg(bool compileTheSrg);

            //! Returns a reference to the asset used to initialize this shader.
            const Data::Asset<ShaderAsset>& GetAsset() const;

            //! Returns the DrawListTag that identifies which Pass and View objects will process this shader.
            //! This tag corresponds to the ShaderAsset object's DrawListName.
            RHI::DrawListTag GetDrawListTag() const;

        private:
            explicit Shader(const SupervariantIndex& supervariantIndex) : m_supervariantIndex(supervariantIndex){};
            Shader() = delete;

            static Data::Instance<Shader> CreateInternal(ShaderAsset& shaderAsset, const AZStd::any* supervariantName);

            RHI::ResultCode Init(ShaderAsset& shaderAsset);

            void Shutdown();

            AZStd::unordered_map<int, ConstPtr<RHI::PipelineLibraryData>> LoadPipelineLibrary() const;
            void SavePipelineLibrary() const;
            
            const ShaderVariant& GetVariantInternal(ShaderVariantStableId shaderVariantStableId);

            // AssetBus overrides...
            void OnAssetReloaded(Data::Asset<Data::AssetData> asset) override;

            // ShaderVariantFinderNotificationBus overrides...
            void OnShaderVariantTreeAssetReady(Data::Asset<ShaderVariantTreeAsset> /*shaderVariantTreeAsset*/, bool /*isError*/) override {};
            void OnShaderVariantAssetReady(Data::Asset<ShaderVariantAsset> shaderVariantAsset, bool IsError) override;

            //! A strong reference to the shader asset.
            Data::Asset<ShaderAsset> m_asset;

            /////////////////////////////////////////////////////////////////////////////////////
            //! The following variables are necessary to reliably reload the Shader
            //! whenever the Shader source assets and dependencies change.
            //! 
            //! Each time the Shader is initialized, this variable
            //! caches all the Assets that we are expecting to be reloaded whenever
            //! the Shader asset changes. This includes m_asset + each Supervariant ShaderVariantAsset.
            //! Typically most shaders only contain one Supervariant, so this variable becomes 2. 
            size_t m_expectedAssetReloadCount = 0;
            //! Each time one of the assets is reloaded we store it here, and when the
            //! size of this dictionary equals @m_expectedAssetReloadCount then we know it is safe
            //! to reload the Shader.
            AZStd::unordered_map<Data::AssetId, Data::Asset<Data::AssetData>> m_reloadedAssets;
            /////////////////////////////////////////////////////////////////////////////////////

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
            AZStd::unordered_map<int, AZStd::string> m_pipelineLibraryPaths;
        };
    }
}
