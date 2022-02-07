/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

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

        
        void Model::TEMPOrphanFromDatabase(const Data::Asset<ModelAsset>& modelAsset)
        {
            for (auto& modelLodAsset : modelAsset->GetLodAssets())
            {
                for(auto& mesh : modelLodAsset->GetMeshes())
                {
                    for (auto& streamBufferInfo : mesh.GetStreamBufferInfoList())
                    {
                        Data::InstanceDatabase<Buffer>::Instance().TEMPOrphan(
                            Data::InstanceId::CreateFromAssetId(streamBufferInfo.m_bufferAssetView.GetBufferAsset().GetId()));
                    }
                    Data::InstanceDatabase<Buffer>::Instance().TEMPOrphan(
                        Data::InstanceId::CreateFromAssetId(mesh.GetIndexBufferAssetView().GetBufferAsset().GetId()));
                }
                Data::InstanceDatabase<ModelLod>::Instance().TEMPOrphan(Data::InstanceId::CreateFromAssetId(modelLodAsset.GetId()));
            }

            Data::InstanceDatabase<Model>::Instance().TEMPOrphan(
                Data::InstanceId::CreateFromAssetId(modelAsset.GetId()));
        }

        size_t Model::GetLodCount() const
        {
            return m_lods.size();
        }

        AZStd::array_view<Data::Instance<ModelLod>> Model::GetLods() const
        {
            return m_lods;
        }

        Data::Instance<Model> Model::CreateInternal(const Data::Asset<ModelAsset>& modelAsset)
        {
            AZ_PROFILE_SCOPE(RPI, "Model: CreateInternal");
            Data::Instance<Model> model = aznew Model();
            const RHI::ResultCode resultCode = model->Init(modelAsset);

            if (resultCode == RHI::ResultCode::Success)
            {
                return model;
            }

            return nullptr;
        }

        RHI::ResultCode Model::Init(const Data::Asset<ModelAsset>& modelAsset)
        {
            AZ_PROFILE_SCOPE(RPI, "Model: Init");

            m_lods.resize(modelAsset->GetLodAssets().size());

            for (size_t lodIndex = 0; lodIndex < m_lods.size(); ++lodIndex)
            {
                const Data::Asset<ModelLodAsset>& lodAsset = modelAsset->GetLodAssets()[lodIndex];

                if (!lodAsset)
                {
                    AZ_Error("Model", false, "Invalid Operation: Last ModelLod is not loaded.");
                    return RHI::ResultCode::Fail;
                }

                Data::Instance<ModelLod> lodInstance = ModelLod::FindOrCreate(lodAsset, modelAsset);
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

            m_modelAsset = modelAsset;
            m_isUploadPending = true;
            return RHI::ResultCode::Success;
        }

        void Model::WaitForUpload()
        {
            if (m_isUploadPending)
            {
                AZ_PROFILE_SCOPE(RPI, "Model::WaitForUpload - %s", GetDatabaseName());
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

        const Data::Asset<ModelAsset>& Model::GetModelAsset() const
        {
            return m_modelAsset;
        }

        bool Model::LocalRayIntersection(const AZ::Vector3& rayStart, const AZ::Vector3& rayDir, float& distanceNormalized, AZ::Vector3& normal) const
        {
            AZ_PROFILE_SCOPE(RPI, "Model: LocalRayIntersection");
            
            if (!GetModelAsset())
            {
                AZ_Assert(false, "Invalid Model - not created from a ModelAsset?");
                return false;
            }
            
            float start;
            float end;
            const int result = Intersect::IntersectRayAABB2(rayStart, rayDir.GetReciprocal(), GetModelAsset()->GetAabb(), start, end);
            if (Intersect::ISECT_RAY_AABB_NONE != result)
            {
                if (ModelAsset* modelAssetPtr = m_modelAsset.Get())
                {
#if defined(AZ_RPI_PROFILE_RAYCASTING_AGAINST_MODELS)
                    AZ::Debug::Timer timer;
                    timer.Stamp();
#endif
                    constexpr bool AllowBruteForce = false;
                    const bool hit = modelAssetPtr->LocalRayIntersectionAgainstModel(
                        rayStart, rayDir, AllowBruteForce, distanceNormalized, normal);
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

        bool Model::RayIntersection(
            const AZ::Transform& modelTransform,
            const AZ::Vector3& nonUniformScale,
            const AZ::Vector3& rayStart,
            const AZ::Vector3& rayDir,
            float& distanceNormalized,
            AZ::Vector3& normal) const
        {
            AZ_PROFILE_SCOPE(RPI, "Model: RayIntersection");
            const AZ::Vector3 clampedScale = nonUniformScale.GetMax(AZ::Vector3(AZ::MinTransformScale));

            const AZ::Transform inverseTM = modelTransform.GetInverse();
            const AZ::Vector3 raySrcLocal = inverseTM.TransformPoint(rayStart) / clampedScale;

            // Instead of just rotating 'rayDir' we need it to be scaled too, so that 'distanceNormalized' will be in the target units rather
            // than object local units.
            const AZ::Vector3 rayDest = rayStart + rayDir;
            const AZ::Vector3 rayDestLocal = inverseTM.TransformPoint(rayDest) / clampedScale;
            const AZ::Vector3 rayDirLocal = rayDestLocal - raySrcLocal;

            const bool result = LocalRayIntersection(raySrcLocal, rayDirLocal, distanceNormalized, normal);
            normal = (normal * clampedScale).GetNormalized();
            return result;
        }

        const AZStd::unordered_set<AZ::Name>& Model::GetUvNames() const
        {
            return m_uvNames;
        }
    } // namespace RPI
} // namespace AZ
