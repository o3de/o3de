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

            //! Set the number of indices for the lod. Must be called before calling CreateIndexBuffer.
            void SetIndexCount(uint32_t indexCount);

            //! Set the number of vertices for the lod. Must be called before calling CreateSkinningInputBuffer or CreateStaticBuffer.
            void SetVertexCount(uint32_t vertexCount);

            //! Get the number of vertices for the lod
            uint32_t GetVertexCount() const;

            //! Set the index buffer asset
            void SetIndexBufferAsset(const Data::Asset<RPI::BufferAsset> bufferAsset);

            //! Create the index buffer. SetIndexCount must be called first as CreateIndexBuffer depends on that to know the number of indices to create.
            //! @param data The indices to be used for the index buffer. The index buffer is used by the target skinned model, but is not modified during skinning so it is shared between all instances of the same skinned mesh.
            //! @param bufferName Optional debug name for the buffer. A Uuid will automatically be appended to make it unique. If none is specified, a default name will be used.
            void CreateIndexBuffer(const uint32_t* data, const AZStd::string& bufferNamePrefix = "");

            //! Create the buffer for the specified inputStream. SetVertexCount must be called first as CreateSkinningInputBuffer depends on that to know the number of vertices to create.
            //! @param data The per-vertex data to use as input. The stride of data is expected to match the SkinnedMeshVertexStreamInfo::elementSize for the given input stream, and the size of the data buffer should be m_vertexCount * elementSize.
            //! @param inputStream The input stream this buffer is used for. SkinnedMeshInputVertexStreams are used for input to the skinning shader.
            //! @param bufferName Optional debug name for the buffer. A Uuid will automatically be appended to make it unique. If none is specified, a default name for the inputStream will be used.
            void CreateSkinningInputBuffer(void* data, SkinnedMeshInputVertexStreams inputStream, const AZStd::string& bufferNamePrefix = "");

            //! Create the buffer for the specified staticInputStream. SetVertexCount must be called first as CreateSkinningInputBuffer depends on that to know the number of vertices to create.
            //! @param data The per-vertex data to use as input. The stride of data is expected to match the SkinnedMeshVertexStreamInfo::elementSize for the given input stream, and the size of the data buffer should be m_vertexCount * elementSize.
            //! @param staticInputStream The static input stream this buffer is used for. SkinnedMeshStaticInputVertexStreams are used by the target skinned ModelLod, but are not modified during skinning so they are shared between all instances of the same skinned mesh.
            //! @param bufferName Optional debug name for the buffer. A Uuid will automatically be appended to make it unique. If none is specified, a default name for the staticInputStream will be used.
            void CreateStaticBuffer(void* data, SkinnedMeshStaticVertexStreams staticInputStream, const AZStd::string& bufferNamePrefix = "");

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

            //! Sets the input vertex stream from an existing buffer asset.
            void SetSkinningInputBufferAsset(const Data::Asset<RPI::BufferAsset> bufferAsset, SkinnedMeshInputVertexStreams inputStream);

            //! Sets the static vertex stream from an existing buffer asset.
            void SetStaticBufferAsset(const Data::Asset<RPI::BufferAsset> bufferAsset, SkinnedMeshStaticVertexStreams staticStream);

            //! Returns the BufferAsset of an input vertex stream.
            const Data::Asset<RPI::BufferAsset>& GetSkinningInputBufferAsset(SkinnedMeshInputVertexStreams stream) const;

            //! Calls RPI::Buffer::WaitForUpload for each buffer in the lod.
            void WaitForUpload();

        private:
            RHI::BufferViewDescriptor CreateInputViewDescriptor(
                SkinnedMeshInputVertexStreams inputStream, RHI::Format elementFormat, const RHI::StreamBufferView& streamBufferView);
            
            using HasInputStreamArray = AZStd::array<bool, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)>;
            HasInputStreamArray CreateInputBufferViews(
                uint32_t lodIndex,
                uint32_t meshIndex,
                const RHI::InputStreamLayout& inputLayout,
                const RPI::ModelLod::StreamBufferViewList& streamBufferViews,
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

            //! One BufferAsset for each input vertex stream
            AZStd::array<Data::Asset<RPI::BufferAsset>, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)> m_inputBufferAssets;
            //! One buffer for each input vertex stream
            AZStd::array<Data::Instance<RPI::Buffer>, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)> m_inputBuffers;
            //! One buffer view for each input vertex stream
            AZStd::array<RHI::Ptr<RHI::BufferView>, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)> m_bufferViews;
            
            //! Per-mesh data for the lod
            AZStd::vector<SkinnedSubMeshProperties> m_meshes;

            //! One BufferAsset for each static vertex stream. Not needed as input to the skinning shader, but used to create per-instance models as targets for skinning.
            AZStd::vector<Data::Asset<RPI::BufferAsset>> m_staticBufferAssets;

            //! One buffer for each static vertex stream. Not needed as input to the skinning shader, but used to create per-instance models as targets for skinning.
            AZStd::array<Data::Instance<RPI::Buffer>, static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::NumVertexStreams)> m_staticBuffers;

            //! One index buffer for the entire lod. Not needed as input to the skinning shader, but used to create per-instance models as targets for skinning.
            //! Effectively this is one of the SkinnedMeshStaticVertexStreams, but since it is an index buffer it is created slightly differently so it is a special case
            Data::Asset<RPI::BufferAsset> m_indexBufferAsset;
            Data::Instance<RPI::Buffer> m_indexBuffer;

            //! Container with one MorphTargetMetaData per morph target that can potentially be applied to an instance of this lod
            AZStd::vector<MorphTargetComputeMetaData> m_morphTargetComputeMetaDatas;

            //! Container with one MorphTargetInputBuffers per morph target that can potentially be applied to an instance of this lod
            AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>> m_morphTargetInputBuffers;

            //! Total number of indices for the entire lod
            uint32_t m_indexCount = 0;
            //! Total number of vertices for the entire lod
            uint32_t m_vertexCount = 0;

            SkinnedMeshOutputVertexCounts m_outputVertexCountsByStream;
        };

        //! Container for all the buffers and views needed for per-source model input to both the skinning shader and subsequent mesh shaders
        class SkinnedMeshInputBuffers
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(SkinnedMeshInputBuffers, AZ::SystemAllocator, 0);

            SkinnedMeshInputBuffers();
            ~SkinnedMeshInputBuffers();

            //! Create SkinnedMeshInputBuffers from a model
            void CreateFromModelAsset(const Data::Asset<RPI::ModelAsset>& modelAsset);

            //! Get the ModelAsset used to create the SkinnedMeshInputBuffers
            Data::Asset<RPI::ModelAsset> GetModelAsset() const;

            //! Get the number of meshes for the lod.
            uint32_t GetMeshCount(uint32_t lodIndex) const;

            //! Set the total number of lods for the SkinnedMeshInputBuffers
            void SetLodCount(size_t lodCount);

            //! Get the total number of lods
            uint32_t GetLodCount() const;

            //! Sets the SkinnedMeshInputLod at the specified index. SetLodCount must be called first.
            void SetLod(size_t lodIndex, const SkinnedMeshInputLod& lod);

            //! Get an individual lod
            const SkinnedMeshInputLod& GetLod(uint32_t lodIndex) const;

            //! Get a span of the buffer views for all the input streams
            AZStd::span<const AZ::RHI::Ptr<RHI::BufferView>> GetInputBufferViews(uint32_t lodIndex) const;

            //! Get the buffer view for a specific input stream
            AZ::RHI::Ptr<const RHI::BufferView> GetInputBufferView(uint32_t lodIndex, uint8_t inputStream) const;

            //! Get the number of vertices for the specified lod.
            uint32_t GetVertexCount(uint32_t lodIndex, uint32_t meshIndex) const;

            //! Set the buffer views and vertex count on the given SRG
            void SetBufferViewsOnShaderResourceGroup(uint32_t lodIndex, uint32_t meshIndex, const Data::Instance<RPI::ShaderResourceGroup>& perInstanceSRG);

            //! Create a model and resource views into the SkinnedMeshOutputBuffer that can be used as a target for the skinned vertices
            AZStd::intrusive_ptr<SkinnedMeshInstance> CreateSkinnedMeshInstance() const;

            //! Calls RPI::Buffer::WaitForUpload for each buffer
            void WaitForUpload();

            //! Returns true if WaitForUpload has finished, false if it has not been called.
            bool IsUploadPending() const;
            
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
            void CreateLod(size_t lodIndex);
            AZStd::fixed_vector<SkinnedMeshInputLod, RPI::ModelLodAsset::LodCountMax> m_lods;
            Data::Asset<RPI::ModelAsset> m_modelAsset;
            Data::Instance<RPI::Model> m_model;

            bool m_isUploadPending = false;
        };

        Data::Asset<RPI::BufferAsset> CreateBufferAsset(
            const void* data, const RHI::BufferViewDescriptor& viewDescriptor, RHI::BufferBindFlags bindFlags,
            Data::Asset<RPI::ResourcePoolAsset> resourcePoolAsset, const char* bufferName);
    }// Render
}// AZ
