/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Shader/ShaderReloadNotificationBus.h>
#include <Atom/RPI.Reflect/Base.h>

namespace AZ::Render
{

    class RayTracingMaterialShaderManager;
    class RayTracingFeatureProcessor;

    // Holder for a Raytracing Shader Library that handles shader reload notifications.
    // A shader library is effectively the bytecode for a single shader entry function.
    // Note that a library holds the bytecode from an .azshader file, which can can hold arbitrary many entry functions,
    // but we use only one of the entry functions, and allow the same .azshader file to be loaded in many libraries
    class RayTracingMaterialShaderLibrary
        : public AZStd::intrusive_base
        , private RPI::ShaderReloadNotificationBus::Handler
    {
    public:
        RayTracingMaterialShaderLibrary() = default;
        ~RayTracingMaterialShaderLibrary();

        RHI::DrawListTag GetDrawListTag() const
        {
            return m_drawListTag;
        }

        const bool IsStale() const
        {
            return m_isStale;
        }

        const int32_t GetShaderIndex() const
        {
            return m_shaderIndex;
        }

        const Data::Instance<RPI::Shader>& GetShader() const
        {
            return m_shader;
        }

        const RHI::PipelineStateDescriptorForRayTracing& GetPipelineStateDescriptor() const
        {
            return m_descriptor;
        }
        const AZ::Name& GetEntryFunctionName() const
        {
            return m_entryFunction;
        }
        void OnShaderReload();

        void ConnectMaterialShaderManager(RayTracingMaterialShaderManager* shaderManager)
        {
            m_shaderManager = shaderManager;
        }

        void OnShaderReinitialized(const RPI::Shader& shader) override;
        void OnShaderAssetReinitialized(const Data::Asset<RPI::ShaderAsset>& shaderAsset) override;
        void OnShaderVariantReinitialized(const RPI::ShaderVariant& shaderVariant) override;

        void Init(
            const RPI::Material* material,
            const RPI::ShaderCollection::Item& shaderItem,
            const AZ::Name& entryFunction,
            const int shaderIndex);

    private:
        RayTracingMaterialShaderManager* m_shaderManager;

        bool m_isStale = false;
        RHI::DrawListTag m_drawListTag;
        Data::Instance<RPI::Shader> m_shader;
        RHI::PipelineStateDescriptorForRayTracing m_descriptor;
        // The name of the entry function as compiled in the shader
        AZ::Name m_entryFunction;
        RPI::ShaderVariantId m_shaderVariantId;
        int32_t m_shaderIndex;
    };

    // Collection of all Raytracing Shader Libraries for one drawListTag: generally one ray generation shader, several closest hit shaders,
    // and one miss shader. The functions can be distributed across multiple files.
    class MaterialShaderLibraries : public AZStd::intrusive_base
    {
    public:
        using HitShaderId = size_t;

        static HitShaderId CalculateMaterialHitShaderId(const RPI::Material* material);
        Data::Instance<RayTracingMaterialShaderLibrary> GetOrCreateHitShader(
            const RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem);
        Data::Instance<RayTracingMaterialShaderLibrary> GetOrCreateRayGenShader(
            const RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem);
        Data::Instance<RayTracingMaterialShaderLibrary> GetOrCreateMissShader(
            const RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem);

        void Finalize(RayTracingFeatureProcessor* rtfp);

        const bool IsStale() const;

        void ConnectMaterialShaderManager(RayTracingMaterialShaderManager* shaderManager);

        void CreateGlobalPipelineState();
        void CreateRayTracingShaderTable(RayTracingFeatureProcessor* rtfp);
        // TODO: these parameters should come from the .shader file, and not from the passdata
        void CreateRayTracingPipelineState(
            const uint32_t maxPayloadSize, const uint32_t maxAttributeSize, const uint32_t maxRecursionDepth);

        const RHI::PipelineState* GetGlobalPipelineState() const
        {
            return m_globalPipelineState.get();
        }
        RHI::RayTracingShaderTable* GetRayTracingShaderTable() const
        {
            return m_rayTracingShaderTable.get();
        }
        RHI::RayTracingPipelineState* GetRayTracingPipelineState() const
        {
            return m_rayTracingPipelineState.get();
        }

        int HitShaderCount() const
        {
            return static_cast<int>(m_hitShaders.size());
        }

        Data::Instance<RPI::ShaderResourceGroup> CreatePassSrg() const;
        bool RequiresSrg(const uint32_t bindingSlot) const;
        bool RequiresSrg(const Name& bindingSlot) const;

    private:
        bool m_isFinalized = false;
        AZStd::unordered_map<HitShaderId, Data::Instance<RayTracingMaterialShaderLibrary>> m_hitShaders;
        Data::Instance<RayTracingMaterialShaderLibrary> m_rayGenShader;
        Data::Instance<RayTracingMaterialShaderLibrary> m_missShader;

        // revision number of the ray tracing TLAS when the shader table was built
        uint32_t m_rayTracingShaderTableRevision{ std::numeric_limits<uint32_t>::max() };
        uint32_t m_dispatchRaysShaderTableRevision{ std::numeric_limits<uint32_t>::max() };

        // raytracing shaders, pipeline states, and shader table
        RHI::Ptr<RHI::RayTracingPipelineState> m_rayTracingPipelineState;
        RHI::ConstPtr<RHI::PipelineState> m_globalPipelineState;
        RHI::Ptr<RHI::RayTracingShaderTable> m_rayTracingShaderTable;
    };

    // The RayTracingMaterialShaderManager collects and manages the shader-libraries for the different materials, with one
    // MaterialShaderLibraries entry per DrawListTag
    class RayTracingMaterialShaderManager
    {
    public:
        const Data::Instance<MaterialShaderLibraries>& GetShaderLibraries(const RHI::DrawListTag drawListTag) const
        {
            return m_shaderLibraries.at(drawListTag);
        }

        const bool HasShaderLibraries(const RHI::DrawListTag drawListTag) const
        {
            return m_shaderLibraries.contains(drawListTag);
        }

        const int HitShaderCount() const
        {
            for (auto& [_, shaderLibraries] : m_shaderLibraries)
            {
                return shaderLibraries->HitShaderCount();
            }
            return 0;
        }

        Data::Instance<MaterialShaderLibraries>& GetOrCreateShaderLibraries(
            RHI::DrawListTag drawListTag, RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem);

        Data::Instance<RayTracingMaterialShaderLibrary> GetOrAppendClosestHitShader(
            MaterialShaderLibraries* shaderLibraries, RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem);

        void SetNeedsUpdate(const bool needsUpdate)
        {
            m_needsUpdate = needsUpdate;
        }

        const bool NeedsUpdate() const
        {
            return m_needsUpdate;
        }

        void UpdateShaderLibraries(RayTracingFeatureProcessor* rtfp);

    private:
        void InitializeCommonShaders(
            MaterialShaderLibraries* shaderLibraries, RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem);

        AZStd::unordered_map<RHI::DrawListTag, Data::Instance<MaterialShaderLibraries>> m_shaderLibraries;
        bool m_needsUpdate = false;
    };

    // We have one MeshRayTracingMaterialShaderLibraries instance per raytracing - mesh, that holds a reference to the
    // MaterialShaderLibraries - entry for each DrawListTag, and also a reference to the hit-shader - entry
    // relevant for this mesh only
    class MeshRayTracingMaterialShaderLibraries
    {
    public:
        MeshRayTracingMaterialShaderLibraries() = default;
        MeshRayTracingMaterialShaderLibraries(
            Data::Instance<RPI::ModelLod> modelLod, size_t modelLodMeshIndex, Data::Instance<RPI::Material> materialOverride);

        AZ_DEFAULT_COPY(MeshRayTracingMaterialShaderLibraries);
        AZ_DEFAULT_MOVE(MeshRayTracingMaterialShaderLibraries);

        // The shader library contains one RayGen, one Miss and all Hit-Shaders for all materials.
        auto GetShaderLibraries(const RHI::DrawListTag& drawList) -> Data::Instance<MaterialShaderLibraries>;

        // The hit shader relevant for this sub-mesh. This is one of the hit-shaders in the ShaderLibraries
        auto GetHitShader(const RHI::DrawListTag& drawList) -> Data::Instance<RayTracingMaterialShaderLibrary>;

        // The hit shader index for the TLAS entry for this mesh
        int32_t GetHitGroupIndex() const;

        void Update(RayTracingMaterialShaderManager* manager);

    private:
        void UpdateShaderLibraries(MaterialShaderLibraries* shaderLibraries, const RPI::ShaderCollection::Item& shaderItem);

        AZStd::unordered_map<RHI::DrawListTag, Data::Instance<MaterialShaderLibraries>> m_shaderLibraries;
        AZStd::unordered_map<RHI::DrawListTag, Data::Instance<RayTracingMaterialShaderLibrary>> m_hitShaders;
        Data::Instance<RPI::ModelLod> m_modelLod;
        size_t m_modelLodMeshIndex;
        Data::Instance<RPI::Material> m_material;
        // Tracks whether the Material has change since the DrawPacket was last built
        RPI::Material::ChangeId m_materialChangeId = RPI::Material::DEFAULT_CHANGE_ID;
    };

} // namespace AZ::Render