/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/SkinnedMesh/SkinnedMeshVertexStreams.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshInstance.h>
#include <Atom/Feature/MorphTargets/MorphTargetInputBuffers.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Reflect/Model/MorphTargetMetaAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RHI.Reflect/Base.h>
#include <AtomCore/Instance/InstanceData.h>

#include <AzCore/Math/Aabb.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    namespace RHI
    {
        class BufferView;
        class IndexBufferView;
    }

    namespace RPI
    {
        class Model;
        class ShaderResourceGroup;

    }

    namespace Render
    {
        //! All of the views needed for skinning input, skinning output, and static rendering of a single mesh
        struct SkinnedSubMeshProperties
        {
            struct SrgNameViewPair
            {
                Name m_srgName;
                RHI::Ptr<RHI::BufferView> m_bufferView;
            };

            //! Inputs to the skinning compute shader and their corresponding srg names
            AZStd::vector<SrgNameViewPair> m_inputBufferViews;

            //! Inputs that are not used or modified during skinning, but are used for rendering during the static mesh pipeline
            AZStd::vector<RPI::ModelLodAsset::Mesh::StreamBufferInfo> m_staticBufferInfo;

            //! Offset from the start of the stream in bytes for each output stream
            SkinnedMeshOutputVertexOffsets m_vertexOffsetsFromStreamStartInBytes;

            //! Number of vertices in this sub-mesh
            uint32_t m_vertexCount;

            //! Number of influences per vertex across the sub-mesh
            uint32_t m_skinInfluenceCountPerVertex;

            //! See Render::ComputeMorphTargetIntegerEncoding. A negative value indicates there are no morph targets that impact this mesh
            float m_morphTargetIntegerEncoding = -1.0f;
        };

        //! Container for all the buffers and views needed for a single lod of a skinned mesh
        class SkinnedMeshInputLod
        {
            friend class SkinnedMeshInputBuffers;
        public:
            //! Set all the input data for the skinned mesh lod from a model lod
            void CreateFromModelLod(
                const Data::Asset<RPI::ModelAsset>& modelAsset,
                const Data::Instance<RPI::Model>& model,
                uint32_t lodIndex);

            //! Get the ModelLodAsset that was used to create this lod
            Data::Asset<RPI::ModelLodAsset> GetModelLodAsset() const;

            //! Get the number of vertices for the lod
            uint32_t GetVertexCount() const;

            //! Add a single morph target that can be applied to an instance of this skinned mesh
            //! Creates a view into the larger morph target buffer to be used for applying an individual morph
            //! @param morphTarget The metadata that has info such as the min/max weight, offset, and vertex count for the morph
            //! @param morphBufferAssetView The view of all the morph target deltas that can be applied to this mesh
            //! @param bufferNamePrefix A prefix that can be used to identify this morph target when creating the view into the morph target buffer.
            //! @param minWeight The minimum weight that might be applied to this morph target. It's possible for the weight of a morph target to be outside the 0-1 range. Defaults to 0
            //! @param maxWeight The maximum weight that might be applied to this morph target. It's possible for the weight of a morph target to be outside the 0-1 range. 
            void AddMorphTarget(
                const RPI::MorphTargetMetaAsset::MorphTarget& morphTarget,
                const RPI::BufferAssetView* morphBufferAssetView,
                const AZStd::string& bufferNamePrefix,
                float minWeight,
                float maxWeight);

            //! Get the MetaDatas for all the morph targets that can be applied to an instance of this skinned mesh
            const AZStd::vector<MorphTargetComputeMetaData>& GetMorphTargetComputeMetaDatas() const;

            //! Get the MorphTargetInputBuffers for all the morph targets that can be applied to an instance of this skinned mesh
            const AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>>& GetMorphTargetInputBuffers() const;

            //! Check if there are any morph targets that can be applied to a particular sub-mesh
            bool HasMorphTargetsForMesh(uint32_t meshIndex) const;

        private:
            RHI::BufferViewDescriptor CreateInputViewDescriptor(
                SkinnedMeshInputVertexStreams inputStream, RHI::Format elementFormat, const RHI::StreamBufferView& streamBufferView);
            
            using HasInputStreamArray = AZStd::array<bool, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)>;
            HasInputStreamArray CreateInputBufferViews(
                uint32_t lodIndex,
                uint32_t meshIndex,
                const RHI::InputStreamLayout& inputLayout,
                RPI::ModelLod::Mesh& mesh,
                const RHI::StreamBufferIndices& streamIndices,
                const char* modelName);

            void CreateOutputOffsets(
                uint32_t meshIndex,
                const HasInputStreamArray& meshHasInputStream,
                SkinnedMeshOutputVertexOffsets& currentMeshOffsetFromStreamStart);

            void TrackStaticBufferViews(uint32_t meshIndex);
            
            //! After all morph targets have been added, determine the integer encoding for each mesh.
            void CalculateMorphTargetIntegerEncodings();

            //! The lod asset from the underlying mesh
            Data::Asset<RPI::ModelLodAsset> m_modelLodAsset;
            Data::Instance<RPI::ModelLod> m_modelLod;
            
            //! Per-mesh data for the lod
            AZStd::vector<SkinnedSubMeshProperties> m_meshes;

            //! One BufferAsset for each static vertex stream. Not needed as input to the skinning shader, but used to create per-instance models as targets for skinning.
            AZStd::vector<Data::Asset<RPI::BufferAsset>> m_staticBufferAssets;

            //! Container with one MorphTargetMetaData per morph target that can potentially be applied to an instance of this lod
            AZStd::vector<MorphTargetComputeMetaData> m_morphTargetComputeMetaDatas;

            //! Container with one MorphTargetInputBuffers per morph target that can potentially be applied to an instance of this lod
            AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>> m_morphTargetInputBuffers;

            SkinnedMeshOutputVertexCounts m_outputVertexCountsByStream;
        };

        //! Container for all the buffers and views needed for per-source model input to both the skinning shader and subsequent mesh shaders
        class SkinnedMeshInputBuffers
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(SkinnedMeshInputBuffers, AZ::SystemAllocator);

            SkinnedMeshInputBuffers();
            ~SkinnedMeshInputBuffers();

            //! Create SkinnedMeshInputBuffers from a model
            void CreateFromModelAsset(const Data::Asset<RPI::ModelAsset>& modelAsset);

            //! Get the ModelAsset used to create the SkinnedMeshInputBuffers
            Data::Asset<RPI::ModelAsset> GetModelAsset() const;

            //! Get the Model used to as input to the skinning compute shader
            Data::Instance<RPI::Model> GetModel() const;

            //! Get the number of meshes for the lod.
            uint32_t GetMeshCount(uint32_t lodIndex) const;

            //! Get the total number of lods
            uint32_t GetLodCount() const;

            //! Get an individual lod
            const SkinnedMeshInputLod& GetLod(uint32_t lodIndex) const;

            //! Get the number of vertices for the specified lod.
            uint32_t GetVertexCount(uint32_t lodIndex, uint32_t meshIndex) const;

            //! Set the buffer views and vertex count on the given SRG
            void SetBufferViewsOnShaderResourceGroup(uint32_t lodIndex, uint32_t meshIndex, const Data::Instance<RPI::ShaderResourceGroup>& perInstanceSRG);

            //! Create a model and resource views into the SkinnedMeshOutputBuffer that can be used as a target for the skinned vertices
            AZStd::intrusive_ptr<SkinnedMeshInstance> CreateSkinnedMeshInstance() const;
            
            //! Returns the number of influences per vertex for a mesh
            uint32_t GetInfluenceCountPerVertex(uint32_t lodIndex, uint32_t meshIndex) const;
            
            //! Returns a vector of MorphTargetMetaData with one entry for each morph target that could be applied to this mesh
            const AZStd::vector<MorphTargetComputeMetaData>& GetMorphTargetComputeMetaDatas(uint32_t lodIndex) const;

            //! Returns a vector of MorphTargetInputBuffers which serve as input to the morph target pass
            const AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>>& GetMorphTargetInputBuffers(uint32_t lodIndex) const;

            //! Return the integer encoding used for the morph targets for a given lod/mesh, or -1 if there are no morph targets for the mesh.
            //! If the values are not yet pre-calculated, they will be when calling this function
            float GetMorphTargetIntegerEncoding(uint32_t lodIndex, uint32_t meshIndex) const;

            //! Add a single morph target that can be applied to an instance of this skinned mesh
            //! Creates a view into the larger morph target buffer to be used for applying an individual morph
            //! Must call Finalize after all morph targets have been added
            //! @param lodIndex The index of the lod modified by the morph target
            //! @param morphTarget The metadata that has info such as the min/max weight, offset, and vertex count for the morph
            //! @param morphBufferAssetView The view of all the morph target deltas that can be applied to this mesh
            //! @param bufferNamePrefix A prefix that can be used to identify this morph target when creating the view into the morph target buffer.
            //! @param minWeight The minimum weight that might be applied to this morph target. It's possible for the weight of a morph target to be outside the 0-1 range. Defaults to 0
            //! @param maxWeight The maximum weight that might be applied to this morph target. It's possible for the weight of a morph target to be outside the 0-1 range.
            void AddMorphTarget(
                uint32_t lodIndex,
                const RPI::MorphTargetMetaAsset::MorphTarget& morphTarget,
                const RPI::BufferAssetView* morphBufferAssetView,
                const AZStd::string& bufferNamePrefix,
                float minWeight,
                float maxWeight);

            //! Do any internal calculations that must be done after the input buffers are created from the model
            //! and after all morph targets have been added
            void Finalize();
        private:
            AZStd::fixed_vector<SkinnedMeshInputLod, RPI::ModelLodAsset::LodCountMax> m_lods;
            Data::Asset<RPI::ModelAsset> m_modelAsset;
            Data::Instance<RPI::Model> m_model;
        };
    }// Render
}// AZ
