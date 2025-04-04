/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAssetHelpers.h>
#include <Atom/RPI.Reflect/Model/ModelKdTree.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/std/limits.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <AzFramework/Asset/AssetSystemBus.h>

namespace AZ
{
    namespace RPI
    {
        void ModelAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ModelAsset, Data::AssetData>()
                    ->Version(1)
                    ->Field("Name", &ModelAsset::m_name)
                    ->Field("Aabb", &ModelAsset::m_aabb)
                    ->Field("MaterialSlots", &ModelAsset::m_materialSlots)
                    ->Field("LodAssets", &ModelAsset::m_lodAssets)
                    ->Field("Tags", &ModelAsset::m_tags)
                    ;

                // Note: This class needs to have edit context reflection so PropertyAssetCtrl::OnEditButtonClicked
                //       can open the asset with the preferred asset editor (Scene Settings).
                if (auto* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<ModelAsset>("Model Asset", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ;
                }
            }
        }

        ModelAsset::ModelAsset()
        {
            // c-tor and d-tor have to be defined in .cpp in order to have AZStd::unique_ptr<ModelKdTree> without having to include the header of KDTree
        }

        ModelAsset::~ModelAsset()
        {
            // c-tor and d-tor have to be defined in .cpp in order to have AZStd::unique_ptr<ModelKdTree> without having to include the header of KDTree
        }

        const Name& ModelAsset::GetName() const
        {
            return m_name;
        }

        const Aabb& ModelAsset::GetAabb() const
        {
            return m_aabb;
        }
        
        const ModelMaterialSlotMap& ModelAsset::GetMaterialSlots() const
        {
            return m_materialSlots;
        }
            
        const ModelMaterialSlot& ModelAsset::FindMaterialSlot(uint32_t stableId) const
        {
            auto iter = m_materialSlots.find(stableId);

            if (iter == m_materialSlots.end())
            {
                return m_fallbackSlot;
            }
            else
            {
                return iter->second;
            }
        }

        size_t ModelAsset::GetLodCount() const
        {
            return m_lodAssets.size();
        }

        AZStd::span<const Data::Asset<ModelLodAsset>> ModelAsset::GetLodAssets() const
        {
            return AZStd::span<const Data::Asset<ModelLodAsset>>(m_lodAssets);
        }

        void ModelAsset::LoadBufferAssets()
        {
            for (auto& lodAsset : m_lodAssets)
            {
                lodAsset->LoadBufferAssets();
            }
        }

        void ModelAsset::ReleaseBufferAssets()
        {
            for (auto& lodAsset : m_lodAssets)
            {
                lodAsset->ReleaseBufferAssets();
            }
        }

        void ModelAsset::AddRefBufferAssets()
        {
            if (m_bufferAssetsRef == 0)
            {
                LoadBufferAssets();
            }
            m_bufferAssetsRef++;
        }

        void ModelAsset::ReleaseRefBufferAssets()
        {
            if (m_bufferAssetsRef > 0)
            {
                m_bufferAssetsRef--;
                if (m_bufferAssetsRef == 0)
                {
                    ReleaseBufferAssets();
                }
            }
        }

        bool ModelAsset::SupportLocalRayIntersection() const
        {
            return m_bufferAssetsRef > 0;
        }

        void ModelAsset::SetReady()
        {
            m_status = Data::AssetData::AssetStatus::Ready;
        }

        bool ModelAsset::LocalRayIntersectionAgainstModel(
            const AZ::Vector3& rayStart, const AZ::Vector3& rayDir, bool allowBruteForce,
            float& distanceNormalized, AZ::Vector3& normal) const
        {
            if (!m_modelTriangleCount)
            {
                // [GFX TODO][ATOM-4343 Bake mesh spatial information during AP processing]
                m_modelTriangleCount = CalculateTriangleCount();
            }

            // check the total vertex count for this model and skip kd-tree if the model is simple enough
            if (*m_modelTriangleCount > s_minimumModelTriangleCountToOptimize)
            {
                if (!m_kdTree)
                {
                    BuildKdTree();

                    AZ_WarningOnce("Model", false, "ray intersection against a model that is still creating spatial information");
                    return allowBruteForce ? BruteForceRayIntersect(rayStart, rayDir, distanceNormalized, normal) : false;
                }
                else
                {
                    return m_kdTree->RayIntersection(rayStart, rayDir, distanceNormalized, normal);
                }
            }

            return BruteForceRayIntersect(rayStart, rayDir, distanceNormalized, normal);
        }

        const AZStd::vector<AZ::Name>& ModelAsset::GetTags() const
        {
            return m_tags;
        }

        void ModelAsset::BuildKdTree() const
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_kdTreeLock);
            if ((m_isKdTreeCalculationRunning == false) && !m_kdTree)
            {
                m_isKdTreeCalculationRunning = true;

                // ModelAsset can go away while the job is queued up or is in progress, keep it alive until the job is done
                const_cast<ModelAsset*>(this)->Acquire();

                // [GFX TODO][ATOM-4343 Bake mesh spatial information during AP processing]
                // This is a temporary workaround to enable interactive Editor experience.
                // For runtime approach is to do this during asset processing and serialized spatial information alongside with mesh model assets
                const auto jobLambda = [&]() -> void
                {
                    AZ_PROFILE_FUNCTION(RPI);

                    AZStd::unique_ptr<ModelKdTree> tree = AZStd::make_unique<ModelKdTree>();
                    tree->Build(this);

                    AZStd::lock_guard<AZStd::mutex> jobLock(m_kdTreeLock);
                    m_isKdTreeCalculationRunning = false;
                    m_kdTree = AZStd::move(tree);

                    const_cast<ModelAsset*>(this)->Release();
                };

                Job* executeGroupJob = aznew JobFunction<decltype(jobLambda)>(jobLambda, true, nullptr); // Auto-deletes
                executeGroupJob->Start();
            }
        }

        bool ModelAsset::BruteForceRayIntersect(
            const AZ::Vector3& rayStart, const AZ::Vector3& rayDir, float& distanceNormalized, AZ::Vector3& normal) const
        {
            // brute force - check every triangle
            if (GetLodAssets().empty() == false)
            {
                // intersect against the highest level of detail
                if (ModelLodAsset* loadAssetPtr = GetLodAssets()[0].Get())
                {
                    bool anyHit = false;
                    AZ::Vector3 intersectionNormal;
                    float shortestDistanceNormalized = AZStd::numeric_limits<float>::max();
                    for (const ModelLodAsset::Mesh& mesh : loadAssetPtr->GetMeshes())
                    {
                        float currentDistanceNormalized;
                        if (LocalRayIntersectionAgainstMesh(mesh, rayStart, rayDir, currentDistanceNormalized, intersectionNormal))
                        {
                            anyHit = true;

                            if (currentDistanceNormalized < shortestDistanceNormalized)
                            {
                                normal = intersectionNormal;
                                shortestDistanceNormalized = currentDistanceNormalized;
                            }
                        }
                    }

                    if (anyHit)
                    {
                        distanceNormalized = shortestDistanceNormalized;
                    }

                    return anyHit;
                }
            }

            return false;
        }

        bool ModelAsset::LocalRayIntersectionAgainstMesh(
            const ModelLodAsset::Mesh& mesh,
            const AZ::Vector3& rayStart,
            const AZ::Vector3& rayDir,
            float& distanceNormalized,
            AZ::Vector3& normal) const
        {
            const BufferAssetView& indexBufferView = mesh.GetIndexBufferAssetView();
            const BufferAssetView* positionBufferView = mesh.GetSemanticBufferAssetView(m_positionName);

            if (positionBufferView && positionBufferView->GetBufferAsset().Get())
            {
                BufferAsset* bufferAssetViewPtr = positionBufferView->GetBufferAsset().Get();
                BufferAsset* indexAssetViewPtr = indexBufferView.GetBufferAsset().Get();

                if (!bufferAssetViewPtr || !indexAssetViewPtr)
                {
                    return false;
                }

                RHI::BufferViewDescriptor positionBufferViewDesc = positionBufferView->GetBufferViewDescriptor();
                AZStd::span<const uint8_t> positionRawBuffer = bufferAssetViewPtr->GetBuffer();

                const uint32_t positionElementSize = positionBufferViewDesc.m_elementSize;
                const uint32_t positionElementCount = positionBufferViewDesc.m_elementCount;

                // Position is 3 floats
                if (positionElementSize != sizeof(float) * 3)
                {
                    AZ_Warning(
                        "ModelAsset", false, "unsupported mesh posiiton format, only full 3 floats per vertex are supported at the moment");
                    return false;
                }

                RHI::BufferViewDescriptor indexBufferViewDesc = indexBufferView.GetBufferViewDescriptor();
                AZStd::span<const uint8_t> indexRawBuffer = indexAssetViewPtr->GetBuffer();

                const AZ::Vector3 rayEnd = rayStart + rayDir;
                AZ::Vector3 a, b, c;
                AZ::Vector3 intersectionNormal;

                bool anyHit = false;
                float shortestDistanceNormalized = AZStd::numeric_limits<float>::max();

                const AZ::u32* indexPtr = reinterpret_cast<const AZ::u32*>(
                    indexRawBuffer.data() + (indexBufferViewDesc.m_elementOffset * indexBufferViewDesc.m_elementSize));
                const float* positionPtr = reinterpret_cast<const float*>(
                    positionRawBuffer.data() + (positionBufferViewDesc.m_elementOffset * positionBufferViewDesc.m_elementSize));

                Intersect::SegmentTriangleHitTester hitTester(rayStart, rayEnd);

                constexpr int StepSize = 3; // number of values per vertex (x, y, z)
                for (uint32_t indexIter = 0; indexIter < indexBufferViewDesc.m_elementCount; indexIter += StepSize, indexPtr += StepSize)
                {
                    AZ::u32 index0 = indexPtr[0];
                    AZ::u32 index1 = indexPtr[1];
                    AZ::u32 index2 = indexPtr[2];

                    if (index0 >= positionElementCount || index1 >= positionElementCount || index2 >= positionElementCount)
                    {
                        AZ_Warning("ModelAsset", false, "mesh has a bad vertex index");
                        return false;
                    }

                    // faster than AZ::Vector3 c-tor
                    const float* aRef = &positionPtr[index0 * StepSize];
                    a.Set(aRef);
                    const float* bRef = &positionPtr[index1 * StepSize];
                    b.Set(bRef);
                    const float* cRef = &positionPtr[index2 * StepSize];
                    c.Set(cRef);

                    float currentDistanceNormalized;
                    if (hitTester.IntersectSegmentTriangleCCW(a, b, c, intersectionNormal, currentDistanceNormalized))
                    {
                        anyHit = true;

                        if (currentDistanceNormalized < shortestDistanceNormalized)
                        {
                            normal = intersectionNormal;
                            shortestDistanceNormalized = currentDistanceNormalized;
                        }
                    }
                }

                if (anyHit)
                {
                    distanceNormalized = shortestDistanceNormalized;
                }

                return anyHit;
            }

            return false;
        }

        AZStd::size_t ModelAsset::CalculateTriangleCount() const
        {
            AZStd::size_t modelTriangleCount = 0;

            if (GetLodAssets().empty() == false)
            {
                if (ModelLodAsset* loadAssetPtr = GetLodAssets()[0].Get())
                {
                    for (const ModelLodAsset::Mesh& mesh : loadAssetPtr->GetMeshes())
                    {
                        const AZStd::span<const ModelLodAsset::Mesh::StreamBufferInfo>& streamBufferList = mesh.GetStreamBufferInfoList();

                        // find position semantic
                        const ModelLodAsset::Mesh::StreamBufferInfo* positionBuffer = nullptr;

                        for (const ModelLodAsset::Mesh::StreamBufferInfo& bufferInfo : streamBufferList)
                        {
                            if (bufferInfo.m_semantic.m_name == m_positionName)
                            {
                                positionBuffer = &bufferInfo;
                                break;
                            }
                        }

                        if (positionBuffer)
                        {
                            const RHI::BufferViewDescriptor& desc = positionBuffer->m_bufferAssetView.GetBufferViewDescriptor();
                            modelTriangleCount += desc.m_elementCount / 3;
                        }
                    }
                }
            }

            AZ_Warning("Model", modelTriangleCount < ((2<<23) / 3), "Model has too many vertices for the spatial optimization. Currently only up to 16,777,216 is supported");

            return modelTriangleCount;
        }

        void ModelAsset::InitData(
            AZ::Name name,
            AZStd::span<Data::Asset<ModelLodAsset>> lodAssets,
            const ModelMaterialSlotMap& materialSlots,
            const ModelMaterialSlot& fallbackSlot,
            AZStd::span<AZ::Name> tags)
        {
            AZ_Assert(!m_isKdTreeCalculationRunning, "Overwriting a ModelAsset while it is calculating its kd tree.");

            // Clear out the current AABB, we'll reset it with the data from the LOD assets.
            m_aabb = AZ::Aabb::CreateNull();

            // Copy the trivially-copyable data.
            m_name = name;
            m_materialSlots = materialSlots;
            m_fallbackSlot = fallbackSlot;

            // Clear out the runtime-calculated data.
            m_kdTree = {};
            m_isKdTreeCalculationRunning = false;
            m_modelTriangleCount = {};

            // Clear out tags and LOD Assets, we'll set those individually.
            m_lodAssets.clear();
            m_tags = {};

            for (const auto& lodAsset : lodAssets)
            {
                m_lodAssets.push_back(lodAsset);
                if (lodAsset.IsReady())
                {
                    m_aabb.AddAabb(lodAsset->GetAabb());
                }
            }

            for (const auto& tag : tags)
            {
                m_tags.push_back(tag);
            }
        }

        // Create a stable ID for our default fallback model.
        const AZ::Data::AssetId ModelAssetHandler::s_defaultModelAssetId{ "{D676DD3C-0560-4F39-99E0-B6DCBC7CEDAA}", 0 };

        Data::AssetHandler::LoadResult ModelAssetHandler::LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB)
        {
            // If there's a 0-length stream, this must be trying to load our default fallback model.
            // Fill in the asset data with a generated unit X-shaped model.
            // We need to generate the data instead of load a fallback asset because model assets have dependencies
            // on buffer and material assets, and fallback assets need to not have any dependencies to be able to load
            // correctly when used as fallbacks. (The AssetManager doesn't currently support handling dependency pre-loading
            // for fallback assets)
            if (stream->GetLength() == 0)
            {
                auto assetData = asset.GetAs<AZ::RPI::ModelAsset>();
                if (assetData)
                {
                    ModelAssetHelpers::CreateUnitX(assetData);
                }

                return Data::AssetHandler::LoadResult::LoadComplete;
            }

            return AssetHandler::LoadAssetData(asset, stream, assetLoadFilterCB);
        }

        Data::AssetId ModelAssetHandler::AssetMissingInCatalog(const Data::Asset<Data::AssetData>& asset)
        {
            AZ_Info(
                "Model",
                "Model id " AZ_STRING_FORMAT " not found in asset catalog, using fallback model.\n",
                AZ_STRING_ARG(asset.GetId().ToFixedString()));

            // Find out if the asset is missing completely or just still processing
            // and escalate the asset to the top of the list if it's queued.
            AzFramework::AssetSystem::AssetStatus missingAssetStatus = AzFramework::AssetSystem::AssetStatus::AssetStatus_Unknown;
            AzFramework::AssetSystemRequestBus::BroadcastResult(
                missingAssetStatus, &AzFramework::AssetSystem::AssetSystemRequests::GetAssetStatusById, asset.GetId().m_guid);

            if (missingAssetStatus == AzFramework::AssetSystem::AssetStatus::AssetStatus_Queued)
            {
                bool sendSucceeded = false;
                AzFramework::AssetSystemRequestBus::BroadcastResult(
                    sendSucceeded, &AzFramework::AssetSystem::AssetSystemRequests::EscalateAssetByUuid, asset.GetId().m_guid);
            }

            // Make sure the default model asset has an entry in the asset catalog so that the asset system will try to load it.
            // Note that we specifically give it a 0-byte size and a non-empty path so that the load will just trivially succeed
            // with a 0-byte asset stream. This will enable us to detect it in LoadAssetData and fill in the data with a generated
            // unit cube. We can't use an on-disk model asset because the AssetMissing system currently doesn't correctly handle
            // assets with dependent assets (like azmodel), so we need to just load an "empty" asset and then fill it in with data
            // in LoadAssetData.
            {
                Data::AssetInfo assetInfo;
                assetInfo.m_assetId = s_defaultModelAssetId;
                assetInfo.m_assetType = azrtti_typeid<AZ::RPI::ModelAsset>();
                assetInfo.m_relativePath = "default_fallback_model";
                assetInfo.m_sizeBytes = 0;
                AZ::Data::AssetCatalogRequestBus::Broadcast(
                    &AZ::Data::AssetCatalogRequestBus::Events::RegisterAsset, assetInfo.m_assetId, assetInfo);
            }

            return s_defaultModelAssetId;
        }


    } // namespace RPI
} // namespace AZ
