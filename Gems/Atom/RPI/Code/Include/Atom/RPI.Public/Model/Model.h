/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Base.h>

#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelLodIndex.h>

#include <Atom/RPI.Public/Model/ModelLod.h>

#include <AtomCore/Instance/InstanceData.h>

#include <AzCore/std/containers/fixed_vector.h>
#include <AzCore/std/containers/unordered_set.h>

namespace AZ
{
    namespace RPI
    {
        class ModelAsset;

        class Model final
            : public Data::InstanceData
        {
            friend class ModelSystem;

        public:
            AZ_INSTANCE_DATA(Model, "{C30F5522-B381-4B38-BBAF-6E0B1885C8B9}");
            AZ_CLASS_ALLOCATOR(Model, AZ::SystemAllocator, 0);

            static Data::Instance<Model> FindOrCreate(const Data::Asset<ModelAsset>& modelAsset);

            //! Orphan the model, its lods, and all their buffers so that they can be replaced in the instance database
            //! This is a temporary function, that will be removed once the Model/ModelAsset classes no longer need it
            static void TEMPOrphanFromDatabase(const Data::Asset<ModelAsset>& modelAsset);

            ~Model() = default;

            //! Blocks the CPU until the streaming upload is complete. Returns immediately if no
            //! streaming upload is currently pending.
            void WaitForUpload();

            //! Returns the number of Lods in the model.
            size_t GetLodCount() const;

            //! Returns the full list of Lods, where index 0 is the most detailed, and N-1 is the least.
            AZStd::array_view<Data::Instance<ModelLod>> GetLods() const;

            //! Returns whether a buffer upload is pending.
            bool IsUploadPending() const;

            const Data::Asset<ModelAsset>& GetModelAsset() const;

            //! Checks a ray for intersection against this model. The ray must be in the same coordinate space as the model.
            //! Important: only to be used in the Editor, it may kick off a job to calculate spatial information.
            //! [GFX TODO][ATOM-4343 Bake mesh spatial during AP processing]
            //!
            //! @param rayStart  The starting point of the ray.
            //! @param rayDir  The direction and length of the ray (magnitude is encoded in the direction).
            //! @param[out] distanceNormalized  If an intersection is found, will be set to the normalized distance of the intersection
            //! (in the range 0.0-1.0) - to calculate the actual distance, multiply distanceNormalized by the magnitude of rayDir.
            //! @param[out] normal If an intersection is found, will be set to the normal at the point of collision.
            //! @return  True if the ray intersects the mesh.
            bool LocalRayIntersection(const AZ::Vector3& rayStart, const AZ::Vector3& rayDir, float& distanceNormalized, AZ::Vector3& normal) const;

            //! Checks a ray for intersection against this model, where the ray is in a different coordinate space.
            //! Important: only to be used in the Editor, it may kick off a job to calculate spatial information.
            //! [GFX TODO][ATOM-4343 Bake mesh spatial during AP processing]
            //!
            //! @param modelTransform  a transform that puts the model into the ray's coordinate space
            //! @param nonUniformScale  Non-uniform scale applied in the model's local frame.
            //! @param rayStart  The starting point of the ray.
            //! @param rayDir  The direction and length of the ray (magnitude is encoded in the direction).
            //! @param[out] distanceNormalized  If an intersection is found, will be set to the normalized distance of the intersection
            //! (in the range 0.0-1.0) - to calculate the actual distance, multiply distanceNormalized by the magnitude of rayDir.
            //! @param[out] normal If an intersection is found, will be set to the normal at the point of collision.
            //! @return  True if the ray intersects the mesh.
            bool RayIntersection(
                const AZ::Transform& modelTransform,
                const AZ::Vector3& nonUniformScale,
                const AZ::Vector3& rayStart,
                const AZ::Vector3& rayDir,
                float& distanceNormalized,
                AZ::Vector3& normal) const;

            //! Get available UV names from the model and its lods.
            const AZStd::unordered_set<AZ::Name>& GetUvNames() const;

        private:
            Model() = default;

            static Data::Instance<Model> CreateInternal(const Data::Asset<ModelAsset>& modelAsset);
            RHI::ResultCode Init(const Data::Asset<ModelAsset>& modelAsset);

            AZStd::fixed_vector<Data::Instance<ModelLod>, ModelLodAsset::LodCountMax> m_lods;
            Data::Asset<ModelAsset> m_modelAsset;

            AZStd::unordered_set<AZ::Name> m_uvNames;

            // Tracks whether buffers have all been streamed up to the GPU.
            bool m_isUploadPending = false;
        };
    } // namespace RPI
} // namespace AZ
