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

#include <Atom/Feature/SkinnedMesh/SkinnedMeshVertexStreams.h>
#include <Atom/Feature/SkinnedMesh/SkinnedMeshInstance.h>
#include <Atom/Feature/MorphTargets/MorphTargetInputBuffers.h>
#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
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
        //! Info needed to create per-submesh views into the skinned mesh buffers so that the target skinned model can be broken into multiple sub-meshes.
        //! These properties should be set explicitly when creating a SkinnedMeshInputLod
        struct SkinnedSubMeshProperties
        {
            uint32_t m_indexOffset = 0;
            uint32_t m_indexCount = 0;
            uint32_t m_vertexOffset = 0;
            uint32_t m_vertexCount = 0;
            Aabb m_aabb = Aabb::CreateNull();
            Data::Asset<RPI::MaterialAsset> m_material;
        };

        //! Buffer views for a specific sub-mesh that are not modified during skinning and thus are shared by all instances of the same skinned mesh
        //! These views are created internally by the SkinnedMeshInputLod.
        struct SkinnedSubMeshSharedViews
        {
            RPI::BufferAssetView m_indexBufferView;
            AZStd::array < RPI::BufferAssetView, static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::NumVertexStreams)> m_staticStreamViews;
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
            //! Set the number of indices for the lod. Must be called before calling CreateIndexBuffer.
            void SetIndexCount(uint32_t indexCount);

            //! Set the number of vertices for the lod. Must be called before calling CreateSkinningInputBuffer or CreateStaticBuffer.
            void SetVertexCount(uint32_t vertexCount);

            //! Get the number of vertices for the lod
            uint32_t GetVertexCount() const;

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

            //! Set the properties used by each sub-mesh of a target skinned ModelLod and create per-sub-mesh views of the index and static buffers.
            //! Since this function is creating buffer views, it must be called after CreateIndexBuffer and CreateStaticBuffer.
            void SetSubMeshProperties(const AZStd::vector<SkinnedSubMeshProperties>& subMeshProperties);

            //! Get the properties of each submesh
            const AZStd::vector<SkinnedSubMeshProperties>& GetSubMeshProperties() const;

            //! Add a single morph target that can be applied to an instance of this skinned mesh
            //! @param minWeight The minimum weight. Used in combination with the min/max deltas to determine the maximum value this morph could possibly offset a vertex.
            //! @param maxWeight The maximum weight. Used in combination with the min/max deltas to determine the maximum value this morph could possibly offset a vertex.
            //! @param minDelta The minimum value of all the deltas. Used for decoding the deltas.
            //! @param maxDelta The maximum value of all the deltas. Used for decoding the deltas.
            //! @param vertexCount The total number of vertices that are deformed.
            //! @param vertexNumbers The indices of the vertices that are deformed, since the morph target can deform a sparse set of vertices.
            //! @param deltas The encoded deltas, stored as a raw buffer of 4 byte components.
            //! @param bufferNamePrefix A prefix that can be used to identify this morph target when creating names for the buffers.
            void AddMorphTarget(float minWeight, float maxWeight, float minDelta, float maxDelta, uint32_t vertexCount, const AZStd::vector<uint32_t>& vertexNumbers, const AZStd::vector<uint32_t>& deltas, const AZStd::string& bufferNamePrefix);

            //! Get the MetaDatas for all the morph targets that can be applied to an instance of this skinned mesh
            const AZStd::vector<MorphTargetMetaData>& GetMorphTargetMetaDatas() const;

            //! Get the MorphTargetInputBuffers for all the morph targets that can be applied to an instance of this skinned mesh
            const AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>>& GetMorphTargetInputBuffers() const;
            
            //! Returns the BufferAsset of an input vertex stream.
            const Data::Asset<RPI::BufferAsset>& GetSkinningInputBufferAsset(SkinnedMeshInputVertexStreams stream) const;

            //! Calls RPI::Buffer::WaitForUpload for each buffer in the lod.
            void WaitForUpload();

        private:

            void CreateSharedSubMeshBufferViews();

            //! One BufferAsset for each input vertex stream
            AZStd::array<Data::Asset<RPI::BufferAsset>, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)> m_inputBufferAssets;
            //! One buffer for each input vertex stream
            AZStd::array<Data::Instance<RPI::Buffer>, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)> m_inputBuffers;
            //! One buffer view for each input vertex stream
            AZStd::array<RHI::Ptr<RHI::BufferView>, static_cast<uint8_t>(SkinnedMeshInputVertexStreams::NumVertexStreams)> m_bufferViews;

            //! One BufferAsset for each static vertex stream. Not needed as input to the skinning shader, but used to create per-instance models as targets for skinning.
            AZStd::array<Data::Asset<RPI::BufferAsset>, static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::NumVertexStreams)> m_staticBufferAssets;
            //! One buffer for each static vertex stream. Not needed as input to the skinning shader, but used to create per-instance models as targets for skinning.
            AZStd::array<Data::Instance<RPI::Buffer>, static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::NumVertexStreams)> m_staticBuffers;

            //! One index buffer for the entire lod. Not needed as input to the skinning shader, but used to create per-instance models as targets for skinning.
            //! Effectively this is one of the SkinnedMeshStaticVertexStreams, but since it is an index buffer it is created slightly differently so it is a special case
            Data::Asset<RPI::BufferAsset> m_indexBufferAsset;
            Data::Instance<RPI::Buffer> m_indexBuffer;

            //! Structure of the sub-meshes within the lod. Stores all the sub-mesh info needed to create per-instance models as targets for skinning
            AZStd::vector<SkinnedSubMeshProperties> m_subMeshProperties;

            //! Stores the actual per-sub-mesh views that are not modified during skinning and thus shared between all instances of the same skinned mesh
            AZStd::vector<SkinnedSubMeshSharedViews> m_sharedSubMeshViews;

            //! Container with one MorphTargetMetaData per morph target that can potentially be applied to an instance of this skinned mesh
            AZStd::vector<MorphTargetMetaData> m_morphTargetMetaDatas;

            //! Container with one MorphTargetInputBuffers per morph target that can potentially be applied to an instance of this skinned mesh
            AZStd::vector<AZStd::intrusive_ptr<MorphTargetInputBuffers>> m_morphTargetInputBuffers;

            //! Total number of indices for the entire lod
            uint32_t m_indexCount = 0;
            //! Total number of vertices for the entire lod
            uint32_t m_vertexCount = 0;
        };

        //! Container for all the buffers and views needed for per-source model input to both the skinning shader and subsequent mesh shaders
        class SkinnedMeshInputBuffers
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(SkinnedMeshInputBuffers, AZ::SystemAllocator, 0);

            SkinnedMeshInputBuffers();
            ~SkinnedMeshInputBuffers();

            //! Keep track of the assetId of the asset used to create the SkinnedMeshInputBuffers
            void SetAssetId(Data::AssetId assetId);

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

            //! Get the number of vertices for the specified lod.
            uint32_t GetVertexCount(size_t lodIndex) const;

            //! Set the buffer views and vertex count on the given SRG
            void SetBufferViewsOnShaderResourceGroup(size_t lodIndex, const Data::Instance<RPI::ShaderResourceGroup>& perInstanceSRG);

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
            Data::AssetId m_assetId;

            bool m_isUploadPending = false;
        };

        Data::Asset<RPI::BufferAsset> CreateBufferAsset(
            const void* data, const RHI::BufferViewDescriptor& viewDescriptor, RHI::BufferBindFlags bindFlags,
            Data::Asset<RPI::ResourcePoolAsset> resourcePoolAsset, const char* bufferName);
    }// Render
}// AZ
