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
        class Model final
            : public Data::InstanceData
        {
            friend class ModelSystem;
        public:
            AZ_INSTANCE_DATA(Model, "{C30F5522-B381-4B38-BBAF-6E0B1885C8B9}");
            AZ_CLASS_ALLOCATOR(Model, AZ::SystemAllocator, 0);

            static Data::Instance<Model> FindOrCreate(const Data::Asset<ModelAsset>& modelAsset);

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

            const AZ::Aabb& GetAabb() const;

            const Data::Asset<ModelAsset>& GetModelAsset() const;

            //! Checks a ray for intersection against this model. The ray must be in the same coordinate space as the model.
            //! Important: only to be used in the Editor, it may kick off a job to calculate spatial information.
            //! [GFX TODO][ATOM-4343 Bake mesh spatial during AP processing]
            //!
            //! @param rayStart  position where the ray starts
            //! @param dir  direction where the ray ends (does not have to be unit length)
            //! @param distanceFactor  if an intersection is detected, this will be set such that distanceFactor * dir.length == distance to intersection
            //! @return  true if the ray intersects the mesh
            bool LocalRayIntersection(const AZ::Vector3& rayStart, const AZ::Vector3& dir, float& distanceFactor) const;

            //! Checks a ray for intersection against this model, where the ray is in a different coordinate space.
            //! Important: only to be used in the Editor, it may kick off a job to calculate spatial information.
            //! [GFX TODO][ATOM-4343 Bake mesh spatial during AP processing]
            //!
            //! @param modelTransform  a transform that puts the model into the ray's coordinate space
            //! @param nonUniformScale  Non-uniform scale applied in the model's local frame.
            //! @param rayStart  position where the ray starts
            //! @param dir  direction where the ray ends (does not have to be unit length)
            //! @param distanceFactor  if an intersection is detected, this will be set such that distanceFactor * dir.length == distance to intersection
            //! @return  true if the ray intersects the mesh
            bool RayIntersection(const AZ::Transform& modelTransform, const AZ::Vector3& nonUniformScale, const AZ::Vector3& rayStart,
                const AZ::Vector3& dir, float& distanceFactor) const;

            //! Get available UV names from the model and its lods.
            const AZStd::unordered_set<AZ::Name>& GetUvNames() const;

        private:
            Model() = default;

            static Data::Instance<Model> CreateInternal(ModelAsset& modelAsset);
            RHI::ResultCode Init(ModelAsset& modelAsset);

            AZStd::fixed_vector<Data::Instance<ModelLod>, ModelLodAsset::LodCountMax> m_lods;
            Data::Asset<ModelAsset> m_modelAsset;

            AZStd::unordered_set<AZ::Name> m_uvNames;

            // Tracks whether buffers have all been streamed up to the GPU.
            bool m_isUploadPending = false;

            AZ::Aabb m_aabb;
        };
    } // namespace RPI
} // namespace AZ
