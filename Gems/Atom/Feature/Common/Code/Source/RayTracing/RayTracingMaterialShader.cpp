/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/Format.h>
#include <AzCore/Name/Name.h>

#include <Atom/RHI/RHIUtils.h>
#include <RayTracing/RayTracingFeatureProcessor.h>
#include <RayTracing/RayTracingMaterialShader.h>

namespace AZ::Render
{

    RayTracingMaterialShaderLibrary::~RayTracingMaterialShaderLibrary()
    {
        RPI::ShaderReloadNotificationBus::Handler::BusDisconnect();
    }

    void RayTracingMaterialShaderLibrary::Init(
        const RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem, const Name& entryFunction, const int shaderIndex)
    {
        auto shaderAsset = shaderItem.GetShaderAsset();
        AZ_Assert(shaderAsset.IsReady(), "Failed to load shader %s", shaderAsset.GetHint().c_str());

        // TODO: do we need to hold on to the Shader or ShaderVariant here?
        m_shader = RPI::Shader::FindOrCreate(shaderAsset);
        AZ_Assert(m_shader, "Failed to create shader from asset %s", shaderAsset.GetHint().c_str());
        m_drawListTag = m_shader->GetDrawListTag();
        m_shaderVariantId = shaderItem.GetShaderVariantId();

        RPI::ShaderReloadNotificationBus::Handler::BusConnect(m_shader->GetAssetId());

        auto shaderVariant = m_shader->GetVariant(m_shaderVariantId);
        auto shaderVariantAsset = shaderVariant.GetShaderVariantAsset();
        auto shaderStageFunction = shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::RayTracing);
        AZ_Assert(shaderStageFunction != nullptr, "Shader %s has no raytracing shader functions");

        shaderVariant.ConfigurePipelineState(m_descriptor, *shaderItem.GetShaderOptions());

        // TODO: we are using other entry-funktions than the closestHitShader
        m_entryFunction = entryFunction;
        m_shaderIndex = shaderIndex;
    }

    void RayTracingMaterialShaderLibrary::OnShaderReload()
    {
        // we mark this shader library as stale, and tell the shader-manager we need an update. We don't actually do a reload, the
        // shader-manager will create a new draw-item, triggered by the first MeshRayTracingMaterialShaderLibraries - entry
        m_isStale = true;
        if (m_shaderManager)
        {
            m_shaderManager->SetNeedsUpdate(true);
        }
    }

    // ShaderReloadNotificationBus::Handler overrides...
    void RayTracingMaterialShaderLibrary::OnShaderReinitialized(const RPI::Shader& shader)
    {
        OnShaderReload();
    }
    void RayTracingMaterialShaderLibrary::OnShaderAssetReinitialized(const Data::Asset<RPI::ShaderAsset>& shaderAsset)
    {
        OnShaderReload();
    }
    void RayTracingMaterialShaderLibrary::OnShaderVariantReinitialized(const RPI::ShaderVariant& shaderVariant)
    {
        OnShaderReload();
    }

    void MaterialShaderLibraries::CreateGlobalPipelineState()
    {
        auto& shader = m_rayGenShader->GetShader();
        m_globalPipelineState = shader->AcquirePipelineState(m_rayGenShader->GetPipelineStateDescriptor());
    }

    void MaterialShaderLibraries::CreateRayTracingPipelineState(
        const uint32_t maxPayloadSize, const uint32_t maxAttributeSize, const uint32_t maxRecursionDepth)
    {
        RHI::RayTracingPipelineStateDescriptor descriptor;

        descriptor.m_pipelineState = m_globalPipelineState.get();

        descriptor.m_configuration.m_maxPayloadSize = maxPayloadSize;
        descriptor.m_configuration.m_maxAttributeSize = maxAttributeSize;
        descriptor.m_configuration.m_maxRecursionDepth = maxRecursionDepth;

        {
            auto& lib = descriptor.m_shaderLibraries.emplace_back();
            lib.m_descriptor = m_rayGenShader->GetPipelineStateDescriptor();
            lib.m_rayGenerationShaderName = m_rayGenShader->GetEntryFunctionName();
        }
        for (auto& [_, shaderLib] : m_hitShaders)
        {
            // create a shader library that contains the bytecode for this hit-shader
            auto& lib = descriptor.m_shaderLibraries.emplace_back();
            lib.m_descriptor = shaderLib->GetPipelineStateDescriptor();
            // we need to make sure the shader name is unique, since we are using this as lookup for the shader indices.
            lib.m_closestHitShaderName =
                Name(AZStd::string::format("%s_%d", shaderLib->GetEntryFunctionName().GetCStr(), shaderLib->GetShaderIndex()));
            // the actual entry function in the bytecode
            lib.m_closestHitEntryFunctionName = shaderLib->GetEntryFunctionName();

            // TODO: We need to deal with AnyHit, ProceduralIntersection and Callable Shaders here

            // create a hit-group for this closestHit - shader
            auto& hitGroup = descriptor.m_hitGroups.emplace_back();
            hitGroup.m_hitGroupName = Name(AZStd::string::format("HitGroup_%d", shaderLib->GetShaderIndex()));
            hitGroup.m_closestHitShaderName = lib.m_closestHitShaderName;
        }
        {
            auto& lib = descriptor.m_shaderLibraries.emplace_back();
            lib.m_descriptor = m_missShader->GetPipelineStateDescriptor();
            lib.m_missShaderName = m_missShader->GetEntryFunctionName();
        }
        // create the ray tracing pipeline state object
        m_rayTracingPipelineState = aznew RHI::RayTracingPipelineState;
        m_rayTracingPipelineState->Init(RHI::RHISystemInterface::Get()->GetRayTracingSupport(), descriptor);
    }

    Data::Instance<RPI::ShaderResourceGroup> MaterialShaderLibraries::CreatePassSrg() const
    {
        auto& shader = m_rayGenShader->GetShader();
        auto& layout = shader->FindShaderResourceGroupLayout(RPI::SrgBindingSlot::Pass);
        if (layout)
        {
            return RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), layout->GetName());
        }
        return nullptr;
    }

    void MaterialShaderLibraries::CreateRayTracingShaderTable(RayTracingFeatureProcessor* rtfp)
    {
        // scene changed, need to rebuild the shader table
        m_rayTracingShaderTable = aznew RHI::RayTracingShaderTable();
        m_rayTracingShaderTable->Init(RHI::RHISystemInterface::Get()->GetRayTracingSupport(), rtfp->GetBufferPools());

        AZStd::shared_ptr<RHI::RayTracingShaderTableDescriptor> descriptor = AZStd::make_shared<RHI::RayTracingShaderTableDescriptor>();

        // build the ray tracing shader table descriptor
        descriptor->m_name = Name("RayTracingShaderTable");
        descriptor->m_rayTracingPipelineState = m_rayTracingPipelineState;
        descriptor->m_rayGenerationRecord.emplace_back(m_rayGenShader->GetEntryFunctionName());
        descriptor->m_missRecords.emplace_back(m_missShader->GetEntryFunctionName());

        AZStd::vector<Name> sortedHitGroupNames(m_hitShaders.size());

        for (auto& [_, hitShader] : m_hitShaders)
        {
            sortedHitGroupNames[hitShader->GetShaderIndex()] = Name(AZStd::string::format("HitGroup_%d", hitShader->GetShaderIndex()));
        }
        for (auto& hitGroupName : sortedHitGroupNames)
        {
            descriptor->m_hitGroupRecords.emplace_back(hitGroupName);
        }

        // add a hit group for procedural meshes to the shader table
        const auto& proceduralGeometryTypes = rtfp->GetProceduralGeometryTypes();
        for (auto it = proceduralGeometryTypes.cbegin(); it != proceduralGeometryTypes.cend(); ++it)
        {
            descriptor->m_hitGroupRecords.emplace_back(it->m_name);
        }

        m_rayTracingShaderTable->Build(descriptor);
    }

    bool MaterialShaderLibraries::RequiresSrg(const uint32_t bindingSlot) const
    {
        auto& shader = m_rayGenShader->GetShader();
        return shader->FindShaderResourceGroupLayout(bindingSlot) != nullptr;
    }

    bool MaterialShaderLibraries::RequiresSrg(const Name& bindingSlotName) const
    {
        auto& shader = m_rayGenShader->GetShader();
        return shader->FindShaderResourceGroupLayout(bindingSlotName) != nullptr;
    }

    auto MaterialShaderLibraries::CalculateMaterialHitShaderId(const RPI::Material* material) -> HitShaderId
    {
        HitShaderId seed{};
        AZStd::hash_combine(seed, material->GetMaterialTypeId());

        // The HitShaderId we calculate here is used to determine if multiple meshes can share the same hit-shader id within a DrawListTag.
        // But since a mesh is registered in the TLAS with exactly one hit-shader index, this can lead to problems:
        // Assume the following setup:
        //   - Material "mat_a" has shaders for two different Raytracing DrawListTags, "tag_a", and "tag_b".
        //   - Mesh "mesh_a" uses mat_a
        //   - Mesh "mesh_b" also uses mat_a, but different shader-options only for tag_b.
        // This means for the pass drawing tag_a, both meshes can use the same hit-shader (= have the same hit-shader index in the TLAS)
        // However, for the pass drawing tag_b, they need different hit-shaders, but since we have only one global TLAS, we now cannot
        // assign a different hit-shader index. To get arround this, we take all shader options for all DrawListTags into account, and make
        // sure a mesh always ends up with the same hit-shader index, even if it could share the hit-shader with another mesh for one
        // drawListTag.

        material->ForAllShaderItems(
            [&seed](const Name& materialPipelineName, const RPI::ShaderCollection::Item& shaderItem)
            {
                if (shaderItem.GetDrawItemType() == RPI::ShaderCollection::Item::DrawItemType::RayTracing && shaderItem.IsEnabled())
                {
                    RPI::ShaderOptionGroup shaderOptions = *shaderItem.GetShaderOptions();
                    shaderOptions.SetUnspecifiedToDefaultValues();
                    auto requestedShaderVariantId = shaderOptions.GetShaderVariantId();
                    AZStd::hash_combine(seed, requestedShaderVariantId);
                }
                return true;
            });

        return seed;
    }

    Data::Instance<RayTracingMaterialShaderLibrary> MaterialShaderLibraries::GetOrCreateHitShader(
        const RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem)
    {
        auto shaderId = CalculateMaterialHitShaderId(material);
        if (!m_hitShaders.contains(shaderId))
        {
            auto* shaderLibrary = aznew RayTracingMaterialShaderLibrary{};
            int shaderIndex = static_cast<int>(m_hitShaders.size());
            // TODO: the entry function name should really come from the .azshader file somehow.
            // Until then, the shaders have to use these names.
            shaderLibrary->Init(material, shaderItem, Name("ClosestHitShader"), shaderIndex);
            m_hitShaders[shaderId] = shaderLibrary;
            // we need to recreate the pipeline-state and shader table if we get a new ClosestHitShader
            m_isFinalized = false;
        }
        return m_hitShaders[shaderId];
    }

    Data::Instance<RayTracingMaterialShaderLibrary> MaterialShaderLibraries::GetOrCreateRayGenShader(
        const RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem)
    {
        if (!m_rayGenShader)
        {
            m_rayGenShader = aznew RayTracingMaterialShaderLibrary{};
            // TODO: the entry function name should really come from the .azshader file somehow.
            // Until then, the shaders have to use these names.
            m_rayGenShader->Init(material, shaderItem, Name("RayGeneration"), 0);
            // we need to recreate the pipeline-state and shader table if we get a new shader
            m_isFinalized = false;
        }
        return m_rayGenShader;
    }

    Data::Instance<RayTracingMaterialShaderLibrary> MaterialShaderLibraries::GetOrCreateMissShader(
        const RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem)
    {
        if (!m_missShader)
        {
            m_missShader = aznew RayTracingMaterialShaderLibrary{};
            // TODO: the entry function name should really come from the .azshader file somehow.
            // Until then, the shaders have to use these names.
            m_missShader->Init(material, shaderItem, Name("Miss"), 0);
            // we need to recreate the pipeline-state and shader table if we get a new shader
            m_isFinalized = false;
        }
        return m_missShader;
    }

    void MaterialShaderLibraries::ConnectMaterialShaderManager(RayTracingMaterialShaderManager* shaderManager)
    {
        for (auto [_, hitShader] : m_hitShaders)
        {
            hitShader->ConnectMaterialShaderManager(shaderManager);
        }
        if (m_rayGenShader)
        {
            m_rayGenShader->ConnectMaterialShaderManager(shaderManager);
        }
        if (m_missShader)
        {
            m_missShader->ConnectMaterialShaderManager(shaderManager);
        }
    }

    void MaterialShaderLibraries::Finalize(RayTracingFeatureProcessor* rtfp)
    {
        if (m_isFinalized == false)
        {
            CreateGlobalPipelineState();

            // TODO: these values should come from the compiled shader somehow
            float m_maxPayloadSize = 64;
            float m_maxAttributeSize = 8;
            float m_maxRecursionDepth = 1;
            CreateRayTracingPipelineState(m_maxPayloadSize, m_maxAttributeSize, m_maxRecursionDepth);
            CreateRayTracingShaderTable(rtfp);
            m_isFinalized = true;
        }
    }

    void RayTracingMaterialShaderManager::InitializeCommonShaders(
        MaterialShaderLibraries* shaderLibraries, RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem)
    {
        // Initialize the shaders common for all materials.
        // Note: Even though the RayGeneration and Miss - shaders are compiled once per material,
        // we assume they behave exactly the same, so this will be the shaders from whichever material arrives here first
        auto rayGenShader = shaderLibraries->GetOrCreateRayGenShader(material, shaderItem);
        AZ_Assert(
            rayGenShader != nullptr,
            "Unable to create RayGenerationShader for material %s, shader %s",
            material->GetAsset().GetHint().c_str(),
            shaderItem.GetShaderAsset().GetHint().c_str());

        rayGenShader->ConnectMaterialShaderManager(this);

        auto missShader = shaderLibraries->GetOrCreateMissShader(material, shaderItem);
        AZ_Assert(
            missShader != nullptr,
            "Unable to create MissShader for material %s, shader %s",
            material->GetAsset().GetHint().c_str(),
            shaderItem.GetShaderAsset().GetHint().c_str());

        missShader->ConnectMaterialShaderManager(this);
    }

    Data::Instance<RayTracingMaterialShaderLibrary> RayTracingMaterialShaderManager::GetOrAppendClosestHitShader(
        MaterialShaderLibraries* shaderLibraries, RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem)
    {
        auto hitShader = shaderLibraries->GetOrCreateHitShader(material, shaderItem);
        AZ_Assert(
            hitShader != nullptr,
            "Unable to create ClosestHitShader for material %s, shader %s",
            material->GetAsset().GetHint().c_str(),
            shaderItem.GetShaderAsset().GetHint().c_str());

        hitShader->ConnectMaterialShaderManager(this);

        return hitShader;
    }

    Data::Instance<MaterialShaderLibraries>& RayTracingMaterialShaderManager::GetOrCreateShaderLibraries(
        RHI::DrawListTag drawListTag, RPI::Material* material, const RPI::ShaderCollection::Item& shaderItem)
    {
        Data::Instance<MaterialShaderLibraries> shaderLibraries = nullptr;

        if (!m_shaderLibraries.contains(drawListTag))
        {
            auto shaderLibraries = aznew MaterialShaderLibraries{};
            InitializeCommonShaders(shaderLibraries, material, shaderItem);
            m_shaderLibraries[drawListTag] = shaderLibraries;
        }
        else
        {
            auto currentShaderLibraries = m_shaderLibraries[drawListTag];
            if (currentShaderLibraries->IsStale())
            {
                shaderLibraries = aznew MaterialShaderLibraries{};
                InitializeCommonShaders(shaderLibraries.get(), material, shaderItem);
                // TODO: make sure all shaders of this shader-library are marked as stale before we overwrite it!
                m_shaderLibraries[drawListTag] = shaderLibraries;
            }
        }
        return m_shaderLibraries[drawListTag];
    }

    void RayTracingMaterialShaderManager::UpdateShaderLibraries(RayTracingFeatureProcessor* rtfp)
    {
        for (auto& [drawListTag, shaderLibraries] : m_shaderLibraries)
        {
            shaderLibraries->Finalize(rtfp);
        }
    }

    const bool MaterialShaderLibraries::IsStale() const
    {
        bool isStale = false;
        isStale |= m_rayGenShader->IsStale();
        isStale |= m_missShader->IsStale();
        for (auto& [drawListTag, hitShader] : m_hitShaders)
        {
            isStale |= hitShader->IsStale();
        }
        return isStale;
    }

    MeshRayTracingMaterialShaderLibraries::MeshRayTracingMaterialShaderLibraries(
        Data::Instance<RPI::ModelLod> modelLod, size_t modelLodMeshIndex, Data::Instance<RPI::Material> materialOverride)
        : m_modelLod(modelLod)
        , m_modelLodMeshIndex(modelLodMeshIndex)
    {
        if (materialOverride != nullptr)
        {
            m_material = materialOverride;
        }
        else
        {
            const RPI::ModelLod::Mesh& mesh = m_modelLod->GetMeshes()[modelLodMeshIndex];
            m_material = mesh.m_material;
        }
    }

    auto MeshRayTracingMaterialShaderLibraries::GetHitShader(const RHI::DrawListTag& drawList)
        -> Data::Instance<RayTracingMaterialShaderLibrary>
    {
        if (m_hitShaders.contains(drawList))
        {
            return m_hitShaders.at(drawList);
        }
        return {};
    }

    int32_t MeshRayTracingMaterialShaderLibraries::GetHitGroupIndex() const
    {
        if (m_hitShaders.empty())
        {
            // fallback value: we can't use a negative hit-shader index
            return 0;
        }
        // TODO: ensure that each hit-shader has the same shader index, regardless of the DrawListTag
        return m_hitShaders.begin()->second->GetShaderIndex();
    }

    void MeshRayTracingMaterialShaderLibraries::Update(RayTracingMaterialShaderManager* manager)
    {
        // TODO: this stale-thing needs work.
        // A shader is stale if it was reloaded, but if only the shaders for one material-type were changed, we still discard the
        // hit-shaders of the other unchanged types. But the other meshes won't realize that they need to recreate their hit-shaders.
        bool isStale = false;
        for (auto& [drawListTag, shaderLibraries] : m_shaderLibraries)
        {
            isStale |= shaderLibraries->IsStale();
        }
        for (auto& [drawListTag, hitShader] : m_hitShaders)
        {
            isStale |= hitShader->IsStale();
        }

        if (m_materialChangeId == m_material->GetCurrentChangeId() && isStale == false)
        {
            return;
        }

        // This doesn't mean the shaders will be recreated, since the manager keeps a reference to them
        // But if our material changed sufficiently enough, we will get a new one
        m_shaderLibraries.clear();
        m_hitShaders.clear();

        m_material->ApplyGlobalShaderOptions();

        m_material->ForAllShaderItems(
            [&](const Name& materialPipelineName, const RPI::ShaderCollection::Item& shaderItem)
            {
                if (shaderItem.IsEnabled() && shaderItem.GetDrawItemType() == RPI::ShaderCollection::Item::DrawItemType::RayTracing)
                {
                    auto drawListTagRegistry = RHI::GetDrawListTagRegistry();
                    auto drawListTag = drawListTagRegistry->FindTag(shaderItem.GetShaderAsset()->GetDrawListName());

                    // find or create the shader-libraries for this drawListTag
                    auto shaderLibraries = manager->GetOrCreateShaderLibraries(drawListTag, m_material.get(), shaderItem);
                    // hold on to the shader-libraries
                    m_shaderLibraries[drawListTag] = shaderLibraries;

                    // Append the hit-shader for this mesh-material, if it doesn't exist yet.
                    auto hitShader = manager->GetOrAppendClosestHitShader(shaderLibraries.get(), m_material.get(), shaderItem);
                    // The shader-library already holds the hit-shader, but we keep track of which one is ours.
                    m_hitShaders[drawListTag] = hitShader;
                }
                return true;
            });
        m_materialChangeId = m_material->GetCurrentChangeId();
        return;
    }

} // namespace AZ::Render