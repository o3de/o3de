/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelKdTree.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Math/IntersectSegment.h>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        const char* ModelAsset::DisplayName = "ModelAsset";
        const char* ModelAsset::Group = "Model";
        const char* ModelAsset::Extension = "azmodel";

        void ModelAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ModelAsset, Data::AssetData>()
                    ->Version(0)
                    ->Field("Name", &ModelAsset::m_name)
                    ->Field("Aabb", &ModelAsset::m_aabb)
                    ->Field("LodAssets", &ModelAsset::m_lodAssets)
                    ;
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

        size_t ModelAsset::GetLodCount() const
        {
            return m_lodAssets.size();
        }

        AZStd::array_view<Data::Asset<ModelLodAsset>> ModelAsset::GetLodAssets() const
        {
            return AZStd::array_view<Data::Asset<ModelLodAsset>>(m_lodAssets);
        }

        void ModelAsset::SetReady()
        {
            m_status = Data::AssetData::AssetStatus::Ready;
        }

        bool ModelAsset::LocalRayIntersectionAgainstModel(const AZ::Vector3& rayStart, const AZ::Vector3& dir, float& distance, AZ::Vector3& normal) const
        {
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzRender);

            if (!m_modelTriangleCount)
            {
                // [GFX TODO][ATOM-4343 Bake mesh spatial information during AP processing]
                m_modelTriangleCount = CalculateTriangleCount();
            }

            // check the total vertex count for this model and skip kdtree if the model is simple enough
            if (*m_modelTriangleCount > s_minimumModelTriangleCountToOptimize)
            {
                if (!m_kdTree)
                {
                    BuildKdTree();

                    AZ_WarningOnce("Model", false, "ray intersection against a model that is still creating spatial information");
                    return false;
                }
                else
                {
                    return m_kdTree->RayIntersection(rayStart, dir, distance, normal);
                }
            }

            return BruteForceRayIntersect(rayStart, dir, distance, normal);
        }

        void ModelAsset::BuildKdTree() const
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_kdTreeLock);
            if (m_isKdTreeCalculationRunning == false)
            {
                m_isKdTreeCalculationRunning = true;

                // ModelAsset can go away while the job is queued up or is in progress, keep it alive until the job is done
                const_cast<ModelAsset*>(this)->Acquire();

                // [GFX TODO][ATOM-4343 Bake mesh spatial information during AP processing]
                // This is a temporary workaround to enable interactive Editor experience.
                // For runtime approach is to do this during asset processing and serialized spatial information alongside with mesh model assets
                const auto jobLambda = [&]() -> void
                {
                    AZ_TRACE_METHOD();

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

        bool ModelAsset::BruteForceRayIntersect(const AZ::Vector3& rayStart, const AZ::Vector3& dir, float& distance, AZ::Vector3& normal) const
        {
            // brute force - check every triangle
            if (GetLodAssets().empty() == false)
            {
                // intersect against the highest level of detail
                if (ModelLodAsset* loadAssetPtr = GetLodAssets()[0].Get())
                {
                    float shortestDistance = std::numeric_limits<float>::max();
                    bool anyHit = false;

                    AZ::Vector3 intersectionNormal;

                    for (const ModelLodAsset::Mesh& mesh : loadAssetPtr->GetMeshes())
                    {
                        if (LocalRayIntersectionAgainstMesh(mesh, rayStart, dir, distance, intersectionNormal))
                        {
                            anyHit = true;
                            if (distance < shortestDistance)
                            {
                                normal = intersectionNormal;
                                shortestDistance = distance;
                            }
                        }
                    }

                    if (anyHit)
                    {
                        distance = shortestDistance;
                    }

                    return anyHit;
                }
            }

            return false;
        }

        bool ModelAsset::LocalRayIntersectionAgainstMesh(const ModelLodAsset::Mesh& mesh, const AZ::Vector3& rayStart, const AZ::Vector3& dir, float& distance, AZ::Vector3& normal) const
        {
            const BufferAssetView& indexBufferView = mesh.GetIndexBufferAssetView();
            const AZStd::array_view<ModelLodAsset::Mesh::StreamBufferInfo>& streamBufferList = mesh.GetStreamBufferInfoList();

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

            if (positionBuffer && positionBuffer->m_bufferAssetView.GetBufferAsset().Get())
            {
                BufferAsset* bufferAssetViewPtr = positionBuffer->m_bufferAssetView.GetBufferAsset().Get();
                BufferAsset* indexAssetViewPtr = indexBufferView.GetBufferAsset().Get();

                if (!bufferAssetViewPtr || !indexAssetViewPtr)
                {
                    return false;
                }

                RHI::BufferViewDescriptor positionBufferViewDesc = bufferAssetViewPtr->GetBufferViewDescriptor();
                AZStd::array_view<uint8_t> positionRawBuffer = bufferAssetViewPtr->GetBuffer();

                const uint32_t positionElementSize = positionBufferViewDesc.m_elementSize;
                const uint32_t positionElementCount = positionBufferViewDesc.m_elementCount;

                // Position is 3 floats
                if (positionElementSize != sizeof(float) * 3)
                {
                    AZ_Warning("ModelAsset", false, "unsupported mesh posiiton format, only full 3 floats per vertex are supported at the moment");
                    return false;
                }

                AZStd::array_view<uint8_t> indexRawBuffer = indexAssetViewPtr->GetBuffer();
                RHI::BufferViewDescriptor indexRawDesc = indexAssetViewPtr->GetBufferViewDescriptor();

                float closestNormalizedDistance = 1.f;
                bool anyHit = false;

                const AZ::Vector3 rayEnd = rayStart + dir * distance;
                AZ::Vector3 a, b, c;
                AZ::Vector3 intersectionNormal;
                float normalizedDistance = 1.f;

                const AZ::u32* indexPtr = reinterpret_cast<const AZ::u32*>(indexRawBuffer.data());
                for (uint32_t indexIter = 0; indexIter <= indexRawDesc.m_elementCount - 3; indexIter += 3, indexPtr += 3)
                {
                    AZ::u32 index0 = indexPtr[0];
                    AZ::u32 index1 = indexPtr[1];
                    AZ::u32 index2 = indexPtr[2];

                    if (index0 >= positionElementCount || index1 >= positionElementCount || index2 >= positionElementCount)
                    {
                        AZ_Warning("ModelAsset", false, "mesh has a bad vertex index");
                        return false;
                    }

                    const float* p = reinterpret_cast<const float*>(&positionRawBuffer[index0 * positionElementSize]);
                    a.Set(const_cast<float*>(p)); // faster than AZ::Vector3 c-tor

                    p = reinterpret_cast<const float*>(&positionRawBuffer[index1 * positionElementSize]);
                    b.Set(const_cast<float*>(p));

                    p = reinterpret_cast<const float*>(&positionRawBuffer[index2 * positionElementSize]);
                    c.Set(const_cast<float*>(p));

                    if (AZ::Intersect::IntersectSegmentTriangleCCW(rayStart, rayEnd, a, b, c, intersectionNormal, normalizedDistance))
                    {
                        if (normalizedDistance < closestNormalizedDistance)
                        {
                            normal = intersectionNormal;
                            closestNormalizedDistance = normalizedDistance;
                        }
                        anyHit = true;
                    }
                }

                if (anyHit)
                {
                    distance = closestNormalizedDistance * distance;
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
                        const AZStd::array_view<ModelLodAsset::Mesh::StreamBufferInfo>& streamBufferList = mesh.GetStreamBufferInfoList();

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
    } //namespace RPI
} // namespace AZ
