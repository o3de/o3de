/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Feature/RayTracing/RayTracingFeatureProcessorInterface.h>
#include <Atom/RHI/DispatchRaysIndirectBuffer.h>
#include <Atom/RHI/DispatchRaysItem.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>
#include <AtomCore/std/containers/small_vector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace AZ
{
    namespace Render
    {
        struct RayTracingPassData;

        // One ShaderLib corresponds to one compiled .shader - file with Raytracing shader functions, but one .shader can contain up to five
        // different functions. A HitGroup specifies which ClosestHit, AnyHit and Intersection - shader functions are in the same Hit-Group,
        // but they don't have to be from the same ShaderLib.
        //
        // the RayTracingShaderLibs class holds the ShaderLibs, and the RayTracingHitGroups class holds the Hit-Groups, but the logic to
        // create the Hit-Groups resides in the function RayTracingPass::PrepareHitGroups(), and is somewhat hard-coded for our specific use
        // case.

        // Loaded Raytracing Shader-Libraries
        class RayTracingShaderLibs
        {
        public:
            // a ShaderLib corresponds to one compiled .shader - file that can contain up to five different raytracing shader
            // functions.
            struct ShaderLib
            {
            public:
                ShaderLib(const Data::Instance<RPI::Shader>& shader)
                    : m_shader(shader)
                {
                    // the pipelineStateDescriptor has one 'RayTracing' function, but the referenced shader code can contain multiple entry
                    // functions
                    auto shaderVariant{ shader->GetVariant(RPI::ShaderAsset::RootShaderVariantStableId) };
                    shaderVariant.ConfigurePipelineState(m_pipelineStateDescriptor, shader->GetDefaultShaderOptions());
                }

                Name m_rayGen;
                Name m_closestHit;
                Name m_proceduralClosestHit;
                Name m_anyHit;
                Name m_intersection;
                Name m_miss;

                const Data::Instance<RPI::Shader> m_shader;
                RHI::PipelineStateDescriptorForRayTracing m_pipelineStateDescriptor;
            };

            enum class ShaderFunctionType : uint32_t
            {
                RayGen = 0,
                ClosestHit,
                ProceduralClosestHit, // this is a ClosestHit for RHI
                AnyHit,
                Intersection,
                Miss,
                MAX
            };

            // Load the Shader - asset, create a ShaderLib - entry and set the function name in corresponding slot.
            // If the shader was already loaded only update the function name.
            // e.g.
            //     - "ShaderA.shader", "RayGenerationFunction()" and "ShaderB.shader", "Miss()" will result in two separate ShaderLib -
            //     entries, with one function set each.
            //     - "ShaderA.shader", "RayGenerationFunction()", and "ShaderA.shader", "Miss()" will result in a single ShaderLib - entry
            //     with two functions.
            void AddShaderFunction(
                const ShaderFunctionType type,
                const AZStd::string& entryFunction,
                const Data::Instance<RPI::Shader>& shader,
                const Name& supervariantName = Name(""));

            void AddShaderFunction(
                const ShaderFunctionType type,
                const AZStd::string& entryFunction,
                const RPI::AssetReference& shaderReference,
                const Name& supervariantName = Name(""));

            // list of shader libraries, assigned to slots by the shader function type.
            // e.g. if a shader library contains both a RayGen and a ClosestHit shader,
            // it will be assigned to both slots
            using AssignedShaderLibraries = AZStd::array<AZStd::small_vector<ShaderLib*, 1>, static_cast<size_t>(ShaderFunctionType::MAX)>;
            const AssignedShaderLibraries& GetAssignedShaderLibraries() const;

            // list of unique Shader Libraries, regardless of the shader functions they contain
            using UniqueShaderLibraries = AZStd::unordered_map<Data::AssetId, AZStd::unique_ptr<ShaderLib>>;
            const UniqueShaderLibraries& GetShaderLibraries() const;

            // The ShaderLib containing the RPI::Shader and RHI::PipelineStateDescriptorForRayTracing for the RayGeneration function,
            // which should be used to create the SRGs.
            const ShaderLib* GetRayGenShaderLib() const;
            // Convenience function to return the shader containing the RayGen function
            const RPI::Shader* GetShaderForSrgs() const;

            // Register the shader libraries with the pipeline
            void RegisterShaderLibraries(RHI::RayTracingPipelineStateDescriptor& descriptor);

            void Reset();

        private:
            ShaderLib* GetOrCreateShaderLib(const RPI::AssetReference& assetReference, const Name& supervariantName);
            ShaderLib* GetOrCreateShaderLib(const Data::Instance<RPI::Shader>& shader, const Name& supervariantName);
            void AddShaderFunction(const ShaderFunctionType type, const AZStd::string& entryFunction, ShaderLib* shaderLib);
            UniqueShaderLibraries m_shaderLibs;
            AssignedShaderLibraries m_assignedShaderLibs;
        };

        // helper struct to avoid passing three arguments of the same type to RayTracingHitGroups.AddHitGroup()
        struct HitGroupShaderLibs
        {
            RayTracingShaderLibs::ShaderLib* m_closestHit = nullptr;
            RayTracingShaderLibs::ShaderLib* m_anyHit = nullptr;
            RayTracingShaderLibs::ShaderLib* m_intersection = nullptr;
        };

        // Names of the Shaders used for RayGeneration, Miss and ClosestHit/AnyHit/Intersection for the hit-groups.
        class RayTracingHitGroups
        {
        public:
            struct HitGroup
            {
                Name m_name;
                Name m_closestHit;
                Name m_anyHit;
                Name m_intersection;
            };

            void Reset();

            AZStd::shared_ptr<RHI::RayTracingShaderTableDescriptor> CreateRayTracingShaderTableDescriptor(
                RHI::Ptr<RHI::RayTracingPipelineState>& rayTracingPipelineState) const;
            const AZStd::vector<HitGroup>& GetHitGroups() const;

            void RegisterHitGroups(RHI::RayTracingPipelineStateDescriptor& descriptor);

            void SetRayGenerationShader(RayTracingShaderLibs::ShaderLib* shaderLib);
            void SetMissShader(RayTracingShaderLibs::ShaderLib* shaderLib);
            void AddHitGroup(const Name& name, const HitGroupShaderLibs& shaderLibs, bool IsProceduralHitGroup = false);

        private:
            Name m_rayGenShader;
            Name m_missShader;
            AZStd::vector<HitGroup> m_hitGroups;
        };

        //! This pass executes a raytracing shader as specified in the PassData.
        class RayTracingPass
            : public RPI::RenderPass
            , private RPI::ShaderReloadNotificationBus::MultiHandler
        {
            AZ_RPI_PASS(RayTracingPass);

        public:
            AZ_RTTI(RayTracingPass, "{7A68A36E-956A-4258-93FE-38686042C4D9}", RPI::RenderPass);
            AZ_CLASS_ALLOCATOR(RayTracingPass, SystemAllocator);
            virtual ~RayTracingPass();

            //! Creates a RayTracingPass
            static RPI::Ptr<RayTracingPass> Create(const RPI::PassDescriptor& descriptor);

            void SetMaxRayLength(float maxRayLength) { m_maxRayLength = maxRayLength; }

        protected:
            RayTracingPass(const RPI::PassDescriptor& descriptor);

            // load the shaders speficied in the passData and fill the m_shaders shader-lib
            void LoadShaderLibs(const RayTracingPassData* passData);
            // Load the intersection shaders for the procedural meshes from the RaytracingFeatureProcessor
            void LoadProceduralShaderLibs(RayTracingFeatureProcessorInterface* rtfp);
            void PrepareSrgs();
            void PrepareHitGroups();

            // check if the shader names are unique and the srg layouts match
            bool ValidateShaderLibs(const AZStd::span<const RayTracingShaderLibs* const> shaderLibs) const;

            // Pass overrides
            bool IsEnabled() const override;
            void BuildInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;
            void FrameEndInternal() override;

            // Scope producer functions
            void SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph) override;
            void CompileResources(const RHI::FrameGraphCompileContext& context) override;
            void BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context) override;

            // ShaderReloadNotificationBus::Handler overrides
            void OnShaderReinitialized(const RPI::Shader& shader) override;
            void OnShaderAssetReinitialized(const Data::Asset<RPI::ShaderAsset>& shaderAsset) override;
            void OnShaderVariantReinitialized(const RPI::ShaderVariant& shaderVariant) override;

            // load the raytracing shaders and setup pipeline states
            void CreatePipelineState();

            // pass data
            RPI::PassDescriptor m_passDescriptor;
            const RayTracingPassData* m_passData = nullptr;

            Name m_fullscreenSizeSourceSlotName;
            bool m_fullscreenDispatch = false;
            RPI::PassAttachmentBinding* m_fullscreenSizeSourceBinding = nullptr;

            bool m_indirectDispatch = false;
            Name m_indirectDispatchBufferSlotName;
            RPI::PassAttachmentBinding* m_indirectDispatchRaysBufferBinding = nullptr;
            RHI::Ptr<RHI::IndirectBufferSignature> m_indirectDispatchRaysBufferSignature;
            RHI::IndirectBufferView m_indirectDispatchRaysBufferView;
            RHI::Ptr<RHI::DispatchRaysIndirectBuffer> m_dispatchRaysIndirectBuffer;

            // revision number of the ray tracing TLAS when the shader table was built
            uint32_t m_rayTracingShaderTableRevision{ std::numeric_limits<uint32_t>::max() };
            uint32_t m_dispatchRaysShaderTableRevision{ std::numeric_limits<uint32_t>::max() };
            uint32_t m_proceduralGeometryTypeRevision{ std::numeric_limits<uint32_t>::max() };

            // raytracing shaders: Separated into meshShaders and procedural shaders to allow partial reloading
            RayTracingShaderLibs m_meshShaders;
            RayTracingShaderLibs m_proceduralShaders;
            // RayGen, Miss and HitGroups
            RayTracingHitGroups m_hitGroups;

            RHI::Ptr<RHI::RayTracingPipelineState> m_rayTracingPipelineState;
            RHI::ConstPtr<RHI::PipelineState> m_globalPipelineState;
            RHI::Ptr<RHI::RayTracingShaderTable> m_rayTracingShaderTable;

            // [GFX TODO][ATOM-15610] Add RenderPass::SetSrgsForRayTracingDispatch
            // Remove this as soon as we can use the RenderPass::BindSrg() for raytracing
            AZStd::vector<RHI::ShaderResourceGroup*> m_rayTracingSRGsToBind;

            bool m_requiresViewSrg = false;
            bool m_requiresSceneSrg = false;
            bool m_requiresRayTracingMaterialSrg = false;
            bool m_requiresRayTracingSceneSrg = false;
            float m_maxRayLength = 1e27f;

            RHI::ShaderInputNameIndex m_maxRayLengthInputIndex = "m_maxRayLength";

            RHI::DispatchRaysItem m_dispatchRaysItem;
        };
    }   // namespace RPI
}   // namespace AZ
