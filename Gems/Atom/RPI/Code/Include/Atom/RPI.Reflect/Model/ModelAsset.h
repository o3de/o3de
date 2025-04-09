/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        class ModelKdTree;

        //! Contains a set of RPI::ModelLodAsset objects.
        //! Serialized to a .azmodel file.
        //! Actual model data is stored in the BufferAssets referenced by ModelLodAssets.
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API ModelAsset
            : public AZ::Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class ModelAssetCreator;
            friend class ModelAssetHelpers;

        public:
            static constexpr const char* DisplayName{ "ModelAsset" };
            static constexpr const char* Group{ "Model" };
            static constexpr const char* Extension{ "azmodel" };

            AZ_RTTI(ModelAsset, "{2C7477B6-69C5-45BE-8163-BCD6A275B6D8}", AZ::Data::AssetData);
            AZ_CLASS_ALLOCATOR(ModelAsset, AZ::SystemAllocator);

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

            AZStd::span<const Data::Asset<ModelLodAsset>> GetLodAssets() const;

            // These two functions are used to keep references for BufferAssets so we can release them when they are done using after RPI::Model was created
            // Sometime they might need to be stay in memory for certain cpu operations such as ray intersection etc.

            //! Increase reference for an overall reference count for all BufferAssets referenced by this ModelAsset
            //! When the ref count was 0, increase ref count would trigger block loading for all the BufferAssets
            void AddRefBufferAssets();

            //! Reduce reference for an overall reference count for all BufferAssets referenced by this ModelAsset
            //! When the ref count reaches 0 after the reduce, it would release all the BufferAssets from the ModelAsset
            void ReleaseRefBufferAssets();

            //! Returns true if the ModelAsset contains data which is required by LocalRayIntersectionAgainstModel() function.
            bool SupportLocalRayIntersection() const;

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

            //! Returns the model tags
            const AZStd::vector<AZ::Name>& GetTags() const;

        private:
            //! Initialize the ModelAsset with the given set of data.
            //! This is used by ModelAssetHelpers to overwrite an already-created ModelAsset.
            //! @param name The name to associate with the model
            //! @param lodAssets The list of LodAssets to use with the model
            //! @param materialSlots The map of slots to materials for the model
            //! @param fallbackSlot The slot to use as a fallback material
            //! @param tags The set of tags to associate with the model.
            void InitData(
                AZ::Name name,
                AZStd::span<Data::Asset<ModelLodAsset>> lodAssets,
                const ModelMaterialSlotMap& materialSlots,
                const ModelMaterialSlot& fallbackSlot,
                AZStd::span<AZ::Name> tags);

            // AssetData overrides...
            bool HandleAutoReload() override
            {
                // Automatic asset reloads via the AssetManager are disabled for Atom models and their dependent assets because reloads
                // need to happen in a specific order to refresh correctly. They require more complex code than what the default
                // AssetManager reloading provides. See ModelReloader() for the actual handling of asset reloads.
                // Models need to be loaded via the MeshFeatureProcessor to reload correctly, and reloads can be listened
                // to by using MeshFeatureProcessor::ConnectModelChangeEventHandler().
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

            // load/release all the BufferAsset references by this ModelAsset's ModelLodAssets.
            void LoadBufferAssets();
            void ReleaseBufferAssets();

            // Various model information used in raycasting
            AZ::Name m_positionName{ "POSITION" };
            // there is a tradeoff between memory use and performance but anywhere under a few thousand triangles or so remains under a few milliseconds per ray cast
            static constexpr AZ::u32 s_minimumModelTriangleCountToOptimize = 100;
            mutable AZStd::unique_ptr<ModelKdTree> m_kdTree;
            volatile mutable bool m_isKdTreeCalculationRunning = false;
            mutable AZStd::mutex m_kdTreeLock;
            mutable AZStd::optional<AZStd::size_t> m_modelTriangleCount;

            // An overall reference count for all BufferAssets referenced by this ModelAsset
            // Set default to 1 since the ModelAsset would load all its BufferAssets by default.
            // ModelAsset would release these BufferAssets if this ref count reach 0 to save memory
            AZStd::atomic<size_t> m_bufferAssetsRef = 1;
            
            // Lists all of the material slots that are used by this LOD.
            // Note the same slot can appear in multiple LODs in the model, so that LODs don't have to refer back to the model asset.
            ModelMaterialSlotMap m_materialSlots;

            // A default ModelMaterialSlot to be returned upon error conditions.
            ModelMaterialSlot m_fallbackSlot;

            AZStd::size_t CalculateTriangleCount() const;

            AZStd::vector<AZ::Name> m_tags;
        };

        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API ModelAssetHandler
            : public AssetHandler<ModelAsset>
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING

        public:
            AZ_RTTI(ModelAssetHandler, "{993B8CE3-1BBF-4712-84A0-285DB9AE808F}", AssetHandler<ModelAsset>);

            //! Called when an asset requested to load is actually missing from the catalog when we are trying to resolve it
            //! from an ID to a file name and other streaming info.
            //! The AssetId that this returns should reference asset data to use as a fallback asset until the correct asset
            //! is compiled by the Asset Processor and loaded (or not, if it's a missing or failed asset).
            //! Missing assets don't support asset dependencies because they're substituted in at the asset stream load level,
            //! so the substitute asset must be standalone. All processed ModelAsset models have dependencies on LODs, buffers, and
            //! materials, so they can't be used as substitutes. Instead, this generates an in-memory unit cube model with no materials
            //! as a no-dependency asset that can be used until the real one appears.
            Data::AssetId AssetMissingInCatalog(const Data::Asset<Data::AssetData>& asset) override;

            Data::AssetHandler::LoadResult LoadAssetData(
                const AZ::Data::Asset<AZ::Data::AssetData>& asset,
                AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
                const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;

        private:
            static const Data::AssetId s_defaultModelAssetId;
        };
    } //namespace RPI
} // namespace AZ
