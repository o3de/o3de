/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzCore/Math/Aabb.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI.Reflect/BufferViewDescriptor.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/MeshDrawPacket.h>

#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Image/Image.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyValue.h>
#include <Atom/RPI.Reflect/Buffer/BufferAssetCreator.h>
#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Reflect/Model/ModelLodAssetCreator.h>
#include <Atom/RPI.Reflect/Model/ModelMaterialSlot.h>

#include <vector>

#include <MeshletsFeatureProcessor.h>
#include <MeshletsUtilities.h>
#include <MeshletsRenderObject.h>

namespace AZ
{
    namespace Meshlets
    {
        //======================================================================
        //                        MeshletsRenderObject
        //======================================================================
        uint32_t MeshletsRenderObject::s_modelNumber = 0;

        bool MeshletsRenderObject::SetShaders()
        {
            {
                m_computeShader = m_featureProcessor->GetComputeShader();
                if (!m_computeShader)
                {
                    AZ_Error("Meshlets", false, "Failed to get Compute shader");
                    return false;
                }

                m_renderShader = m_featureProcessor->GetRenderShader();
                if (!m_renderShader)
                {
                    AZ_Error("Meshlets", false, "Failed to get Render shader");
                    return false;
                }
            }
            return true;
        }


        void MeshletsRenderObject::PrepareRenderSrgDescriptors(
            MeshRenderData &meshRenderData, uint32_t vertexCount, uint32_t indicesCount)
        {
            if (meshRenderData.RenderBuffersDescriptors.size())
            {
                return;
            }

            meshRenderData.RenderBuffersDescriptors.resize(uint8_t(RenderStreamsSemantics::NumBufferStreams));

            // SP1 fix: ALL render buffers use Format::Unknown to create
            // StructuredBuffer SRVs instead of typed Buffer<T> SRVs. The
            // Atom DX12 backend has a bug in typed-buffer SRV descriptor
            // creation where NumElements is set to 1 for pool-allocated
            // buffers, causing every element read to OOB-clamp to element 0
            // on AMD hardware. StructuredBuffer SRVs use a different DX12
            // code path that sets NumElements correctly.
            //
            // Pool type is ReadOnly (sufficient for CPU-upload + GPU-read).
            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Positions)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown,   // StructuredBuffer<float>
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(float),
                    3 * vertexCount,
                    Name{ "POSITION" }, Name{ "m_positions" }, 0, 0
                );

            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Normals)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown,   // StructuredBuffer<float>
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(float),
                    3 * vertexCount,
                    Name{ "NORMAL" }, Name{ "m_normals" }, 1, 0
                );

            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Tangents)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown,   // StructuredBuffer<float4>
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(float) * 4, vertexCount,
                    Name{ "TANGENT" }, Name{ "m_tangents" }, 2, 0
                );

            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::BiTangents)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown,   // StructuredBuffer<float>
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(float),
                    3 * vertexCount,
                    Name{ "BITANGENT" }, Name{ "m_bitangents" }, 3, 0
                );

            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::UVs)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown,   // StructuredBuffer<float2>
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(float) * 2, vertexCount,
                    Name{ "UV" }, Name{ "m_uvs" }, 4, 0
                );

            meshRenderData.RenderBuffersDescriptors[uint8_t(RenderStreamsSemantics::Indices)] =
                SrgBufferDescriptor(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown,   // StructuredBuffer<uint>
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(uint32_t), indicesCount,
                    Name{ "INDICES" }, Name{ "m_indices" }, 5, 0
                );
        }

        // Notice that unlike the Compute buffers, for the render all the buffers are
        // read only and because of this we can create and bind all in one stage.
        // After the SRG split, this populates the per-object SRG (vertex streams shared
        // by every instance). The per-instance SRG (m_objectId) is created later
        // per-instance by the feature processor.
        bool MeshletsRenderObject::CreateAndBindRenderBuffers(MeshRenderData &meshRenderData)
        {
            // Create the per-object SRG first - required for the buffers generation
            if (m_renderShader)
            {
                meshRenderData.ObjectSrg = RPI::ShaderResourceGroup::Create(
                        m_renderShader->GetAsset(), AZ::Name{ "MeshletsObjectRenderSrg" });
            }
            if (!meshRenderData.ObjectSrg)
            {
                AZ_Error("Meshlets", false, "Failed to create the per-object Render Srg - Meshlets Mesh load will fail");
                return false;
            }

            // Phase 4 VRAM reclaim (streaming-exclusive): keep the (small) per-object
            // SRG but create NONE of the monolithic stream/index buffers. Every path
            // that would consume them gates on the buffers' presence and self-skips.
            if (meshRenderData.MonolithicDropped)
            {
                meshRenderData.RenderBuffers.resize(meshRenderData.RenderBuffersDescriptors.size());
                AZ_TracePrintf("Meshlets",
                    "CreateAndBindRenderBuffers: streaming-exclusive mesh -- %zu monolithic "
                    "stream buffers skipped.\n", meshRenderData.RenderBuffersDescriptors.size());
                return true;
            }

            bool success = true;
            uint32_t streamsNum = (uint32_t) meshRenderData.RenderBuffersDescriptors.size();
            meshRenderData.RenderBuffers.resize(streamsNum);

            // Create and bind each render buffer to the per-object SRG.
            // All buffers use Format::Unknown (StructuredBuffer SRVs) -- the Atom
            // DX12 backend has a bug in typed-buffer SRV creation that sets
            // NumElements=1 for pool-allocated buffers. StructuredBuffer SRVs use
            // a different code path that sets NumElements correctly.
            for (uint32_t stream = 0; stream < streamsNum; ++stream)
            {
                SrgBufferDescriptor& bufferDesc = meshRenderData.RenderBuffersDescriptors[stream];
                if (!bufferDesc.m_bufferData)
                {
                    AZ_Warning("Meshlets", false,
                        "Render stream %u (%s) has no m_bufferData -- skipping.",
                        stream, bufferDesc.m_bufferName.GetCStr());
                    continue;
                }

                meshRenderData.RenderBuffers[stream] = UtilityClass::CreateBufferAndBindToSrg(
                    "Meshlets", bufferDesc, meshRenderData.ObjectSrg);
                if (!meshRenderData.RenderBuffers[stream])
                {
                    success = false;
                }
            }

            // Bind bounds-check constants for GPU-side clamping.
            if (success)
            {
                RHI::ShaderInputConstantIndex indicesCountHandle =
                    meshRenderData.ObjectSrg->FindShaderInputConstantIndex(Name("m_indicesCount"));
                RHI::ShaderInputConstantIndex vertexCountHandle =
                    meshRenderData.ObjectSrg->FindShaderInputConstantIndex(Name("m_vertexCount"));
                if (indicesCountHandle.IsValid())
                {
                    meshRenderData.ObjectSrg->SetConstant(indicesCountHandle, meshRenderData.IndexCount);
                }
                if (vertexCountHandle.IsValid())
                {
                    meshRenderData.ObjectSrg->SetConstant(vertexCountHandle, meshRenderData.VertexCount);
                }
            }

            // Debug coloring: build the per-triangle cluster id buffer and bind it
            // (+ its count and the runtime toggle) to the per-object SRG. The
            // cluster descriptors were populated in PackInit before this runs.
            if (success)
            {
                const uint32_t triangleCount = meshRenderData.IndexCount / 3u;
                meshRenderData.PersistentTriangleCluster.assign(triangleCount, 0u);
                for (uint32_t c = 0; c < meshRenderData.PersistentClusterDescriptors.size(); ++c)
                {
                    const ClusterDescriptor& cd = meshRenderData.PersistentClusterDescriptors[c];
                    const uint32_t triEnd = AZStd::GetMin(cd.m_triangleOffset + cd.m_triangleCount, triangleCount);
                    for (uint32_t t = cd.m_triangleOffset; t < triEnd; ++t)
                    {
                        meshRenderData.PersistentTriangleCluster[t] = c;
                    }
                }

                if (triangleCount > 0)
                {
                    SrgBufferDescriptor tcDesc(
                        RPI::CommonBufferPoolType::ReadOnly,
                        RHI::Format::Unknown,   // StructuredBuffer<uint>
                        RHI::BufferBindFlags::ShaderRead,
                        sizeof(AZ::u32), triangleCount,
                        Name{ "MeshletsTriangleCluster" }, Name{ "m_triangleCluster" }, 6, 0,
                        reinterpret_cast<uint8_t*>(meshRenderData.PersistentTriangleCluster.data()));
                    meshRenderData.TriangleClusterBuffer =
                        UtilityClass::CreateBufferAndBindToSrg("Meshlets", tcDesc, meshRenderData.ObjectSrg);
                    if (!meshRenderData.TriangleClusterBuffer)
                    {
                        AZ_Warning("Meshlets", false,
                            "Failed to create/bind m_triangleCluster buffer -- meshlet debug coloring disabled.");
                    }
                }

                RHI::ShaderInputConstantIndex triClusterCountHandle =
                    meshRenderData.ObjectSrg->FindShaderInputConstantIndex(Name("m_triangleClusterCount"));
                if (triClusterCountHandle.IsValid())
                {
                    meshRenderData.ObjectSrg->SetConstant(
                        triClusterCountHandle,
                        meshRenderData.TriangleClusterBuffer ? triangleCount : 0u);
                }
                RHI::ShaderInputConstantIndex debugColorHandle =
                    meshRenderData.ObjectSrg->FindShaderInputConstantIndex(Name("m_meshletDebugColor"));
                if (debugColorHandle.IsValid())
                {
                    meshRenderData.ObjectSrg->SetConstant(debugColorHandle, 0u);
                }
            }

            // Compile the per-object SRG. Most state is static; the only mutable
            // member is m_meshletDebugColor, toggled via SetMeshletDebugColor (which
            // re-sets the constant and recompiles).
            if (success)
            {
                meshRenderData.ObjectSrg->Compile();
            }

            return success;
        }

        // For the Compute, since some of the buffers are RW the RHI will verify that
        // they are attached to the frame scheduler (ValidateSetBufferView) and this
        // might fail if the creation is not times correctly, hence they is a split
        // between the creation and the binding to the Srg.
        bool MeshletsRenderObject::CreateAndBindComputeSrgAndDispatch(
            Data::Instance<RPI::Shader> computeShader, MeshRenderData& meshRenderData)
        {
            // Start with the Srg creation - it will be required for the buffers generation
            if (meshRenderData.ComputeSrg)
            {
                return true;
            }

            meshRenderData.ComputeSrg =
                RPI::ShaderResourceGroup::Create(computeShader->GetAsset(), AZ::Name{ "MeshletsDataSrg" });
            if (!meshRenderData.ComputeSrg)
            {
                AZ_Error("Meshlets", false, "Failed to create the Compute Srg");
                return false;
            }

            uint32_t streamsNum = (uint32_t)meshRenderData.ComputeBuffersDescriptors.size();
            bool success = true;
            for (uint32_t stream = 0; stream < streamsNum ; ++stream)
            {
                SrgBufferDescriptor& bufferDesc = meshRenderData.ComputeBuffersDescriptors[stream];

                // SP1 fix: ALL compute-side buffers (including RW UVs/Indices) are
                // now dedicated RPI::Buffer instances, not SharedBuffer sub-views.
                // The m_indicesOffset / m_texCoordsOffset shader constants are set
                // to 0 since each buffer starts at element 0 of its own backing.
                const bool isUVs     = (stream == uint8_t(ComputeStreamsSemantics::UVs));
                const bool isIndices = (stream == uint8_t(ComputeStreamsSemantics::Indices));
                if (isUVs || isIndices)
                {
                    AZ::Name constantName = isUVs ?
                        Name("m_texCoordsOffset") : Name("m_indicesOffset");
                    RHI::ShaderInputConstantIndex constantHandle = meshRenderData.ComputeSrg->FindShaderInputConstantIndex(constantName);
                    // Buffer starts at element 0 of its own backing now.
                    if (!meshRenderData.ComputeSrg->SetConstant(constantHandle, 0u))
                    {
                        AZ_Error("Meshlets", false, "Failed to bind Constant [%s]", constantName.GetCStr());
                        return false;
                    }
                }

                if (!meshRenderData.ComputeBuffers[stream])
                {
                    AZ_Error("Meshlets", false, "Buffer doesn't exist");
                    success = false;
                    continue;
                }

                if (isUVs || isIndices)
                {
                    // SP1 fix: the compute SRG declares m_uvs and m_indices as
                    // ReadWrite (RWBuffer in HLSL -> UAV). Atom's RHI validates
                    // that any ReadWrite buffer view bound to an SRG must be a
                    // frame-graph attachment (for cross-pass hazard tracking).
                    //
                    // Our dedicated per-object RPI::Buffer instances are NOT
                    // currently registered as imported frame-graph attachments
                    // (that requires per-frame import + UseShaderAttachment in
                    // MultiDispatchComputePass::SetupFrameGraphDependencies,
                    // which is the proper follow-up; see ATOM-5668 TODO note in
                    // BufferViewDescriptor.h for the engine-wide context).
                    //
                    // Without setting m_ignoreFrameAttachmentValidation = true
                    // the SRG bind fails with:
                    //   "Buffer Input 'm_indices[0]' / 'm_uvs[0]': DeviceBuffer
                    //    is bound to a ReadWrite shader input, but it is not an
                    //    attachment on the frame scheduler."
                    // This flag is the same hack the legacy SharedBuffer
                    // sub-view path used in MeshletsUtilities.cpp:183. Using
                    // dedicated per-object buffers is what unblocks the AMD
                    // page-fault issue (SP1) -- the validation bypass here is a
                    // necessary stop-gap until proper imported-attachment
                    // wiring lands.
                    RHI::BufferViewDescriptor viewDesc;
                    viewDesc.m_elementOffset = 0;
                    viewDesc.m_elementCount  = bufferDesc.m_elementCount;
                    viewDesc.m_elementSize   = bufferDesc.m_elementSize;
                    viewDesc.m_elementFormat = bufferDesc.m_elementFormat;
                    viewDesc.m_overrideBindFlags = bufferDesc.m_bindFlags;
                    viewDesc.m_ignoreFrameAttachmentValidation = true;

                    auto bufferView =
                        meshRenderData.ComputeBuffers[stream]->GetRHIBuffer()->GetBufferView(viewDesc);
                    if (!bufferView)
                    {
                        AZ_Error("Meshlets", false,
                            "Failed to create RW buffer view for [%s]",
                            bufferDesc.m_bufferName.GetCStr());
                        success = false;
                        continue;
                    }

                    success &= UtilityClass::BindBufferViewToSrg(
                        "Meshlets", bufferView, bufferDesc, meshRenderData.ComputeSrg);
                }
                else
                {
                    // Read-only inputs (descriptors, triangles, indirection):
                    // standard SRG bind has no UAV validation requirement.
                    success &= UtilityClass::BindBufferToSrg(
                        "Meshlets", meshRenderData.ComputeBuffers[stream], bufferDesc,
                        meshRenderData.ComputeSrg);
                }
            }

            if (success)
            {
                // Compile the Srg and create the dispatch 
                meshRenderData.ComputeSrg->Compile();

                meshRenderData.MeshDispatchItem.InitDispatch(
                    computeShader.get(), meshRenderData.ComputeSrg, meshRenderData.MeshletsCount);
            }

            return success;
        }

        bool MeshletsRenderObject::EnsureMaterialSrg(
            uint32_t meshIndex, const Data::Instance<RPI::Shader>& forwardShader)
        {
            if (m_modelRenderData.empty() || meshIndex >= m_modelRenderData[0].size())
            {
                return false;
            }
            MeshRenderData* meshRenderData = m_modelRenderData[0][meshIndex];
            if (!meshRenderData)
            {
                return false;
            }
            if (meshRenderData->MaterialResolved)
            {
                return meshRenderData->MaterialSrg != nullptr;
            }
            if (!forwardShader)
            {
                return false; // Forward shader not loaded yet -- retry next frame.
            }

            // Resolve the source model's MaterialAsset for this mesh (LOD 0).
            Data::Asset<RPI::MaterialAsset> materialAsset;
            if (m_sourceModelAsset.IsReady())
            {
                auto lodAssets = m_sourceModelAsset->GetLodAssets();
                if (!lodAssets.empty() && lodAssets[0].IsReady())
                {
                    auto meshes = lodAssets[0]->GetMeshes();
                    if (meshIndex < meshes.size())
                    {
                        const RPI::ModelMaterialSlot::StableId slotId = meshes[meshIndex].GetMaterialSlotId();
                        const RPI::ModelMaterialSlot& slot = m_sourceModelAsset->FindMaterialSlot(slotId);
                        materialAsset = slot.m_defaultMaterialAsset;
                    }
                }
            }

            // If a material is assigned but not yet loaded, retry next frame rather
            // than block the render thread.
            if (materialAsset.GetId().IsValid() && !materialAsset.IsReady())
            {
                return false;
            }

            // Property defaults (used for no-material meshes or absent properties).
            AZ::Color baseColor = AZ::Color(0.5f, 0.5f, 0.5f, 1.0f);
            float metallicFactor = 0.0f;
            float roughnessFactor = 0.5f;
            float roughnessLowerBound = 0.0f;
            float roughnessUpperBound = 1.0f;
            float normalFactor = 1.0f;
            float diffuseAOFactor = 1.0f;
            float specularAOFactor = 1.0f;
            AZ::Vector3 emissiveColor(0.0f, 0.0f, 0.0f);
            float emissiveIntensity = 0.0f;
            Data::Instance<RPI::Image> baseColorMap, normalMap, metallicMap, roughnessMap;
            Data::Instance<RPI::Image> diffuseOcclusionMap, specularOcclusionMap, emissiveMap;

            if (materialAsset.GetId().IsValid())
            {
                Data::Instance<RPI::Material> material = RPI::Material::FindOrCreate(materialAsset);
                if (!material)
                {
                    return false; // Material shaders not ready -- retry next frame.
                }

                const auto getFloat = [&material](const char* name, float def) -> float
                {
                    const RPI::MaterialPropertyIndex idx = material->FindPropertyIndex(AZ::Name{ name });
                    return idx.IsValid() ? material->GetPropertyValue<float>(idx) : def;
                };
                const auto getColor = [&material](const char* name, const AZ::Color& def) -> AZ::Color
                {
                    const RPI::MaterialPropertyIndex idx = material->FindPropertyIndex(AZ::Name{ name });
                    return idx.IsValid() ? material->GetPropertyValue<AZ::Color>(idx) : def;
                };
                const auto getBool = [&material](const char* name, bool def) -> bool
                {
                    const RPI::MaterialPropertyIndex idx = material->FindPropertyIndex(AZ::Name{ name });
                    return idx.IsValid() ? material->GetPropertyValue<bool>(idx) : def;
                };
                const auto getImage = [&material](const char* name) -> Data::Instance<RPI::Image>
                {
                    const RPI::MaterialPropertyIndex idx = material->FindPropertyIndex(AZ::Name{ name });
                    return idx.IsValid() ? material->GetPropertyValue<Data::Instance<RPI::Image>>(idx) : nullptr;
                };

                baseColor = getColor("baseColor.color", baseColor);
                const float baseColorFactor = getFloat("baseColor.factor", 1.0f);
                baseColor.SetR(baseColor.GetR() * baseColorFactor);
                baseColor.SetG(baseColor.GetG() * baseColorFactor);
                baseColor.SetB(baseColor.GetB() * baseColorFactor);

                metallicFactor = getFloat("metallic.factor", 0.0f);
                roughnessFactor = getFloat("roughness.factor", 0.5f);
                // The bounds are shown in the editor only when the texture is
                // bound, but the property still exists on the material instance
                // -- pick them up so the texture remap matches StandardPBR.
                roughnessLowerBound = getFloat("roughness.lowerBound", 0.0f);
                roughnessUpperBound = getFloat("roughness.upperBound", 1.0f);
                normalFactor = getFloat("normal.factor", 1.0f);

                if (getBool("baseColor.useTexture", true)) { baseColorMap = getImage("baseColor.textureMap"); }
                if (getBool("normal.useTexture", true))    { normalMap = getImage("normal.textureMap"); }
                if (getBool("metallic.useTexture", true))  { metallicMap = getImage("metallic.textureMap"); }
                if (getBool("roughness.useTexture", true)) { roughnessMap = getImage("roughness.textureMap"); }

                // Occlusion (AO). pow() exponent gates strength -- default is 1.0
                // (identity); higher values darken the AO more aggressively. Without
                // AO the meshlet looks visibly brighter / more reflective than the
                // standard mesh because crevices get full ambient + IBL.
                diffuseAOFactor  = getFloat("occlusion.diffuseFactor", 1.0f);
                specularAOFactor = getFloat("occlusion.specularFactor", 1.0f);
                if (getBool("occlusion.diffuseUseTexture", true))  { diffuseOcclusionMap  = getImage("occlusion.diffuseTextureMap"); }
                if (getBool("occlusion.specularUseTexture", true)) { specularOcclusionMap = getImage("occlusion.specularTextureMap"); }

                // Emissive. Only contribute when emissive.enable is true (default
                // false) -- otherwise emissive bright spots would appear that the
                // standard shader correctly suppresses. The map alone doesn't
                // imply emissive is on; the material's enable flag rules.
                const bool emissiveEnable = getBool("emissive.enable", false);
                if (emissiveEnable)
                {
                    const AZ::Color ec = getColor("emissive.color", AZ::Color(0.0f, 0.0f, 0.0f, 1.0f));
                    emissiveColor.SetX(ec.GetR());
                    emissiveColor.SetY(ec.GetG());
                    emissiveColor.SetZ(ec.GetB());
                    emissiveIntensity = getFloat("emissive.intensity", 0.0f);
                    if (getBool("emissive.useTexture", true)) { emissiveMap = getImage("emissive.textureMap"); }
                }
            }

            // Create the per-material SRG from the forward shader's MeshletsMaterialSrg layout.
            Data::Instance<RPI::ShaderResourceGroup> materialSrg =
                RPI::ShaderResourceGroup::Create(forwardShader->GetAsset(), AZ::Name{ "MeshletsMaterialSrg" });
            if (!materialSrg)
            {
                AZ_Warning("Meshlets", false, "EnsureMaterialSrg: failed to create MeshletsMaterialSrg");
                return false;
            }

            // 1x1 white fallback so every texture descriptor is valid even when a map is absent.
            const Data::Instance<RPI::Image> whiteImage =
                RPI::ImageSystemInterface::Get()->GetSystemImage(RPI::SystemImage::White);

            uint32_t flags = 0;
            if (baseColorMap)         { flags |= (1u << 0); }
            if (normalMap)            { flags |= (1u << 1); }
            if (metallicMap)          { flags |= (1u << 2); }
            if (roughnessMap)         { flags |= (1u << 3); }
            if (diffuseOcclusionMap)  { flags |= (1u << 4); }
            if (specularOcclusionMap) { flags |= (1u << 5); }
            if (emissiveMap)          { flags |= (1u << 6); }

            const auto bindImage =
                [&materialSrg, &whiteImage](const char* slot, const Data::Instance<RPI::Image>& img)
            {
                const RHI::ShaderInputImageIndex idx = materialSrg->FindShaderInputImageIndex(AZ::Name{ slot });
                if (idx.IsValid())
                {
                    materialSrg->SetImage(idx, img ? img : whiteImage);
                }
            };
            bindImage("m_baseColorMap", baseColorMap);
            bindImage("m_normalMap", normalMap);
            bindImage("m_metallicMap", metallicMap);
            bindImage("m_roughnessMap", roughnessMap);
            bindImage("m_diffuseOcclusionMap", diffuseOcclusionMap);
            bindImage("m_specularOcclusionMap", specularOcclusionMap);
            bindImage("m_emissiveMap", emissiveMap);

            const auto setFloat = [&materialSrg](const char* slot, float v)
            {
                const RHI::ShaderInputConstantIndex idx = materialSrg->FindShaderInputConstantIndex(AZ::Name{ slot });
                if (idx.IsValid()) { materialSrg->SetConstant(idx, v); }
            };
            {
                const AZ::Vector4 bc(baseColor.GetR(), baseColor.GetG(), baseColor.GetB(), baseColor.GetA());
                const RHI::ShaderInputConstantIndex idx = materialSrg->FindShaderInputConstantIndex(AZ::Name{ "m_baseColor" });
                if (idx.IsValid()) { materialSrg->SetConstant(idx, bc); }
            }
            setFloat("m_metallicFactor", metallicFactor);
            setFloat("m_roughnessFactor", roughnessFactor);
            setFloat("m_roughnessLowerBound", roughnessLowerBound);
            setFloat("m_roughnessUpperBound", roughnessUpperBound);
            setFloat("m_normalFactor", normalFactor);
            setFloat("m_diffuseOcclusionFactor", diffuseAOFactor);
            setFloat("m_specularOcclusionFactor", specularAOFactor);
            setFloat("m_emissiveIntensity", emissiveIntensity);
            {
                const RHI::ShaderInputConstantIndex idx = materialSrg->FindShaderInputConstantIndex(AZ::Name{ "m_emissiveColor" });
                if (idx.IsValid()) { materialSrg->SetConstant(idx, emissiveColor); }
            }
            {
                const RHI::ShaderInputConstantIndex idx = materialSrg->FindShaderInputConstantIndex(AZ::Name{ "m_flags" });
                if (idx.IsValid()) { materialSrg->SetConstant(idx, flags); }
            }

            materialSrg->Compile();
            meshRenderData->MaterialSrg = materialSrg;
            meshRenderData->MaterialResolved = true;

            AZ_TracePrintf("Meshlets",
                "EnsureMaterialSrg: mesh %u -- material=%s, flags=0x%x, baseColor=(%.2f,%.2f,%.2f), "
                "metallic=%.2f, roughness=%.2f (texRange %.2f..%.2f), normalFactor=%.2f, "
                "aoFactors=(%.2f diff, %.2f spec), emissive=(%.2f,%.2f,%.2f)*%.2f\n",
                meshIndex, materialAsset.GetId().IsValid() ? "model" : "default", flags,
                static_cast<float>(baseColor.GetR()), static_cast<float>(baseColor.GetG()),
                static_cast<float>(baseColor.GetB()), metallicFactor, roughnessFactor,
                roughnessLowerBound, roughnessUpperBound, normalFactor,
                diffuseAOFactor, specularAOFactor,
                static_cast<float>(emissiveColor.GetX()), static_cast<float>(emissiveColor.GetY()),
                static_cast<float>(emissiveColor.GetZ()), emissiveIntensity);

            return true;
        }

        bool MeshletsRenderObject::EnsureIndirectArgs(
            MeshRenderData& meshRenderData, const RHI::IndirectBufferSignature* signature)
        {
            if (meshRenderData.IndirectReady)
            {
                return true;
            }
            if (!signature)
            {
                return false;  // Signature not created yet -- retry later.
            }

            // PERF -- vertex-cache reuse. The hardware index buffer is the mesh's EXPANDED
            // vertex-index stream (one mesh-vertex index per triangle corner), NOT a [0..N)
            // identity. With the real indices, SV_VertexID == the mesh vertex index, so the
            // post-transform vertex cache reuses shaded vertices across the (many) triangles
            // that share a vertex -- and meshlet data is ordered for exactly this locality.
            // The vertex shader then uses SV_VertexID directly (GetMeshVertexIndex), dropping
            // the per-corner m_indices[] StructuredBuffer load. The old identity buffer made
            // every corner a unique SV_VertexID, defeating the cache -> 3-6x redundant vertex
            // shading across depth/motion/forward/shadow. This benefits the whole-mesh draw
            // AND the CPU per-cluster draws (which slice the same buffer via StartIndexLocation).
            // Phase 4 VRAM reclaim: no index/IA buffers for streaming-exclusive meshes.
            // Callers already treat "not ready" as "skip this path" (lazy-load guards),
            // which is exactly the intended behavior here -- permanently.
            if (meshRenderData.MonolithicDropped)
            {
                return false;
            }
            if (meshRenderData.IndexCount == 0 ||
                meshRenderData.PersistentExpandedIndices.size() < meshRenderData.IndexCount)
            {
                AZ_Warning("Meshlets", false,
                    "EnsureIndirectArgs: expanded index data missing (have %zu, need %u).",
                    meshRenderData.PersistentExpandedIndices.size(), meshRenderData.IndexCount);
                return false;
            }
            {
                // FULL expanded slab: DAG packs keep interior index slices after the
                // leaves; whole-mesh draw counts stay IndexCount (leaf-only).
                const AZ::u32 slabIndexCount =
                    static_cast<AZ::u32>(meshRenderData.PersistentExpandedIndices.size());
                SrgBufferDescriptor ibDesc(
                    RPI::CommonBufferPoolType::StaticInputAssembly,
                    RHI::Format::R32_UINT,
                    RHI::BufferBindFlags::InputAssembly,
                    sizeof(AZ::u32),
                    slabIndexCount,
                    Name{ "MeshletsIndexBuffer" }, Name{ "" }, 0, 0,
                    reinterpret_cast<uint8_t*>(meshRenderData.PersistentExpandedIndices.data()));
                meshRenderData.IndexBuffer = UtilityClass::CreateBuffer("Meshlets", ibDesc, nullptr);
                if (!meshRenderData.IndexBuffer || !meshRenderData.IndexBuffer->GetRHIBuffer())
                {
                    AZ_Warning("Meshlets", false, "EnsureIndirectArgs: failed to create the index buffer.");
                    return false;
                }
                meshRenderData.IndexBufferViewRHI = RHI::IndexBufferView(
                    *meshRenderData.IndexBuffer->GetRHIBuffer(),
                    0,
                    slabIndexCount * sizeof(AZ::u32),
                    RHI::IndexFormat::Uint32);
            }
            meshRenderData.IndirectGeometryView.SetIndexBufferView(meshRenderData.IndexBufferViewRHI);

            // PERF (hardware input-assembly, POSITION): a dedicated InputAssembly vertex
            // buffer (R32G32B32_FLOAT, 12-byte stride) holding the same data as the
            // m_positions StructuredBuffer. The depth/shadow/motion passes fetch position
            // through the hardware vertex fetcher instead of 3 scalar SRV loads/vertex. Kept
            // SEPARATE from the SRV stream (AMD hangs if InputAssembly+ShaderRead share one
            // buffer -- SharedBuffer.cpp). On failure (e.g. the pool rejects a 12-byte typed
            // vertex stride) PositionStreamValid stays false and the IA passes self-skip
            // (ValidateStreamBufferViews) rather than hang.
            meshRenderData.PositionStreamValid = false;
            {
                const uint8_t posSem = static_cast<uint8_t>(RenderStreamsSemantics::Positions);
                if (posSem < meshRenderData.RenderBuffersDescriptors.size() &&
                    meshRenderData.RenderBuffersDescriptors[posSem].m_bufferData &&
                    meshRenderData.VertexCount > 0)
                {
                    const uint32_t posStride = static_cast<uint32_t>(sizeof(float) * 3);
                    SrgBufferDescriptor posIaDesc(
                        RPI::CommonBufferPoolType::StaticInputAssembly,
                        RHI::Format::R32G32B32_FLOAT,
                        RHI::BufferBindFlags::InputAssembly,
                        posStride, meshRenderData.VertexCount,
                        Name{ "MeshletsPositionIA" }, Name{ "" }, 0, 0,
                        meshRenderData.RenderBuffersDescriptors[posSem].m_bufferData);
                    meshRenderData.PositionIaBuffer = UtilityClass::CreateBuffer("Meshlets", posIaDesc, nullptr);
                    if (meshRenderData.PositionIaBuffer && meshRenderData.PositionIaBuffer->GetRHIBuffer())
                    {
                        meshRenderData.PositionStreamView = RHI::StreamBufferView(
                            *meshRenderData.PositionIaBuffer->GetRHIBuffer(),
                            0, meshRenderData.VertexCount * posStride, posStride);
                        meshRenderData.IndirectGeometryView.AddStreamBufferView(meshRenderData.PositionStreamView);
                        meshRenderData.PositionStreamValid = true;
                    }
                    else
                    {
                        AZ_Warning("Meshlets", false,
                            "EnsureIndirectArgs: POSITION IA buffer creation failed (R32G32B32_FLOAT/12B stride may "
                            "be rejected by the StaticInputAssembly pool). Depth/shadow/motion IA path disabled for "
                            "this mesh; rendering falls back safely.");
                    }
                }
            }

            // PERF (hardware input-assembly, FORWARD): four MORE dedicated InputAssembly
            // vertex buffers (NORMAL R32G32B32_FLOAT/12B, TANGENT R32G32B32A32_FLOAT/16B,
            // BITANGENT R32G32B32_FLOAT/12B, UV R32G32_FLOAT/8B), each created exactly like
            // PositionIaBuffer (StaticInputAssembly pool, InputAssembly bind flag ONLY,
            // NEVER combined with ShaderRead -- AMD DEVICE_HUNG). The source bytes are the
            // SAME tightly-packed pack data the StructuredBuffer SRVs read (m_bufferData),
            // verified to use exactly these strides in MeshletPackBuilderCore. Added to
            // IndirectGeometryView in the EXACT order POSITION,NORMAL,TANGENT,BITANGENT,UV so
            // the view's streams are [0]=POSITION,[1]=NORMAL,[2]=TANGENT,[3]=BITANGENT,[4]=UV,
            // matching m_forwardInputLayout's channel order. ForwardStreamsValid is set true
            // ONLY if POSITION + all four allocated; on any failure we add NO partial streams
            // (that would desync the stream indices) and leave the forward IA path off -- the
            // forward DrawItem then self-skips rather than binding a partial layout (hang).
            meshRenderData.ForwardStreamsValid = false;
            if (meshRenderData.PositionStreamValid && meshRenderData.VertexCount > 0)
            {
                struct ForwardIaStream
                {
                    RenderStreamsSemantics m_sem;
                    RHI::Format            m_format;
                    uint32_t               m_stride;
                    const char*            m_name;
                    Data::Instance<RPI::Buffer>* m_buffer;
                    RHI::StreamBufferView*       m_view;
                };
                ForwardIaStream streams[] = {
                    { RenderStreamsSemantics::Normals,    RHI::Format::R32G32B32_FLOAT,    static_cast<uint32_t>(sizeof(float) * 3),
                      "MeshletsNormalIA",    &meshRenderData.NormalIaBuffer,    &meshRenderData.NormalStreamView },
                    { RenderStreamsSemantics::Tangents,   RHI::Format::R32G32B32A32_FLOAT, static_cast<uint32_t>(sizeof(float) * 4),
                      "MeshletsTangentIA",   &meshRenderData.TangentIaBuffer,   &meshRenderData.TangentStreamView },
                    { RenderStreamsSemantics::BiTangents, RHI::Format::R32G32B32_FLOAT,    static_cast<uint32_t>(sizeof(float) * 3),
                      "MeshletsBitangentIA", &meshRenderData.BitangentIaBuffer, &meshRenderData.BitangentStreamView },
                    { RenderStreamsSemantics::UVs,        RHI::Format::R32G32_FLOAT,       static_cast<uint32_t>(sizeof(float) * 2),
                      "MeshletsUvIA",        &meshRenderData.UvIaBuffer,        &meshRenderData.UvStreamView },
                };

                bool allOk = true;
                for (ForwardIaStream& s : streams)
                {
                    const uint8_t sem = static_cast<uint8_t>(s.m_sem);
                    if (sem >= meshRenderData.RenderBuffersDescriptors.size() ||
                        !meshRenderData.RenderBuffersDescriptors[sem].m_bufferData)
                    {
                        allOk = false;
                        break;
                    }
                    SrgBufferDescriptor iaDesc(
                        RPI::CommonBufferPoolType::StaticInputAssembly,
                        s.m_format,
                        RHI::BufferBindFlags::InputAssembly,
                        s.m_stride, meshRenderData.VertexCount,
                        Name{ s.m_name }, Name{ "" }, 0, 0,
                        meshRenderData.RenderBuffersDescriptors[sem].m_bufferData);
                    *s.m_buffer = UtilityClass::CreateBuffer("Meshlets", iaDesc, nullptr);
                    if (!(*s.m_buffer) || !(*s.m_buffer)->GetRHIBuffer())
                    {
                        allOk = false;
                        break;
                    }
                    *s.m_view = RHI::StreamBufferView(
                        *(*s.m_buffer)->GetRHIBuffer(),
                        0, meshRenderData.VertexCount * s.m_stride, s.m_stride);
                }

                if (allOk)
                {
                    // Add in EXACT order after POSITION: NORMAL, TANGENT, BITANGENT, UV.
                    meshRenderData.IndirectGeometryView.AddStreamBufferView(meshRenderData.NormalStreamView);
                    meshRenderData.IndirectGeometryView.AddStreamBufferView(meshRenderData.TangentStreamView);
                    meshRenderData.IndirectGeometryView.AddStreamBufferView(meshRenderData.BitangentStreamView);
                    meshRenderData.IndirectGeometryView.AddStreamBufferView(meshRenderData.UvStreamView);
                    meshRenderData.ForwardStreamsValid = true;
                }
                else
                {
                    AZ_Warning("Meshlets", false,
                        "EnsureIndirectArgs: forward IA buffer creation failed (NORMAL/TANGENT/BITANGENT/UV). "
                        "Forward hardware-IA path disabled for this mesh; the forward DrawItem self-skips and "
                        "rendering falls back safely (depth/shadow/motion still use the POSITION-only IA path).");
                }
            }

            // 2) Indirect args: one DrawIndexedIndirectCommand for the whole mesh (C1).
            //    {indexCount, instanceCount, startIndex, baseVertex, startInstance}
            //    (C2's CPU cull will emit only visible clusters' commands.)
            meshRenderData.IndirectArgsData = { meshRenderData.IndexCount, 1u, 0u, 0u, 0u };
            const uint32_t commandCount = 1;
            const uint32_t elementCount = static_cast<uint32_t>(meshRenderData.IndirectArgsData.size());

            SrgBufferDescriptor argsDesc(
                RPI::CommonBufferPoolType::Indirect,
                RHI::Format::R32_UINT,
                RHI::BufferBindFlags::Indirect,
                sizeof(AZ::u32), elementCount,
                Name{ "MeshletsIndirectArgs" }, Name{ "" }, 0, 0,
                reinterpret_cast<uint8_t*>(meshRenderData.IndirectArgsData.data()));
            meshRenderData.IndirectArgsBuffer = UtilityClass::CreateBuffer("Meshlets", argsDesc, nullptr);
            if (!meshRenderData.IndirectArgsBuffer || !meshRenderData.IndirectArgsBuffer->GetRHIBuffer())
            {
                AZ_Warning("Meshlets", false, "EnsureIndirectArgs: failed to create the indirect args buffer.");
                return false;
            }

            meshRenderData.IndirectArgsView = RHI::IndirectBufferView(
                *meshRenderData.IndirectArgsBuffer->GetRHIBuffer(),
                *signature,
                0,
                elementCount * sizeof(AZ::u32),
                signature->GetByteStride());

            RHI::DrawIndirect indirectArgs(commandCount, meshRenderData.IndirectArgsView, 0);
            meshRenderData.IndirectGeometryView.SetDrawArguments(RHI::DrawArguments(indirectArgs));
            meshRenderData.IndirectReady = true;

            AZ_TracePrintf("Meshlets",
                "EnsureIndirectArgs: OK -- indexed indirect, indexCount=%u\n", meshRenderData.IndexCount);
            return true;
        }

        void MeshletsRenderObject::EnsureMeshBounds(MeshRenderData& meshRenderData)
        {
            if (meshRenderData.MeshBoundsRadius >= 0.0f)
            {
                return;   // already computed (per-mesh, shared across instances)
            }
            const AZStd::vector<ClusterBoundsRecord>& bounds = meshRenderData.PersistentClusterBounds;
            if (bounds.empty())
            {
                return;   // leave radius < 0 (invalid); caller falls back to always-cull
            }
            // Conservative whole-mesh sphere: AABB of all cluster spheres, centre at the
            // AABB centre, radius = farthest (clusterCentre distance + clusterRadius).
            AZ::Aabb aabb = AZ::Aabb::CreateNull();
            for (const ClusterBoundsRecord& b : bounds)
            {
                const AZ::Vector3 c(b.m_center[0], b.m_center[1], b.m_center[2]);
                const AZ::Vector3 r(b.m_radius);
                aabb.AddPoint(c - r);
                aabb.AddPoint(c + r);
            }
            const AZ::Vector3 center = aabb.GetCenter();
            float radius = 0.0f;
            for (const ClusterBoundsRecord& b : bounds)
            {
                const AZ::Vector3 c(b.m_center[0], b.m_center[1], b.m_center[2]);
                radius = AZStd::GetMax(radius, center.GetDistance(c) + b.m_radius);
            }
            meshRenderData.MeshBoundsCenter = center;
            meshRenderData.MeshBoundsRadius = radius;
        }

        bool MeshletsRenderObject::EnsureCullGpuBuffers(MeshRenderData& meshRenderData)
        {
            if (meshRenderData.CullGpuBuffersReady)
            {
                return true;
            }
            const AZStd::vector<ClusterBoundsRecord>& bounds = meshRenderData.PersistentClusterBounds;
            const AZStd::vector<ClusterDescriptor>& descs = meshRenderData.PersistentClusterDescriptors;
            if (bounds.empty() || descs.empty())
            {
                AZ_Warning("Meshlets", false,
                    "EnsureCullGpuBuffers: missing cluster bounds/descriptors -- GPU cull data unavailable.");
                return false;
            }

            // Per-cluster bounds (sphere + cone), 48 bytes each -> StructuredBuffer<ClusterBoundsRecord>.
            {
                SrgBufferDescriptor boundsDesc(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown,   // structured
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(ClusterBoundsRecord), static_cast<uint32_t>(bounds.size()),
                    Name{ "MeshletsClusterBounds" }, Name{ "m_clusterBounds" }, 0, 0,
                    reinterpret_cast<uint8_t*>(const_cast<ClusterBoundsRecord*>(bounds.data())));
                meshRenderData.ClusterBoundsBuffer = UtilityClass::CreateBuffer("Meshlets", boundsDesc, nullptr);
            }

            // Per-cluster descriptors (triangle offset/count), 16 bytes each -> StructuredBuffer<ClusterDescriptor>.
            {
                SrgBufferDescriptor descDesc(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown,   // structured
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(ClusterDescriptor), static_cast<uint32_t>(descs.size()),
                    Name{ "MeshletsClusterDescriptors" }, Name{ "m_clusterDescriptors" }, 1, 0,
                    reinterpret_cast<uint8_t*>(const_cast<ClusterDescriptor*>(descs.data())));
                meshRenderData.ClusterDescBuffer = UtilityClass::CreateBuffer("Meshlets", descDesc, nullptr);
            }

            if (!meshRenderData.ClusterBoundsBuffer || !meshRenderData.ClusterDescBuffer)
            {
                AZ_Warning("Meshlets", false, "EnsureCullGpuBuffers: failed to create GPU cluster buffers.");
                return false;
            }

            // Phase 6 cluster DAG (pack v3): per-cluster cut records, 48 bytes each ->
            // StructuredBuffer<DagNodeRecord>. Absent (null) for v2 packs -- the DAG cut
            // stays disabled and nothing binds it.
            if (meshRenderData.DagClusterCount > 0 && !meshRenderData.PersistentDagNodes.empty())
            {
                SrgBufferDescriptor dagDesc(
                    RPI::CommonBufferPoolType::ReadOnly,
                    RHI::Format::Unknown,   // structured
                    RHI::BufferBindFlags::ShaderRead,
                    sizeof(DagNodeRecord), static_cast<uint32_t>(meshRenderData.PersistentDagNodes.size()),
                    Name{ "MeshletsDagNodes" }, Name{ "m_dagNodes" }, 0, 0,
                    reinterpret_cast<uint8_t*>(const_cast<DagNodeRecord*>(meshRenderData.PersistentDagNodes.data())));
                meshRenderData.DagNodesBuffer = UtilityClass::CreateBuffer("Meshlets", dagDesc, nullptr);
                AZ_Warning("Meshlets", meshRenderData.DagNodesBuffer != nullptr,
                    "EnsureCullGpuBuffers: failed to create the DAG-node buffer; DAG LOD unavailable for this mesh.");
            }

            EnsureMeshBounds(meshRenderData);   // two-level cull whole-mesh bounding sphere
            meshRenderData.CullGpuBuffersReady = true;
            AZ_TracePrintf("Meshlets",
                "EnsureCullGpuBuffers: OK -- %zu clusters (bounds + descriptors) on GPU.\n", bounds.size());
            return true;
        }

        uint32_t MeshletsRenderObject::CullClustersToCommands(
            uint32_t meshIndex,
            const AZ::Frustum& frustum,
            const AZ::Vector3& cameraPos,
            const AZ::Matrix4x4& objectToWorld,
            bool doFrustumCull, bool doConeCull,
            AZStd::vector<AZ::u32>& outCommands,
            uint32_t& outCulled) const
        {
            outCommands.clear();
            outCulled = 0;
            if (m_modelRenderData.empty() || meshIndex >= m_modelRenderData[0].size())
            {
                return 0;
            }
            const MeshRenderData* mrd = m_modelRenderData[0][meshIndex];
            if (!mrd)
            {
                return 0;
            }

            const AZStd::vector<ClusterDescriptor>& clusters = mrd->PersistentClusterDescriptors;
            const AZStd::vector<ClusterBoundsRecord>& bounds = mrd->PersistentClusterBounds;
            // LEAF clusters only: for Phase 6 DAG packs the persistent arrays also hold
            // interior DAG clusters (after the leaves) -- the CPU cull path is not
            // DAG-aware and drawing interiors on top of leaves would double-render.
            const size_t clusterCount = AZStd::GetMin(clusters.size(), size_t(mrd->MeshletsCount));
            const bool haveBounds = (!bounds.empty() && bounds.size() >= clusterCount);

            // Max basis length -> conservative world-space radius scale (handles non-uniform scale).
            const float scaleX = objectToWorld.GetColumnAsVector3(0).GetLength();
            const float scaleY = objectToWorld.GetColumnAsVector3(1).GetLength();
            const float scaleZ = objectToWorld.GetColumnAsVector3(2).GetLength();
            const float maxScale = AZStd::GetMax(scaleX, AZStd::GetMax(scaleY, scaleZ));

            outCommands.reserve(clusterCount * 5);
            uint32_t visible = 0;
            for (size_t c = 0; c < clusterCount; ++c)
            {
                bool isVisible = true;
                if (haveBounds && (doFrustumCull || doConeCull))
                {
                    const ClusterBoundsRecord& b = bounds[c];

                    // Frustum cull against the bounding sphere.
                    if (doFrustumCull)
                    {
                        const AZ::Vector3 centerW =
                            (objectToWorld * AZ::Vector4(b.m_center[0], b.m_center[1], b.m_center[2], 1.0f)).GetAsVector3();
                        const float radiusW = b.m_radius * maxScale;
                        if (frustum.IntersectSphere(centerW, radiusW) == AZ::IntersectResult::Exterior)
                        {
                            isVisible = false;
                        }
                    }

                    // Backface normal-cone cull (meshopt convention): the cluster is
                    // entirely back-facing if dot(normalize(apex - eye), axis) >= cutoff.
                    // cutoff >= 1 means meshopt produced no valid cone -> never cull.
                    if (isVisible && doConeCull && b.m_coneCutoff < 1.0f)
                    {
                        const AZ::Vector3 apexW =
                            (objectToWorld * AZ::Vector4(b.m_coneApex[0], b.m_coneApex[1], b.m_coneApex[2], 1.0f)).GetAsVector3();
                        AZ::Vector3 axisW =
                            objectToWorld.Multiply3x3(AZ::Vector3(b.m_coneAxis[0], b.m_coneAxis[1], b.m_coneAxis[2]));
                        AZ::Vector3 toApex = apexW - cameraPos;
                        if (!axisW.IsZero() && !toApex.IsZero())
                        {
                            axisW.Normalize();
                            toApex.Normalize();
                            if (toApex.Dot(axisW) >= b.m_coneCutoff)
                            {
                                isVisible = false;
                            }
                        }
                    }
                }

                if (isVisible)
                {
                    const ClusterDescriptor& cd = clusters[c];
                    outCommands.push_back(cd.m_triangleCount * 3);    // indexCountPerInstance
                    outCommands.push_back(1u);                        // instanceCount
                    outCommands.push_back(cd.m_triangleOffset * 3);   // startIndexLocation
                    outCommands.push_back(0u);                        // baseVertexLocation
                    outCommands.push_back(0u);                        // startInstanceLocation
                    ++visible;
                }
                else
                {
                    ++outCulled;
                }
            }
            return visible;
        }

        void MeshletsRenderObject::SetMeshletDebugColor(bool enabled)
        {
            const uint32_t value = enabled ? 1u : 0u;
            for (ModelLodDataArray& lod : m_modelRenderData)
            {
                for (MeshRenderData* mrd : lod)
                {
                    if (!mrd || !mrd->ObjectSrg)
                    {
                        continue;
                    }
                    RHI::ShaderInputConstantIndex handle =
                        mrd->ObjectSrg->FindShaderInputConstantIndex(Name("m_meshletDebugColor"));
                    if (handle.IsValid())
                    {
                        mrd->ObjectSrg->SetConstant(handle, value);
                        mrd->ObjectSrg->Compile();
                    }
                }
            }
        }

        MeshletsRenderObject::~MeshletsRenderObject()
        {
            for (auto modelLodDataArray : m_modelRenderData)
            {
                for (auto lodData : modelLodDataArray)
                {
                    delete lodData;
                }
            }
        }

    } // namespace Meshlets
} // namespace AZ
