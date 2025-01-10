/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/MeshDrawPacket.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Shader/ShaderSystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Reflect/Material/MaterialFunctor.h>
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzCore/Console/Console.h>
#include <Atom/RPI.Public/Shader/ShaderReloadDebugTracker.h>

namespace AZ
{
    namespace RPI
    {
        AZ_CVAR(bool,
            r_forceRootShaderVariantUsage,
            false,
            [](const bool&) { AZ::Interface<AZ::IConsole>::Get()->PerformCommand("MeshFeatureProcessor.ForceRebuildDrawPackets"); },
            ConsoleFunctorFlags::Null,
            "(For Testing) Forces usage of root shader variant in the mesh draw packet level, ignoring any other shader variants that may exist."
        );

        MeshDrawPacket::MeshDrawPacket(
            ModelLod& modelLod,
            size_t modelLodMeshIndex,
            Data::Instance<Material> materialOverride,
            Data::Instance<ShaderResourceGroup> objectSrg,
            const MaterialModelUvOverrideMap& materialModelUvMap
        )
            : m_modelLod(&modelLod)
            , m_modelLodMeshIndex(modelLodMeshIndex)
            , m_objectSrg(objectSrg)
            , m_material(materialOverride)
            , m_materialModelUvMap(materialModelUvMap)
        {
            if (!m_material)
            {
                m_material = GetMesh().m_material;
            }

            // set to all true so no items would be skipped
            m_drawListFilter.set();
        }

        Data::Instance<Material> MeshDrawPacket::GetMaterial() const
        {
            return m_material;
        }

        const ModelLod::Mesh& MeshDrawPacket::GetMesh() const
        {
            AZ_Assert(m_modelLodMeshIndex < m_modelLod->GetMeshes().size(), "m_modelLodMeshIndex %zu is out of range %zu", m_modelLodMeshIndex, m_modelLod->GetMeshes().size());
            return m_modelLod->GetMeshes()[m_modelLodMeshIndex];
        }

        void MeshDrawPacket::ForValidShaderOptionName(const Name& shaderOptionName, const AZStd::function<bool(const ShaderCollection::Item&, ShaderOptionIndex)>& callback)
        {
            m_material->ForAllShaderItems(
                [&](const Name&, const ShaderCollection::Item& shaderItem)
                {
                    const ShaderOptionGroupLayout* layout = shaderItem.GetShaderOptions()->GetShaderOptionLayout();
                    ShaderOptionIndex index = layout->FindShaderOptionIndex(shaderOptionName);
                    if (index.IsValid())
                    {
                        bool shouldContinue = callback(shaderItem, index);
                        if (!shouldContinue)
                        {
                            return false;
                        }
                    }
                    return true;
                });
        }

        void MeshDrawPacket::SetStencilRef(uint8_t stencilRef)
        {
            if (m_stencilRef != stencilRef)
            {
                m_needUpdate = true;
                m_stencilRef = stencilRef;
            }
        }

        void MeshDrawPacket::SetSortKey(RHI::DrawItemSortKey sortKey)
        {
            if (m_sortKey != sortKey)
            {
                m_needUpdate = true;
                m_sortKey = sortKey;
            }
        }

        bool MeshDrawPacket::SetShaderOption(const Name& shaderOptionName, ShaderOptionValue value)
        {
            // check if the material owns this option in any of its shaders, if so it can't be set externally
            if (m_material->MaterialOwnsShaderOption(shaderOptionName))
            {
                return false;
            }

            // Try to find an existing option entry in the list
            for (ShaderOptionPair& shaderOptionPair : m_shaderOptions)
            {
                if (shaderOptionPair.first == shaderOptionName)
                {
                    shaderOptionPair.second = value;
                    m_needUpdate = true;
                    return true;
                }
            }

            // Shader option isn't on the list, look to see if it's even valid for at least one shader item, and if so, add it.
            ForValidShaderOptionName(shaderOptionName,
                [&]([[maybe_unused]] const ShaderCollection::Item& shaderItem, [[maybe_unused]] ShaderOptionIndex index)
                {
                    // Store the option name and value, they will be used in DoUpdate() to select the appropriate shader variant
                    m_shaderOptions.push_back({ shaderOptionName, value });
                    return false; // stop checking other shader items.
                }
            );

            m_needUpdate = true;
            return true;
        }

        bool MeshDrawPacket::UnsetShaderOption(const Name& shaderOptionName)
        {
            // try to find an existing option entry in the list, then remove it by swapping it with the back.
            for (ShaderOptionPair& shaderOptionPair : m_shaderOptions)
            {
                if (shaderOptionPair.first == shaderOptionName)
                {
                    shaderOptionPair = m_shaderOptions.back();
                    m_shaderOptions.pop_back();
                    m_needUpdate = true;
                    return true;
                }
            }
            return false;
        }

        void MeshDrawPacket::ClearShaderOptions()
        {
            m_needUpdate = m_shaderOptions.size() > 0;
            m_shaderOptions.clear();
        }

        void MeshDrawPacket::SetEnableDraw(RHI::DrawListTag drawListTag, bool enableDraw)
        {
            if (drawListTag.IsNull())
            {
                return;
            }

            uint8_t index = drawListTag.GetIndex();
            if (m_drawListFilter[index] != enableDraw)
            {
                m_needUpdate = true;
                m_drawListFilter[index] = enableDraw;
            }
        }

        RHI::DrawListMask MeshDrawPacket::GetDrawListFilter()
        {
            return m_drawListFilter;
        }

        void MeshDrawPacket::ClearDrawListFilter()
        {
            m_drawListFilter.set();
            m_needUpdate = true;
        }

        bool MeshDrawPacket::Update(const Scene& parentScene, bool forceUpdate /*= false*/)
        {
            // Setup the Shader variant handler when update this MeshDrawPacket the first time .
            // This is because the MeshDrawPacket data can be copied or moved right after it's created.
            // The m_shaderVariantHandler won't be copied correctly due to the capture of 'this' pointer.
            // Instead of override all the copy and move operators, this might be a better solution.
            if (!m_shaderVariantHandler.IsConnected())
            {
                m_shaderVariantHandler = Material::OnMaterialShaderVariantReadyEvent::Handler(
                    [this]()
                    {
                        this->m_needUpdate = true;
                    });
                m_material->ConnectEvent(m_shaderVariantHandler);
            }

            // Why we need to check "!m_material->NeedsCompile()"...
            //    Frame A:
            //      - Material::SetPropertyValue("foo",...). This bumps the material's CurrentChangeId()
            //      - Material::Compile() updates all the material's outputs (SRG data, shader selection, shader options, etc).
            //      - Material::SetPropertyValue("bar",...). This bumps the materials' CurrentChangeId() again.
            //      - We do not process Material::Compile() a second time because you can only call SRG::Compile() once per frame. Material::Compile()
            //        will be processed on the next frame. (See implementation of Material::Compile())
            //      - MeshDrawPacket::Update() is called. It runs DoUpdate() to rebuild the draw packet, but everything is still in the state when "foo" was
            //        set. The "bar" changes haven't been applied yet. It also sets m_materialChangeId to GetCurrentChangeId(), which corresponds to "bar" not "foo".
            //    Frame B:
            //      - Something calls Material::Compile(). This finally updates the material's outputs with the latest data corresponding to "bar".
            //      - MeshDrawPacket::Update() is called. But since the GetCurrentChangeId() hasn't changed since last time, DoUpdate() is not called.
            //      - The mesh continues rendering with only the "foo" change applied, indefinitely.

            if (forceUpdate || (!m_material->NeedsCompile() && m_materialChangeId != m_material->GetCurrentChangeId())
                || m_needUpdate)
            {
                DoUpdate(parentScene);
                m_materialChangeId = m_material->GetCurrentChangeId();
                m_needUpdate = false;

                DebugOutputShaderVariants();
                return true;
            }

            return false;
        }

        static bool HasRootConstants(const RHI::ConstantsLayout* rootConstantsLayout)
        {
            return rootConstantsLayout && rootConstantsLayout->GetDataSize() > 0;
        }

        void MeshDrawPacket::DebugOutputShaderVariants()
        {
#ifdef DEBUG_MESH_SHADERVARIANTS
            uint32_t index = 0;

            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetInfoById, m_modelLod->GetAssetId());

            AZ_TracePrintf("MeshDrawPacket", "Mesh: %s", assetInfo.m_relativePath.data());
            for (const auto& variant : m_shaderVariantNames)
            {
                AZ_TracePrintf("MeshDrawPacket", "%d: %s", index++, variant.data());
            }
#endif
        }

        bool MeshDrawPacket::DoUpdate(const Scene& parentScene)
        {
            auto meshes = m_modelLod->GetMeshes();
            ModelLod::Mesh& mesh = meshes[m_modelLodMeshIndex];

            if (!m_material)
            {
                AZ_Warning("MeshDrawPacket", false, "No material provided for mesh. Skipping.");
                return false;
            }

            ShaderReloadDebugTracker::ScopedSection reloadSection("MeshDrawPacket::DoUpdate");

            RHI::DrawPacketBuilder drawPacketBuilder{RHI::MultiDevice::AllDevices};
            drawPacketBuilder.Begin(nullptr);
            drawPacketBuilder.SetGeometryView(&mesh);
            drawPacketBuilder.AddShaderResourceGroup(m_objectSrg->GetRHIShaderResourceGroup());
            drawPacketBuilder.AddShaderResourceGroup(m_material->GetRHIShaderResourceGroup());

            // We build the list of used shaders in a local list rather than m_activeShaders so that
            // if DoUpdate() fails it won't modify any member data.
            MeshDrawPacket::ShaderList shaderList;
            shaderList.reserve(m_activeShaders.size());

            // The root constants are shared by all draw items in the draw packet. We must populate them with default values.
            // The draw packet builder needs to know where the data is coming from during appendShader, but it's not actually read
            // until drawPacketBuilder.End(), so store the default data out here.
            AZStd::vector<uint8_t> rootConstants;
            bool isFirstShaderItem = true;

            m_perDrawSrgs.clear();

#ifdef DEBUG_MESH_SHADERVARIANTS
            m_shaderVariantNames.clear();
#endif

            auto appendShader = [&](const ShaderCollection::Item& shaderItem, const Name& materialPipelineName)
            {
                // Skip the shader item without creating the shader instance
                // if the mesh is not going to be rendered based on the draw tag
                RHI::RHISystemInterface* rhiSystem = RHI::RHISystemInterface::Get();
                RHI::DrawListTagRegistry* drawListTagRegistry = rhiSystem->GetDrawListTagRegistry();

                // Use the explicit draw list override if exists.
                RHI::DrawListTag drawListTag = shaderItem.GetDrawListTagOverride();

                if (drawListTag.IsNull())
                {
                    Data::Asset<RPI::ShaderAsset> shaderAsset = shaderItem.GetShaderAsset();
                    if (!shaderAsset.IsReady())
                    {
                        // The shader asset needs to be loaded before we can check the draw tag.
                        // If it's not loaded yet, the instance database will do a blocking load
                        // when we create the instance below, so might as well load it now.
                        shaderAsset.QueueLoad();

                        if (shaderAsset.IsLoading())
                        {
                            shaderAsset.BlockUntilLoadComplete();
                        }
                    }

                    drawListTag = drawListTagRegistry->FindTag(shaderAsset->GetDrawListName());
                }

                // draw list tag is filtered out. skip this item
                if (drawListTag.IsNull() || !m_drawListFilter[drawListTag.GetIndex()])
                {
                    return false;
                }

                const bool isRasterShader = (shaderItem.GetShaderAsset()->GetPipelineStateType() == RHI::PipelineStateType::Draw);
                if (isRasterShader && !parentScene.HasOutputForPipelineState(drawListTag))
                {
                    // drawListTag not found in this scene, so don't render this item
                    return false;
                }

                Data::Instance<Shader> shader = RPI::Shader::FindOrCreate(shaderItem.GetShaderAsset());
                if (!shader)
                {
                    AZ_Error("MeshDrawPacket", false, "Shader '%s'. Failed to find or create instance", shaderItem.GetShaderAsset()->GetName().GetCStr());
                    return false;
                }

                RPI::ShaderOptionGroup shaderOptions = *shaderItem.GetShaderOptions();

                // Set all unspecified shader options to default values, so that we get the most specialized variant possible.
                // (because FindVariantStableId treats unspecified options as a request specifically for a variant that doesn't specify those options)
                // [GFX TODO][ATOM-3883] We should consider updating the FindVariantStableId algorithm to handle default values for us, and remove this step here.
                // This might not be necessary anymore though, since ShaderAsset::GetDefaultShaderOptions() does this when the material type builder is creating the ShaderCollection.
                shaderOptions.SetUnspecifiedToDefaultValues();

                if (isRasterShader)
                {
                    // [GFX_TODO][ATOM-14476]: according to this usage, we should make the shader input contract uniform across all shader
                    // variants.
                    m_modelLod->CheckOptionalStreams(
                        shaderOptions,
                        shader->GetInputContract(),
                        m_modelLodMeshIndex,
                        m_materialModelUvMap,
                        m_material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap());
                }

                // apply shader options from this draw packet to the ShaderItem
                for (auto& meshShaderOption : m_shaderOptions)
                {
                    Name& name = meshShaderOption.first;
                    RPI::ShaderOptionValue& value = meshShaderOption.second;

                    ShaderOptionIndex index = shaderOptions.FindShaderOptionIndex(name);

                    // Shader options will be applied to any shader item that supports it, even if
                    // not all the shader items in the draw packet support it
                    if (index.IsValid())
                    {
                        shaderOptions.SetValue(name, value);
                    }
                }

                const ShaderVariantId requestedVariantId = shaderOptions.GetShaderVariantId();
                const ShaderVariant& variant = r_forceRootShaderVariantUsage ? shader->GetRootVariant() : shader->GetVariant(requestedVariantId);

#ifdef DEBUG_MESH_SHADERVARIANTS
                m_shaderVariantNames.push_back(variant.GetShaderVariantAsset().GetHint());
#endif

                UvStreamTangentBitmask uvStreamTangentBitmask;
                RHI::StreamBufferIndices streamIndices;
                RHI::PipelineStateDescriptorForDraw pipelineStateDescriptorDraw;

                RHI::PipelineStateDescriptorForDispatch pipelineStateDescriptorDispatch;

                RHI::PipelineStateDescriptor* pipelineStateDescriptor = nullptr;
                if (isRasterShader)
                {
                    variant.ConfigurePipelineState(pipelineStateDescriptorDraw, shaderOptions);
                    pipelineStateDescriptor = &pipelineStateDescriptorDraw;

                    // Render states need to merge the runtime variation.
                    // This allows materials to customize the render states that the shader uses.
                    const RHI::RenderStates& renderStatesOverlay = *shaderItem.GetRenderStatesOverlay();
                    RHI::MergeStateInto(renderStatesOverlay, pipelineStateDescriptorDraw.m_renderStates);

                    if (!m_modelLod->GetStreamsForMesh(
                            pipelineStateDescriptorDraw.m_inputStreamLayout,
                            streamIndices,
                            &uvStreamTangentBitmask,
                            shader->GetInputContract(),
                            m_modelLodMeshIndex,
                            m_materialModelUvMap,
                            m_material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap()))
                    {
                        return false;
                    }
                    parentScene.ConfigurePipelineState(drawListTag, pipelineStateDescriptorDraw);
                }
                else
                {
                    variant.ConfigurePipelineState(pipelineStateDescriptorDispatch, shaderOptions);
                    pipelineStateDescriptor = &pipelineStateDescriptorDispatch;
                }

                Data::Instance<ShaderResourceGroup> drawSrg = shader->CreateDrawSrgForShaderVariant(shaderOptions, false);
                if (drawSrg)
                {
                    // Pass UvStreamTangentBitmask to the shader if the draw SRG has it.

                    AZ::Name shaderUvStreamTangentBitmask = AZ::Name(UvStreamTangentBitmask::SrgName);
                    auto index = drawSrg->FindShaderInputConstantIndex(shaderUvStreamTangentBitmask);

                    if (index.IsValid())
                    {
                        drawSrg->SetConstant(index, uvStreamTangentBitmask.GetFullTangentBitmask());
                    }

                    drawSrg->Compile();
                };

                const RHI::PipelineState* pipelineState = shader->AcquirePipelineState(*pipelineStateDescriptor);
                if (!pipelineState)
                {
                    AZ_Error("MeshDrawPacket", false, "Shader '%s'. Failed to acquire default pipeline state", shaderItem.GetShaderAsset()->GetName().GetCStr());
                    return false;
                }

                const RHI::ConstantsLayout* rootConstantsLayout =
                    pipelineStateDescriptor->m_pipelineLayoutDescriptor->GetRootConstantsLayout();
                if(isFirstShaderItem)
                {
                    if (HasRootConstants(rootConstantsLayout))
                    {
                        m_rootConstantsLayout = rootConstantsLayout;
                        rootConstants.resize(m_rootConstantsLayout->GetDataSize());
                        drawPacketBuilder.SetRootConstants(rootConstants);
                    }

                    isFirstShaderItem = false;
                }
                else
                {
                    AZ_Error(
                        "MeshDrawPacket",
                        (!m_rootConstantsLayout && !HasRootConstants(rootConstantsLayout)) ||
                        (m_rootConstantsLayout && rootConstantsLayout && m_rootConstantsLayout->GetHash() == rootConstantsLayout->GetHash()),
                        "Shader %s has mis-matched root constant layout in material %s. "
                        "All draw items in a draw packet need to share the same root constants layout. This means that each pass "
                        "(e.g. Depth, Shadows, Forward, MotionVectors) for a given materialtype should use the same layout.",
                        shaderItem.GetShaderAsset()->GetName().GetCStr(),
                        m_material->GetAsset().ToString<AZStd::string>().c_str());
                }

                RHI::DrawPacketBuilder::DrawRequest drawRequest;
                drawRequest.m_listTag = drawListTag;
                drawRequest.m_pipelineState = pipelineState;
                if (isRasterShader)
                {
                    drawRequest.m_streamIndices = streamIndices;
                    drawRequest.m_stencilRef = m_stencilRef;
                }
                drawRequest.m_sortKey = m_sortKey;
                if (drawSrg)
                {
                    drawRequest.m_uniqueShaderResourceGroup = drawSrg->GetRHIShaderResourceGroup();
                    // Hold on to a reference to the drawSrg so the refcount doesn't drop to zero
                    m_perDrawSrgs.push_back(drawSrg);
                }

                if (materialPipelineName != MaterialPipelineNone)
                {
                    RHI::DrawFilterTag pipelineTag = parentScene.GetDrawFilterTagRegistry()->AcquireTag(materialPipelineName);
                    AZ_Assert(pipelineTag.IsValid(), "Could not acquire pipeline filter tag '%s'.", materialPipelineName.GetCStr());
                    drawRequest.m_drawFilterMask = 1 << pipelineTag.GetIndex();
                }

                drawPacketBuilder.AddDrawItem(drawRequest);

                ShaderData shaderData;
                shaderData.m_shader = AZStd::move(shader);
                shaderData.m_materialPipelineName = materialPipelineName;
                shaderData.m_shaderTag = shaderItem.GetShaderTag();
                shaderData.m_requestedShaderVariantId = requestedVariantId;
                shaderData.m_activeShaderVariantId = variant.GetShaderVariantId();
                shaderData.m_activeShaderVariantStableId = variant.GetStableId();
                shaderList.emplace_back(AZStd::move(shaderData));

                return true;
            }; // appendShader

            m_material->ApplyGlobalShaderOptions();

            // TODO(MaterialPipeline): We might want to detect duplicate ShaderItem objects here, and merge them to avoid redundant RHI DrawItems.
            m_material->ForAllShaderItems(
                [&](const Name& materialPipelineName, const ShaderCollection::Item& shaderItem)
                {
                    if (shaderItem.IsEnabled())
                    {
                        if (shaderList.size() == RHI::DrawPacketBuilder::DrawItemCountMax)
                        {
                            AZ_Error("MeshDrawPacket", false, "Material has more than the limit of %d active shader items.", RHI::DrawPacketBuilder::DrawItemCountMax);
                            return false;
                        }

                        appendShader(shaderItem, materialPipelineName);
                    }

                    return true;
                });

            m_drawPacket = drawPacketBuilder.End();

            if (m_drawPacket)
            {
                m_activeShaders = shaderList;
                m_materialSrg = m_material->GetRHIShaderResourceGroup();
                return true;
            }
            else
            {
                return false;
            }
        }

        const RHI::ConstPtr<RHI::ConstantsLayout> MeshDrawPacket::GetRootConstantsLayout() const
        {
            return m_rootConstantsLayout;
        }
    } // namespace RPI
} // namespace AZ

