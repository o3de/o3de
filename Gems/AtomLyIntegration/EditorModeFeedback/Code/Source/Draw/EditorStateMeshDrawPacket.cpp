/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Draw/EditorStateMeshDrawPacket.h>

#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/RPI.Public/Scene.h> 
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzCore/Console/Console.h>

namespace AZ::Render
{
    EditorStateMeshDrawPacket::EditorStateMeshDrawPacket(
        RPI::ModelLod& modelLod,
        size_t modelLodMeshIndex,
        Data::Instance<RPI::Material> materialOverride,
        AZ::Name drawList,
        Data::Instance<RPI::ShaderResourceGroup> objectSrg,
        const RPI::MaterialModelUvOverrideMap& materialModelUvMap
    )
        : m_modelLod(&modelLod)
        , m_modelLodMeshIndex(modelLodMeshIndex)
        , m_objectSrg(objectSrg)
        , m_material(materialOverride)
        , m_materialModelUvMap(materialModelUvMap)
    {
        if (!m_material)
        {
            const RPI::ModelLod::Mesh& mesh = m_modelLod->GetMeshes()[m_modelLodMeshIndex];
            m_material = mesh.m_material;
        }

        RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
        RHI::DrawListTagRegistry* drawListTagRegistry = rhiSystem->GetDrawListTagRegistry();
        m_drawListTag = drawListTagRegistry->AcquireTag(drawList);
    }

    Data::Instance<RPI::Material> EditorStateMeshDrawPacket::GetMaterial()
    {
        return m_material;
    }

    bool EditorStateMeshDrawPacket::SetShaderOption(const Name& shaderOptionName, RPI::ShaderOptionValue value)
    {
        // check if the material owns this option in any of its shaders, if so it can't be set externally
        for (auto& shaderItem : m_material->GetShaderCollection())
        {
            const RPI::ShaderOptionGroupLayout* layout = shaderItem.GetShaderOptions()->GetShaderOptionLayout();
            RPI::ShaderOptionIndex index = layout->FindShaderOptionIndex(shaderOptionName);
            if (index.IsValid())
            {
                if (shaderItem.MaterialOwnsShaderOption(index))
                {
                    return false;
                }
            }
        }

        for (auto& shaderItem : m_material->GetShaderCollection())
        {
            const RPI::ShaderOptionGroupLayout* layout = shaderItem.GetShaderOptions()->GetShaderOptionLayout();
            RPI::ShaderOptionIndex index = layout->FindShaderOptionIndex(shaderOptionName);
            if (index.IsValid())
            {
                // try to find an existing option entry in the list
                auto itEntry = AZStd::find_if(m_shaderOptions.begin(), m_shaderOptions.end(), [&shaderOptionName](const ShaderOptionPair& entry)
                {
                    return entry.first == shaderOptionName;
                });

                // store the option name and value, they will be used in DoUpdate() to select the appropriate shader variant
                if (itEntry == m_shaderOptions.end())
                {
                    m_shaderOptions.push_back({ shaderOptionName, value });
                }
                else
                {
                    itEntry->second = value;
                }
            }
        }

        return true;
    }

    bool EditorStateMeshDrawPacket::Update(const RPI::Scene& parentScene, bool forceUpdate /*= false*/)
    {
        // Why we need to check "!m_material->NeedsCompile()"...
        //    Frame A:
        //      - Material::SetPropertyValue("foo",...). This bumps the material's CurrentChangeId()
        //      - Material::Compile() updates all the material's outputs (SRG data, shader selection, shader options, etc).
        //      - Material::SetPropertyValue("bar",...). This bumps the materials' CurrentChangeId() again.
        //      - We do not process Material::Compile() a second time because because you can only call SRG::Compile() once per frame. Material::Compile()
        //        will be processed on the next frame. (See implementation of Material::Compile())
        //      - EditorStateMeshDrawPacket::Update() is called. It runs DoUpdate() to rebuild the draw packet, but everything is still in the state when "foo" was
        //        set. The "bar" changes haven't been applied yet. It also sets m_materialChangeId to GetCurrentChangeId(), which corresponds to "bar" not "foo".
        //    Frame B:
        //      - Something calls Material::Compile(). This finally updates the material's outputs with the latest data corresponding to "bar".
        //      - EditorStateMeshDrawPacket::Update() is called. But since the GetCurrentChangeId() hasn't changed since last time, DoUpdate() is not called.
        //      - The mesh continues rendering with only the "foo" change applied, indefinitely.

        if (forceUpdate || (!m_material->NeedsCompile() && m_materialChangeId != m_material->GetCurrentChangeId()))
        {
            DoUpdate(parentScene);
            m_materialChangeId = m_material->GetCurrentChangeId();
            return true;
        }

        return false;
    }

    bool EditorStateMeshDrawPacket::DoUpdate(const RPI::Scene& parentScene)
    {
        const RPI::ModelLod::Mesh& mesh = m_modelLod->GetMeshes()[m_modelLodMeshIndex];

        if (!m_material)
        {
            AZ_Warning("EditorStateMeshDrawPacket", false, "No material provided for mesh. Skipping.");
            return false;
        }

        RHI::DrawPacketBuilder drawPacketBuilder;
        drawPacketBuilder.Begin(nullptr);

        drawPacketBuilder.SetDrawArguments(mesh.m_drawArguments);
        drawPacketBuilder.SetIndexBufferView(mesh.m_indexBufferView);
        drawPacketBuilder.AddShaderResourceGroup(m_objectSrg->GetRHIShaderResourceGroup());
        drawPacketBuilder.AddShaderResourceGroup(m_material->GetRHIShaderResourceGroup());

        // We build the list of used shaders in a local list rather than m_activeShaders so that
        // if DoUpdate() fails it won't modify any member data.
        EditorStateMeshDrawPacket::ShaderList shaderList;
        shaderList.reserve(m_activeShaders.size());

        // We have to keep a list of these outside the loops that collect all the shaders because the DrawPacketBuilder
        // keeps pointers to StreamBufferViews until DrawPacketBuilder::End() is called. And we use a fixed_vector to guarantee
        // that the memory won't be relocated when new entries are added.
        AZStd::fixed_vector<RPI::ModelLod::StreamBufferViewList, RHI::DrawPacketBuilder::DrawItemCountMax> streamBufferViewsPerShader;

        m_perDrawSrgs.clear();

        auto appendShader = [&](const RPI::ShaderCollection::Item& shaderItem)
        {
            if (!parentScene.HasOutputForPipelineState(m_drawListTag))
            {
                // drawListTag not found in this scene, so don't render this item
                return false;
            }

            Data::Instance<RPI::Shader> shader = RPI::Shader::FindOrCreate(shaderItem.GetShaderAsset());
            if (!shader)
            {
                AZ_Error("EditorStateMeshDrawPacket", false, "Shader '%s'. Failed to find or create instance", shaderItem.GetShaderAsset()->GetName().GetCStr());
                return false;
            }

            // Set all unspecified shader options to default values, so that we get the most specialized variant possible.
            // (because FindVariantStableId treats unspecified options as a request specifically for a variant that doesn't specify those options)
            // [GFX TODO][ATOM-3883] We should consider updating the FindVariantStableId algorithm to handle default values for us, and remove this step here.
            RPI::ShaderOptionGroup shaderOptions = *shaderItem.GetShaderOptions();
            shaderOptions.SetUnspecifiedToDefaultValues();

            // [GFX_TODO][ATOM-14476]: according to this usage, we should make the shader input contract uniform across all shader variants.
            m_modelLod->CheckOptionalStreams(
                shaderOptions,
                shader->GetInputContract(),
                m_modelLodMeshIndex,
                m_materialModelUvMap,
                m_material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap());

            // apply shader options from this draw packet to the ShaderItem
            for (auto& meshShaderOption : m_shaderOptions)
            {
                Name& name = meshShaderOption.first;
                RPI::ShaderOptionValue& value = meshShaderOption.second;

                RPI::ShaderOptionIndex index = shaderOptions.FindShaderOptionIndex(name);
                if (index.IsValid())
                {
                    shaderOptions.SetValue(name, value);
                }
            }

            const RPI::ShaderVariantId finalVariantId = shaderOptions.GetShaderVariantId();
            const RPI::ShaderVariant& variant = shader->GetVariant(finalVariantId);

            RHI::PipelineStateDescriptorForDraw pipelineStateDescriptor;
            variant.ConfigurePipelineState(pipelineStateDescriptor);

            // Render states need to merge the runtime variation.
            // This allows materials to customize the render states that the shader uses.
            const RHI::RenderStates& renderStatesOverlay = *shaderItem.GetRenderStatesOverlay();
            RHI::MergeStateInto(renderStatesOverlay, pipelineStateDescriptor.m_renderStates);

            streamBufferViewsPerShader.emplace_back();
            auto& streamBufferViews = streamBufferViewsPerShader.back();

            RPI::UvStreamTangentBitmask uvStreamTangentBitmask;

            if (!m_modelLod->GetStreamsForMesh(
                pipelineStateDescriptor.m_inputStreamLayout,
                streamBufferViews,
                &uvStreamTangentBitmask,
                shader->GetInputContract(),
                m_modelLodMeshIndex,
                m_materialModelUvMap,
                m_material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap()))
            {
                return false;
            }

            auto drawSrgLayout = shader->GetAsset()->GetDrawSrgLayout(shader->GetSupervariantIndex());
            Data::Instance<RPI::ShaderResourceGroup> drawSrg;
            if (drawSrgLayout)
            {
                // If the DrawSrg exists we must create and bind it, otherwise the CommandList will fail validation for SRG being null
                drawSrg = RPI::ShaderResourceGroup::Create(shader->GetAsset(), shader->GetSupervariantIndex(), drawSrgLayout->GetName());

                if (!variant.IsFullyBaked() && drawSrgLayout->HasShaderVariantKeyFallbackEntry())
                {
                    drawSrg->SetShaderVariantKeyFallbackValue(shaderOptions.GetShaderVariantKeyFallbackValue());
                }

                // Pass UvStreamTangentBitmask to the shader if the draw SRG has it.
                {
                    AZ::Name shaderUvStreamTangentBitmask = AZ::Name(RPI::UvStreamTangentBitmask::SrgName);
                    auto index = drawSrg->FindShaderInputConstantIndex(shaderUvStreamTangentBitmask);

                    if (index.IsValid())
                    {
                        drawSrg->SetConstant(index, uvStreamTangentBitmask.GetFullTangentBitmask());
                    }
                }

                drawSrg->Compile();
            }

            parentScene.ConfigurePipelineState(m_drawListTag, pipelineStateDescriptor);

            const RHI::PipelineState* pipelineState = shader->AcquirePipelineState(pipelineStateDescriptor);
            if (!pipelineState)
            {
                AZ_Error("EditorStateMeshDrawPacket", false, "Shader '%s'. Failed to acquire default pipeline state", shaderItem.GetShaderAsset()->GetName().GetCStr());
                return false;
            }

            RHI::DrawPacketBuilder::DrawRequest drawRequest;
            drawRequest.m_listTag = m_drawListTag;
            drawRequest.m_pipelineState = pipelineState;
            drawRequest.m_streamBufferViews = streamBufferViews;
            drawRequest.m_stencilRef = m_stencilRef;
            drawRequest.m_sortKey = m_sortKey;
            if (drawSrg)
            {
                drawRequest.m_uniqueShaderResourceGroup = drawSrg->GetRHIShaderResourceGroup();
                m_perDrawSrgs.push_back(drawSrg);
            }
            drawPacketBuilder.AddDrawItem(drawRequest);

            shaderList.emplace_back(AZStd::move(shader));

            return true;
        };

        // [GFX TODO][ATOM-5625] This really needs to be optimized to put the burden on setting global shader options, not applying global shader options.
        // For example, make the shader system collect a map of all shaders and ShaderVaraintIds, and look up the shader option names at set-time.
        RPI::ShaderSystemInterface* shaderSystem = RPI::ShaderSystemInterface::Get();
        for (auto iter : shaderSystem->GetGlobalShaderOptions())
        {
            const AZ::Name& shaderOptionName = iter.first;
            RPI::ShaderOptionValue value = iter.second;
            if (!m_material->SetSystemShaderOption(shaderOptionName, value).IsSuccess())
            {
                AZ_Warning("EditorStateMeshDrawPacket", false, "Shader option '%s' is owned by this this material. Global value for this option was ignored.", shaderOptionName.GetCStr());
            }
        }

        for (auto& shaderItem : m_material->GetShaderCollection())
        {
            if (shaderItem.IsEnabled())
            {
                if (shaderList.size() == RHI::DrawPacketBuilder::DrawItemCountMax)
                {
                    AZ_Error("EditorStateMeshDrawPacket", false, "Material has more than the limit of %d active shader items.", RHI::DrawPacketBuilder::DrawItemCountMax);
                    return false;
                }

                appendShader(shaderItem);
            }
        }

        m_drawPacket = drawPacketBuilder.End();

        if (m_drawPacket)
        {
            m_activeShaders = shaderList;
            m_materialSrg = m_material->GetRHIShaderResourceGroup();
            return true;
        }
        else
        {
            AZ_Error("EditorStateMeshDrawPacket", false, "Invalid draw packet generated.");
            return false;
        }
    }

    const RHI::DrawPacket* EditorStateMeshDrawPacket::GetRHIDrawPacket() const
    {
        return m_drawPacket.get();
    }
} // namespace AZ::Render
