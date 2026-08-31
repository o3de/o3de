/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 */

#include <Builders/JsonSidecarBuilder.h>
#include <Builders/JsonSidecarDescriptor.h>
#include <Builders/MeshletPackBuilderCore.h>
#include <Builders/SourceMeshSet.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/Asset/AssetManager.h>

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>
#include <Atom/RHI.Reflect/Format.h>

// MeshletPackAsset is the runtime type the catalog must record for products
// of this builder — used as the JobProduct's m_assetType. PackResolver filters
// catalog assets by this exact type id when populating its model→pack map.
#include <Meshlets/Reflect/MeshletPackAsset.h>

namespace AZ::Meshlets::Builders
{
    void JsonSidecarBuilder::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest& request,
        AssetBuilderSDK::CreateJobsResponse& response)
    {
        for (const auto& platformInfo : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor jd;
            jd.m_jobKey = JobKey;
            jd.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
            jd.m_critical = false;
            response.m_createJobOutputs.push_back(jd);
        }
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void JsonSidecarBuilder::ProcessJob(
        const AssetBuilderSDK::ProcessJobRequest& request,
        AssetBuilderSDK::ProcessJobResponse& response)
    {
        if (m_cancelled.load())
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        // Step 1: read JSON.
        auto loadResult = AZ::JsonSerializationUtils::ReadJsonFile(request.m_fullPath);
        if (!loadResult.IsSuccess())
        {
            AZ_Error("Meshlets.JsonBuilder", false, "Failed to read JSON: %s",
                     loadResult.GetError().c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }
        JsonSidecarDescriptor desc;
        auto deserializeResult = AZ::JsonSerialization::Load(desc, loadResult.GetValue());
        if (deserializeResult.GetProcessing() != AZ::JsonSerializationResult::Processing::Completed)
        {
            AZ_Error("Meshlets.JsonBuilder", false, "Failed to deserialize JSON sidecar");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // The descriptor stores the source AssetId as the textual "{uuid}:subid"
        // form because AZ::Data::AssetId is not a supported JSON deserialization
        // target (the JSON serializer warns and skips it). Parse the string
        // here via AssetId::CreateString — same routine the catalog uses.
        const AZ::Data::AssetId sourceModelAssetId =
            AZ::Data::AssetId::CreateString(desc.m_sourceModelAssetIdStr);
        if (!sourceModelAssetId.IsValid())
        {
            AZ_Error("Meshlets.JsonBuilder", false,
                "Sidecar source_model_asset_id %s is not a valid AssetId — "
                "expected '{uuid}:subid' (e.g. '{7953FDEE-467C-544C-8512-DD559B57525A}:10b47c3d').",
                desc.m_sourceModelAssetIdStr.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // Step 2: load the referenced .azmodel synchronously.
        // (Per spec §5.3 — blocking load, normal Asset::AssetManager::GetAsset.)
        // Build SourceMeshSet by walking EVERY source LOD the model ships —
        // mirrors what MeshletsRenderObject::CreateMeshletsFromModelAsset did
        // today (which only read LOD0), but runs offline once and bakes the
        // full LOD chain into the pack. Meshes are matched across LODs by
        // index (Atom keeps mesh order consistent across a model's LODs).

        SourceMeshSet src;
        src.m_sourceModelAssetId       = sourceModelAssetId;
        src.m_maxVerticesPerCluster    = desc.m_maxVerticesPerCluster;
        src.m_maxTrianglesPerCluster   = desc.m_maxTrianglesPerCluster;
        src.m_coneWeight               = desc.m_coneWeight;
        src.m_generateClusterDag       = desc.m_generateClusterDag || desc.m_generatePages;
        src.m_generatePages            = desc.m_generatePages;

        // Load the .azmodel asset synchronously.
        auto modelAsset = AZ::Data::AssetManager::Instance().GetAsset<RPI::ModelAsset>(
            sourceModelAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
        modelAsset.BlockUntilLoadComplete();
        if (!modelAsset.IsReady())
        {
            AZ_Error("Meshlets.JsonBuilder", false,
                     "Failed to load ModelAsset %s",
                     sourceModelAssetId.ToString<AZStd::string>().c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // Extract meshes from ALL source LODs.
        const auto& lodAssets = modelAsset->GetLodAssets();
        if (lodAssets.empty())
        {
            AZ_Error("Meshlets.JsonBuilder", false,
                     "ModelAsset has no LOD assets");
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // Diagnostic: how many source LODs does this model ship? This tells us
        // whether the bake path (use the model's own LODs) or the generation
        // fallback (synthesize coarser LODs from LOD0 via meshopt_simplify in
        // MeshletPackBuilderCore) is the active source of coarse levels.
        AZ_TracePrintf("Meshlets.JsonBuilder",
            "Meshlets LOD bake: model has %zu source LODs\n", lodAssets.size());

        // Returns false (and sets the response failed) on a missing/short stream.
        auto extractMesh = [&](const RPI::ModelLodAsset::Mesh& meshAsset,
                               size_t lodIdx, size_t meshIdx, SourceMesh& srcMesh) -> bool
        {
            srcMesh.m_name = AZStd::string::format("LOD%zu_Mesh_%zu", lodIdx, meshIdx);

            const AZ::u32 vertexCount = meshAsset.GetVertexCount();
            const AZ::u32 indexCount = meshAsset.GetIndexCount();

            // Extract POSITION stream (3 floats per vertex).
            {
                const RPI::BufferAssetView* view = meshAsset.GetSemanticBufferAssetView(AZ::Name("POSITION"));
                if (!view)
                {
                    AZ_Error("Meshlets.JsonBuilder", false, "Missing POSITION stream in LOD %zu mesh %zu", lodIdx, meshIdx);
                    return false;
                }
                const auto& bufferAsset = view->GetBufferAsset();
                AZStd::span<const uint8_t> bytes = bufferAsset->GetBuffer();
                srcMesh.m_positions.resize(vertexCount * 3);
                memcpy(srcMesh.m_positions.data(), bytes.data(), vertexCount * 3 * sizeof(float));
            }

            // Extract NORMAL stream (3 floats per vertex).
            {
                const RPI::BufferAssetView* view = meshAsset.GetSemanticBufferAssetView(AZ::Name("NORMAL"));
                if (!view)
                {
                    AZ_Error("Meshlets.JsonBuilder", false, "Missing NORMAL stream in LOD %zu mesh %zu", lodIdx, meshIdx);
                    return false;
                }
                const auto& bufferAsset = view->GetBufferAsset();
                AZStd::span<const uint8_t> bytes = bufferAsset->GetBuffer();
                srcMesh.m_normals.resize(vertexCount * 3);
                memcpy(srcMesh.m_normals.data(), bytes.data(), vertexCount * 3 * sizeof(float));
            }

            // Extract TANGENT stream (4 floats per vertex).
            {
                const RPI::BufferAssetView* view = meshAsset.GetSemanticBufferAssetView(AZ::Name("TANGENT"));
                if (!view)
                {
                    AZ_Error("Meshlets.JsonBuilder", false, "Missing TANGENT stream in LOD %zu mesh %zu", lodIdx, meshIdx);
                    return false;
                }
                const auto& bufferAsset = view->GetBufferAsset();
                AZStd::span<const uint8_t> bytes = bufferAsset->GetBuffer();
                srcMesh.m_tangents.resize(vertexCount * 4);
                memcpy(srcMesh.m_tangents.data(), bytes.data(), vertexCount * 4 * sizeof(float));
            }

            // Extract BITANGENT stream (3 floats per vertex).
            {
                const RPI::BufferAssetView* view = meshAsset.GetSemanticBufferAssetView(AZ::Name("BITANGENT"));
                if (!view)
                {
                    AZ_Error("Meshlets.JsonBuilder", false, "Missing BITANGENT stream in LOD %zu mesh %zu", lodIdx, meshIdx);
                    return false;
                }
                const auto& bufferAsset = view->GetBufferAsset();
                AZStd::span<const uint8_t> bytes = bufferAsset->GetBuffer();
                srcMesh.m_bitangents.resize(vertexCount * 3);
                memcpy(srcMesh.m_bitangents.data(), bytes.data(), vertexCount * 3 * sizeof(float));
            }

            // Extract UV0 stream (2 floats per vertex).
            {
                const RPI::BufferAssetView* view = meshAsset.GetSemanticBufferAssetView(AZ::Name("UV"));
                if (!view)
                {
                    AZ_Error("Meshlets.JsonBuilder", false, "Missing UV stream in LOD %zu mesh %zu", lodIdx, meshIdx);
                    return false;
                }
                const auto& bufferAsset = view->GetBufferAsset();
                AZStd::span<const uint8_t> bytes = bufferAsset->GetBuffer();
                srcMesh.m_uv0.resize(vertexCount * 2);
                memcpy(srcMesh.m_uv0.data(), bytes.data(), vertexCount * 2 * sizeof(float));
            }

            // Extract index buffer, widening R16_UINT to R32_UINT if needed.
            {
                const RPI::BufferAssetView& indexView = meshAsset.GetIndexBufferAssetView();
                const auto& bufferAsset = indexView.GetBufferAsset();
                const auto& idxDesc = indexView.GetBufferViewDescriptor();
                AZStd::span<const uint8_t> bytes = bufferAsset->GetBuffer();

                srcMesh.m_indices.resize(indexCount);
                if (idxDesc.m_elementFormat == RHI::Format::R16_UINT)
                {
                    // Widen 16-bit indices to 32-bit.
                    const AZ::u16* src16 = reinterpret_cast<const AZ::u16*>(bytes.data());
                    for (AZ::u32 i = 0; i < indexCount; ++i)
                    {
                        srcMesh.m_indices[i] = static_cast<AZ::u32>(src16[i]);
                    }
                }
                else
                {
                    // Assume R32_UINT; copy directly.
                    memcpy(srcMesh.m_indices.data(), bytes.data(), indexCount * sizeof(AZ::u32));
                }
            }
            return true;
        };

        // The logical mesh count is defined by LOD0 (the finest level). Higher
        // LODs are matched to the same logical mesh by index; Atom keeps mesh
        // order consistent across LODs. If a higher LOD has fewer meshes than
        // LOD0, the missing logical meshes simply get a short m_lods chain and
        // the builder generates their coarser levels from LOD0.
        const auto& lod0Meshes = lodAssets[0]->GetMeshes();
        src.m_meshes.resize(lod0Meshes.size());
        for (size_t meshIdx = 0; meshIdx < lod0Meshes.size(); ++meshIdx)
        {
            src.m_meshes[meshIdx].m_name = AZStd::string::format("Mesh_%zu", meshIdx);
        }

        for (size_t lodIdx = 0; lodIdx < lodAssets.size(); ++lodIdx)
        {
            const auto& lodMeshes = lodAssets[lodIdx]->GetMeshes();
            for (size_t meshIdx = 0; meshIdx < lodMeshes.size(); ++meshIdx)
            {
                // Only fill logical meshes that exist at LOD0 (the canonical set).
                if (meshIdx >= src.m_meshes.size())
                {
                    break;
                }
                SourceMesh srcMesh;
                if (!extractMesh(lodMeshes[meshIdx], lodIdx, meshIdx, srcMesh))
                {
                    response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
                    return;
                }
                // Match by index: this LOD's mesh meshIdx is level lodIdx of the
                // logical mesh meshIdx. Levels are stored in source-LOD order;
                // m_lods[0] is LOD0.
                src.m_meshes[meshIdx].m_lods.push_back(AZStd::move(srcMesh));
            }
        }

        BuildResult br = BuildPackBytes(src);
        if (!br.m_success)
        {
            AZ_Error("Meshlets.JsonBuilder", false, "BuildPackBytes failed: %s",
                     br.m_errorMessage.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        AZStd::string productName;
        AZ::StringFunc::Path::GetFileName(request.m_fullPath.c_str(), productName);
        const AZStd::string productPath =
            request.m_tempDirPath + AZStd::string("/") + productName + ProductExt;

        AZ::IO::SystemFile out;
        if (!out.Open(productPath.c_str(),
                      AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY |
                      AZ::IO::SystemFile::SF_OPEN_CREATE))
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }
        out.Write(br.m_packBytes.data(), br.m_packBytes.size());
        out.Close();

        // Use MeshletPackAsset's stable type id (not CreateRandom!) so the
        // catalog records the right asset type — PackResolver filters by this
        // type to know which catalog entries are .azmeshletpack products and
        // populate its model→pack map. Using CreateRandom here breaks runtime
        // resolution: the editor's "No .azmeshletpack product registered"
        // warning fires even when the product file exists, because the
        // catalog's m_assetType for it never matches azrtti_typeid<MeshletPackAsset>().
        AssetBuilderSDK::JobProduct prod(productPath, azrtti_typeid<MeshletPackAsset>(), 0);
        prod.m_dependenciesHandled = true;
        prod.m_pathDependencies.emplace(
            AZStd::string::format("modelAsset:{%s}",
                                  sourceModelAssetId.ToString<AZStd::string>().c_str()),
            AssetBuilderSDK::ProductPathDependencyType::ProductFile);
        response.m_outputProducts.push_back(AZStd::move(prod));
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    void JsonSidecarBuilder::OnCancel()
    {
        m_cancelled.store(true);
    }

} // namespace AZ::Meshlets::Builders
