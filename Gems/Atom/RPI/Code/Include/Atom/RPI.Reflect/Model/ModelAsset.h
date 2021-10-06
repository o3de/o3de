/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            
            //! Returns the list of all ModelMaterialSlot's for the model, across all LODs.
            const ModelMaterialSlotMap& GetMaterialSlots() const;
            
            //! Find a material slot with the given stableId, or returns an invalid slot if it isn't found.
            const ModelMaterialSlot& FindMaterialSlot(uint32_t stableId) const;

            //! Returns the number of Lods in the model
            size_t GetLodCount() const;

            AZStd::array_view<Data::Asset<ModelLodAsset>> GetLodAssets() const;

            //! Checks a ray for intersection against this model. The ray must be in the same coordinate space as the model.
            //! Important: only to be used in the Editor, it may kick off a job to calculate spatial information.
            //! [GFX TODO][ATOM-4343 Bake mesh spatial information during AP processing]
            //!
            //! @param rayStart  The starting point of the ray.
            //! @param rayDir  The direction and length of the ray (magnitude is encoded in the direction).
            //! @param allowBruteForce  Allow for brute force queries while the mesh is baking (remove when ATOM-4343 is complete)
            //! @param[out] distanceNormalized  If an intersection is found, will be set to the normalized distance of the intersection
            //! (in the range 0.0-1.0) - to calculate the actual distance, multiply distanceNormalized by the magnitude of rayDir.
            //! @param[out] normal If an intersection is found, will be set to the normal at the point of collision.
            //! @return  True if the ray intersects the mesh.
            virtual bool LocalRayIntersectionAgainstModel(
                const AZ::Vector3& rayStart, const AZ::Vector3& rayDir, bool allowBruteForce,
                float& distanceNormalized, AZ::Vector3& normal) const;

        private:
            // AssetData overrides...
            bool HandleAutoReload() override
            {
                return false;
            }

            void SetReady();

            AZ::Name m_name;
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();
            AZStd::fixed_vector<Data::Asset<ModelLodAsset>, ModelLodAsset::LodCountMax> m_lodAssets;

            // mutable method
            void BuildKdTree() const;
            bool BruteForceRayIntersect(
                const AZ::Vector3& rayStart, const AZ::Vector3& rayDir, float& distanceNormalized, AZ::Vector3& normal) const;

            bool LocalRayIntersectionAgainstMesh(
                const ModelLodAsset::Mesh& mesh,
                const AZ::Vector3& rayStart,
                const AZ::Vector3& rayDir,
                float& distanceNormalized,
                AZ::Vector3& normal) const;

            // Various model information used in raycasting
            AZ::Name m_positionName{ "POSITION" };
            // there is a tradeoff between memory use and performance but anywhere under a few thousand triangles or so remains under a few milliseconds per ray cast
            static const AZ::u32 s_minimumModelTriangleCountToOptimize = 100;
            mutable AZStd::unique_ptr<ModelKdTree> m_kdTree;
            volatile mutable bool m_isKdTreeCalculationRunning = false;
            mutable AZStd::mutex m_kdTreeLock;
            mutable AZStd::optional<AZStd::size_t> m_modelTriangleCount;
            
            // Lists all of the material slots that are used by this LOD.
            // Note the same slot can appear in multiple LODs in the model, so that LODs don't have to refer back to the model asset.
            ModelMaterialSlotMap m_materialSlots;

            // A default ModelMaterialSlot to be returned upon error conditions.
            ModelMaterialSlot m_fallbackSlot;

            AZStd::size_t CalculateTriangleCount() const;
        };

        class ModelAssetHandler
            : public AssetHandler<ModelAsset>
        {
        public:
            AZ_RTTI(ModelAssetHandler, "{993B8CE3-1BBF-4712-84A0-285DB9AE808F}", AssetHandler<ModelAsset>);

            // AZ::AssetTypeInfoBus::Handler overrides
            bool HasConflictingProducts(const AZStd::vector<AZ::Data::AssetType>& productAssetTypes) const override;
        };
    } //namespace RPI
} // namespace AZ
