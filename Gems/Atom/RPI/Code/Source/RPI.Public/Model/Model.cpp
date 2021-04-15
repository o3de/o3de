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

#include <Atom/RPI.Public/Model/Model.h>

#include <Atom/RHI/Factory.h>

#include <AzCore/Debug/EventTrace.h>
#include <AtomCore/Instance/InstanceDatabase.h>
#include <AzCore/Debug/Timer.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Math/IntersectSegment.h>

// Enable to show profile logs of how long it takes to raycast against models in the Editor
//#define AZ_RPI_PROFILE_RAYCASTING_AGAINST_MODELS

namespace AZ
{
    namespace RPI
    {
        Data::Instance<Model> Model::FindOrCreate(const Data::Asset<ModelAsset>& modelAsset)
        {
            return Data::InstanceDatabase<Model>::Instance().FindOrCreate(
                Data::InstanceId::CreateFromAssetId(modelAsset.GetId()),
                modelAsset);
        }

        size_t Model::GetLodCount() const
        {
            return m_lods.size();
        }

        AZStd::array_view<Data::Instance<ModelLod>> Model::GetLods() const
        {
            return m_lods;
        }

        Data::Instance<Model> Model::CreateInternal(ModelAsset& modelAsset)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            Data::Instance<Model> model = aznew Model();
            const RHI::ResultCode resultCode = model->Init(modelAsset);

            if (resultCode == RHI::ResultCode::Success)
            {
                return model;
            }

            return nullptr;
        }

        RHI::ResultCode Model::Init(ModelAsset& modelAsset)
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

            m_aabb = modelAsset.GetAabb();

            m_lods.resize(modelAsset.GetLodAssets().size());

            for (size_t lodIndex = 0; lodIndex < m_lods.size(); ++lodIndex)
            {
                const Data::Asset<ModelLodAsset>& lodAsset = modelAsset.GetLodAssets()[lodIndex];

                if (!lodAsset)
                {
                    AZ_Error("Model", false, "Invalid Operation: Last ModelLod is not loaded.");
                    return RHI::ResultCode::Fail;
                }

                Data::Instance<ModelLod> lodInstance = ModelLod::FindOrCreate(lodAsset);
                if (lodInstance == nullptr)
                {
                    return RHI::ResultCode::Fail;
                }

                for (const AZ::RPI::ModelLod::Mesh& mesh : lodInstance->GetMeshes())
                {
                    for (const AZ::RPI::ModelLod::StreamBufferInfo& stream : mesh.m_streamInfo)
                    {
                        if (stream.m_semantic.m_name.GetStringView().starts_with(RHI::ShaderSemantic::UvStreamSemantic))
                        {
                            // For unnamed UVs, use the semantic instead.
                            if (stream.m_customName.IsEmpty())
                            {
                                m_uvNames.insert(AZ::Name(stream.m_semantic.ToString()));
                            }
                            else
                            {
                                m_uvNames.insert(stream.m_customName);
                            }
                        }
                    }
                }

                m_lods[lodIndex] = AZStd::move(lodInstance);
            }

            m_modelAsset = { &modelAsset, AZ::Data::AssetLoadBehavior::PreLoad };
            m_isUploadPending = true;
            return RHI::ResultCode::Success;
        }

        void Model::WaitForUpload()
        {
            if (m_isUploadPending)
            {
                AZ_PROFILE_SCOPE_STALL_DYNAMIC(Debug::ProfileCategory::AzRender, "Model::WaitForUpload - %s", GetDatabaseName());
                for (const Data::Instance<ModelLod>& lod : m_lods)
                {
                    lod->WaitForUpload();
                }
                m_isUploadPending = false;
            }
        }

        bool Model::IsUploadPending() const
        {
            return m_isUploadPending;
        }

        const AZ::Aabb& Model::GetAabb() const
        {
            return m_aabb;
        }

        const Data::Asset<ModelAsset>& Model::GetModelAsset() const
        {
            return m_modelAsset;
        }

        bool Model::LocalRayIntersection(const AZ::Vector3& rayStart, const AZ::Vector3& dir, float& distance, AZ::Vector3& normal) const
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            float firstHit;
            const int result = Intersect::IntersectRayAABB2(rayStart, dir.GetReciprocal(), m_aabb, firstHit, distance);
            if (Intersect::ISECT_RAY_AABB_NONE != result)
            {
                if (ModelAsset* modelAssetPtr = m_modelAsset.Get())
                {
#if defined(AZ_RPI_PROFILE_RAYCASTING_AGAINST_MODELS)
                    AZ::Debug::Timer timer;
                    timer.Stamp();
#endif
                    const bool hit = modelAssetPtr->LocalRayIntersectionAgainstModel(rayStart, dir, distance, normal);
#if defined(AZ_RPI_PROFILE_RAYCASTING_AGAINST_MODELS)
                    if (hit)
                    {
                        AZ_Printf("Model", "Model::LocalRayIntersection took %.2f ms", timer.StampAndGetDeltaTimeInSeconds() * 1000.f);
                    }
#endif
                    return hit;
                }
            }

            return false;
        }

        bool Model::RayIntersection(const AZ::Transform& modelTransform, const AZ::Vector3& nonUniformScale, const AZ::Vector3& rayStart, const AZ::Vector3& dir, float& distanceFactor, AZ::Vector3& normal) const
        {
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
            const AZ::Transform inverseTM = modelTransform.GetInverse();
            const AZ::Vector3 raySrcLocal = inverseTM.TransformPoint(rayStart) / nonUniformScale;

            // Instead of just rotating 'dir' we need it to be scaled too, so that 'distanceFactor' will be in the target units rather than object local units.
            const AZ::Vector3 rayDest = rayStart + dir;
            const AZ::Vector3 rayDestLocal = inverseTM.TransformPoint(rayDest) / nonUniformScale;
            const AZ::Vector3 rayDirLocal = rayDestLocal - raySrcLocal;

            bool result = LocalRayIntersection(raySrcLocal, rayDirLocal, distanceFactor, normal);
            normal = (normal * nonUniformScale).GetNormalized();
            return result;
        }

        const AZStd::unordered_set<AZ::Name>& Model::GetUvNames() const
        {
            return m_uvNames;
        }
    } // namespace RPI
} // namespace AZ
