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
        struct SkinnedSubMeshProperties
        {
            AZStd::vector< AZStd::tuple<Name, RHI::Ptr<RHI::BufferView>>> m_inputBufferViews;

            AZStd::vector<RPI::ModelLodAsset::Mesh::StreamBufferInfo> m_staticBufferInfo;

            //! Offset from the start of the stream in bytes for each output stream
            SkinnedMeshOutputVertexOffsets m_vertexOffsetsFromStreamStartInBytes;
            uint32_t m_vertexCount;
            uint32_t m_skinInfluenceCountPerVertex;
        };
        //! Container for all the buffers and views needed for a single lod of a skinned mesh
        //! To create a SkinnedMeshInputLod, follow the general pattern
        //!     lod.SetIndexCount()
        //!     lod.SetVertexCount()
        //!     lod.CreateIndexBuffer()
        //!     for each input buffer
        //!         lod.CreateSkinningInputBuffer()
        //!     for each static buffer
        //!         lod.CreateStaticBuffer()
        //!     lod.SetSubMeshProperties()
        class SkinnedMeshInputLod
        {
            friend class SkinnedMeshInputBuffers;
        public:
            //! Set all the input data for the skinned mesh lod from a model lod
            void CreateFromModelLodAsset(const Data::Asset<RPI::ModelLodAsset>& modelLodAsset);

            //! Get the ModelLodAsset that was used to create this lod
            Data::Asset<RPI::ModelLodAsset> GetModelLodAsset() const;

            //! Set the number of indices for the lod. Must be called before calling CreateIndexBuffer.
            void SetIndexCount(uint32_t indexCount);

            //! Set the number of vertices for the lod. Must be called before calling CreateSkinningInputBuffer or CreateStaticBuffer.
            void SetVertexCount(uint32_t vertexCount);

            //! Get the number of vertices for the mesh
            uint32_t GetVertexCountForStream(SkinnedMeshOutputVertexStreams outputStream) const;

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
            //! @param metaAsset The metadata that has info such as the min/max weight, offset, and vertex count for the morph
            //! @param morphBufferAsset The the combined buffer that has all the deltas for all morph targets in the model lod
            //! @param bufferNamePrefix A prefix that can be used to identify this morph target when creating the view into the morph target buffer.
            //! @param minWeight The minimum weight that might be applied to this morph target. It's possible for the weight of a morph target to be outside the 0-1 range. Defaults to 0
            //! @param maxWeight The maximum weight that might be applied to this morph target. It's possible for the weight of a morph target to be outside the 0-1 range. Defaults to 1
            void AddMorphTarget(const RPI::MorphTargetMetaAsset::MorphTarget& metaAsset, const Data::Asset<RPI::BufferAsset>& morphBufferAsset, const AZStd::string& bufferNamePrefix, float minWeight, float maxWeight);

            //! Get the MetaDatas for all the morph targets that can be applied to an instance of this skinned mesh
            const AZStd::vector<MorphTargetMetaData>& GetMorphTargetMetaDatas() const;

            //! Get the MorphTargetInputBuffers for all the morph targets that can be applied to an instance of this skinned mesh
            const AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>>& GetMorphTargetInputBuffers() const;

            //! Sets the input vertex stream from an existing buffer asset.
            void SetSkinningInputBufferAsset(const Data::Asset<RPI::BufferAsset> bufferAsset, SkinnedMeshInputVertexStreams inputStream);

            //! Sets the static vertex stream from an existing buffer asset.
            void SetStaticBufferAsset(const Data::Asset<RPI::BufferAsset> bufferAsset, SkinnedMeshStaticVertexStreams staticStream);

            //! Returns the BufferAsset of an input vertex stream.
            const Data::Asset<RPI::BufferAsset>& GetSkinningInputBufferAsset(SkinnedMeshInputVertexStreams stream) const;

            //! Calls RPI::Buffer::WaitForUpload for each buffer in the lod.
            void WaitForUpload();

            //! Returns true if this lod has a morphed color stream
            bool HasDynamicColors() const;

        private:

            //! The lod asset from the underlying mesh
            Data::Asset<RPI::ModelLodAsset> m_modelLodAsset;

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

            //! Container with one MorphTargetMetaData per morph target that can potentially be applied to an instance of this skinned mesh
            AZStd::vector<MorphTargetMetaData> m_morphTargetMetaDatas;

            //! Container with one MorphTargetInputBuffers per morph target that can potentially be applied to an instance of this skinned mesh
            AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>> m_morphTargetInputBuffers;

            //! Total number of indices for the entire lod
            uint32_t m_indexCount = 0;
            //! Total number of vertices for the entire lod
            uint32_t m_vertexCount = 0;

            //! Bool for keeping track of whether or not this lod has morphed colors
            bool m_hasDynamicColors = false;
            //! Bool for keeping track of whether or not this lod has a static color stream
            bool m_hasStaticColors = false;

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
            size_t GetMeshCount(size_t lodIndex) const;

            //! Set the total number of lods for the SkinnedMeshInputBuffers
            void SetLodCount(size_t lodCount);

            //! Get the total number of lods
            size_t GetLodCount() const;

            //! Sets the SkinnedMeshInputLod at the specified index. SetLodCount must be called first.
            void SetLod(size_t lodIndex, const SkinnedMeshInputLod& lod);

            //! Get an individual lod
            const SkinnedMeshInputLod& GetLod(size_t lodIndex) const;

            //! Get an array_view of the buffer views for all the input streams
            AZStd::array_view<AZ::RHI::Ptr<RHI::BufferView>> GetInputBufferViews(size_t lodIndex) const;

            //! Get the buffer view for a specific input stream
            AZ::RHI::Ptr<const RHI::BufferView> GetInputBufferView(size_t lodIndex, uint8_t inputStream) const;

            //! Check if the mesh has dynamically modified colors
            bool HasDynamicColors(size_t lodIndex, size_t meshIndex) const;

            //! Get the number of vertices for the specified lod.
            uint32_t GetVertexCount(size_t lodIndex, size_t meshIndex) const;

            //! Set the buffer views and vertex count on the given SRG
            void SetBufferViewsOnShaderResourceGroup(size_t lodIndex, size_t meshIndex, const Data::Instance<RPI::ShaderResourceGroup>& perInstanceSRG);

            //! Create a model and resource views into the SkinnedMeshOutputBuffer that can be used as a target for the skinned vertices
            AZStd::intrusive_ptr<SkinnedMeshInstance> CreateSkinnedMeshInstance() const;

            //! Calls RPI::Buffer::WaitForUpload for each buffer
            void WaitForUpload();

            //! Returns true if WaitForUpload has finished, false if it has not been called.
            bool IsUploadPending() const;

            //! Returns a vector of MorphTargetMetaData with one entry for each morph target that could be applied to this mesh
            const AZStd::vector<MorphTargetMetaData>& GetMorphTargetMetaData(size_t lodIndex) const { return m_lods[lodIndex].GetMorphTargetMetaDatas(); }

            //! Returns a vector of MorphTargetInputBuffers which serve as input to the morph target pass
            const AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>>& GetMorphTargetInputBuffers(size_t lodIndex) const { return m_lods[lodIndex].GetMorphTargetInputBuffers(); }

        private:
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
