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

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class ModelKdTree;

        //! Contains a set of RPI::ModelLodAsset objects.
        //! Serialized to a .azmodel file.
        //! Actual model data is stored in the BufferAssets referenced by ModelLodAssets.
        class ModelAsset
            : public AZ::Data::AssetData
        {
            friend class ModelAssetCreator;

        public:
            static const char* DisplayName;
            static const char* Extension;
            static const char* Group;

            AZ_RTTI(ModelAsset, "{2C7477B6-69C5-45BE-8163-BCD6A275B6D8}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(ModelAsset, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            ModelAsset();
            virtual ~ModelAsset();

            //! Returns the human readable name of the model
            const Name& GetName() const;

            //! Returns the model-space axis aligned bounding box
            const AZ::Aabb& GetAabb() const;

            //! Returns the number of Lods in the model
            size_t GetLodCount() const;

            AZStd::array_view<Data::Asset<ModelLodAsset>> GetLodAssets() const;

            //! Checks a ray for intersection against this model. The ray must be in the same coordinate space as the model.
            //! Important: only to be used in the Editor, it may kick off a job to calculate spatial information.
            //! [GFX TODO][ATOM-4343 Bake mesh spatial information during AP processing]
            //!
            //! @param rayStart  position where the ray starts
            //! @param dir  direction where the ray ends (does not have to be unit length)
            //! @param distance  if an intersection is detected, this will be set such that distanceFactor * dir.length == distance to intersection
            //! @param normal if an intersection is detected, this will be set to the normal at the point of collision
            //! @return  true if the ray intersects the mesh
            virtual bool LocalRayIntersectionAgainstModel(const AZ::Vector3& rayStart, const AZ::Vector3& dir, float& distance, AZ::Vector3& normal) const;

        private:
            void SetReady();

            AZ::Name m_name;
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
            AZStd::fixed_vector<Data::Asset<ModelLodAsset>, ModelLodAsset::LodCountMax> m_lodAssets;

            // mutable method
            void BuildKdTree() const;
            bool BruteForceRayIntersect(const AZ::Vector3& rayStart, const AZ::Vector3& dir, float& distance, AZ::Vector3& normal) const;

            bool LocalRayIntersectionAgainstMesh(const ModelLodAsset::Mesh& mesh, const AZ::Vector3& rayStart, const AZ::Vector3& dir, float& distance, AZ::Vector3& normal) const;

            // Various model information used in raycasting
            AZ::Name m_positionName{ "POSITION" };
            // there is a tradeoff between memory use and performance but anywhere under a few thousand triangles or so remains under a few milliseconds per ray cast
            static const AZ::u32 s_minimumModelTriangleCountToOptimize = 100;
            mutable AZStd::unique_ptr<ModelKdTree> m_kdTree;
            volatile mutable bool m_isKdTreeCalculationRunning = false;
            mutable AZStd::mutex m_kdTreeLock;
            mutable AZStd::optional<AZStd::size_t> m_modelTriangleCount;

            AZStd::size_t CalculateTriangleCount() const;
        };

        class ModelAssetHandler : public AssetHandler<ModelAsset>
        {
        public:
            AZ_RTTI(ModelAssetHandler, "{993B8CE3-1BBF-4712-84A0-285DB9AE808F}", AssetHandler<ModelAsset>);
        };
    } //namespace RPI
} // namespace AZ
