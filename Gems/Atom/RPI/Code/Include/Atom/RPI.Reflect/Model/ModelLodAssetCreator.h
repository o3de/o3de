/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Reflect/AssetCreator.h>

namespace AZ
{
    namespace RPI
    {
        //! Constructs an instance of an ModelLodAsset.
        class ATOM_RPI_REFLECT_API ModelLodAssetCreator
            : public AssetCreator<ModelLodAsset>
        {
        public:
            ModelLodAssetCreator() = default;
            ~ModelLodAssetCreator() = default;

            //! Begins construction of a new ModelLodAsset instance. Resets the creator to a fresh state
            //! @param assetId The unique id to use when creating the asset
            void Begin(const Data::AssetId& assetId);

            //! Sets the lod-wide index buffer that can be referenced by subsequent meshes.
            //! @param bufferAsset The buffer asset to set as the lod's index buffer
            void SetLodIndexBuffer(const Data::Asset<BufferAsset>& bufferAsset);

            //! Adds an lod-wide stream buffer that can be referenced by subsequent meshes.
            //! @param bufferAsset The buffer asset to add as an lod-wide stream buffer
            void AddLodStreamBuffer(const Data::Asset<BufferAsset>& bufferAsset);

            //! Begins the addition of a Mesh to the ModelLodAsset. Begin must be called first.
            void BeginMesh();

            //! Sets the name of the current SubMesh.
            void SetMeshName(const AZ::Name& name);

            //! Sets the Aabb of the current SubMesh.
            //! Begin and BeginMesh must be called first.
            void SetMeshAabb(const AZ::Aabb& aabb);

            //! Sets the ID of the model's material slot that this mesh uses.
            //! Begin and BeginMesh must be called first
            void SetMeshMaterialSlot(ModelMaterialSlot::StableId id);

            //! Sets the given BufferAssetView to the current SubMesh as the index buffer.
            //! Begin and BeginMesh must be called first
            void SetMeshIndexBuffer(const BufferAssetView& bufferAssetView);

            //! Adds a BufferAssetView to the current SubMesh as a stream buffer that matches the given semantic name.
            //! Begin and BeginMesh must be called first
            bool AddMeshStreamBuffer(
                const RHI::ShaderSemantic& streamSemantic,
                const AZ::Name& customName,
                const BufferAssetView& bufferAssetView);

            //! Adds a StreamBufferInfo struct to the current SubMesh
            //! Begin and BeginMesh must be called first
            void AddMeshStreamBuffer(
                const ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo);

            //! Finalizes creation of the current Mesh and adds it to the current ModelLodAsset.
            void EndMesh();

            //! Finalizes the ModelLodAsset and assigns ownership of the asset to result if successful, otherwise returns false and result is left untouched.
            bool End(Data::Asset<ModelLodAsset>& result);

            //! Clone the given source model lod asset.
            //! @param sourceAsset The source model lod asset to clone.
            //! @param clonedResult The resulting, cloned model lod asset.
            //! @param inOutLastCreatedAssetId The asset id from the model asset that owns the cloned model lod asset. The sub id will be increased and
            //!                                used as the asset id for the cloned asset.
            //! @result True in case the asset got cloned successfully, false in case an error happened and the clone process got cancelled.
            static bool Clone(const Data::Asset<ModelLodAsset>& sourceAsset, Data::Asset<ModelLodAsset>& clonedResult, Data::AssetId& inOutLastCreatedAssetId);

        private:
            bool m_meshBegan = false;

            ModelLodAsset::Mesh m_currentMesh;
            bool ValidateIsMeshReady();
            bool ValidateIsMeshEnded();
            bool ValidateMesh(const ModelLodAsset::Mesh& mesh);
            bool ValidateLod();
        };
    } // namespace RPI
} // namespace AZ
