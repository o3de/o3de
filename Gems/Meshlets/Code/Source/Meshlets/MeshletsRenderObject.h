#pragma once

#include <AzCore/base.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/list.h>

#include <AtomCore/Instance/Instance.h>
#include <AtomCore/Instance/InstanceData.h>

#include <Atom/RHI/ImagePool.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include "../../External/meshoptimizer.h"
//#include <meshoptimizer.h>

#include <SharedBuffer.h>
#include <MeshletsDispatchItem.h>

namespace AZ
{
    namespace RPI
    {
        class MeshDrawPacket;
    }

    namespace Meshlets
    {
        enum class ComputeStreamsSemantics : uint8_t
        {
            Indices = 0,
            MeshletsData,
            MehsletsTriangles,
            MeshletsIndicesIndirection,
            NumBufferStreams
        };

        enum class RenderStreamsSemantics : uint8_t
        {
            Indices = 0,
            Positions,
            Normals,
            UVs,
            Tangents,
            BiTangents,
            NumBufferStreams
        };

        // All meshlets should be assigned using a single buffer with multiple views
        // So that the assignment to the GPU will be fast.
        // This can be done as a second stage.


        struct MeshletDescriptor
        {
            //! Offset into the indirect indices array representing the global index of
            //! all the meshlet vertices.
            //! The Indirect vertices array is built as follows:
            //!     std::vector<uint32_t> indirectIndices;
            //!     indirectIndices = { { meshlet 1 vertex indices }, { meshlet 2 }, .. { meshlet n} }
            uint32_t vertexOffset;      // In uint32_t steps

            //! Offset into the global meshlets triangleIndices array represented as:
            //!     std::vector<uint8_t> triangleIndices;
            //!     triangleIndices = { {meshlet 1 local indices group}, ... { meshlet n} }
            //! The local indices are an 8 bits index that can represent up to 256 entries.
            uint32_t triangleOffset;    // In bytes from the start of the array

            //! Finding a vertex within the meshlet is done like that:
            //!     triangleOffset = currentMeshlet.triangleOffset + meshletTrIndex * 3;
            //!     localIndex_i = meshletTriangles[triangleOffset + i];    // i = triangle vertex index 0..2
            //!     vertexIndex_i =  indirectIndices[currentMeshlet.vertexOffset + localIndex_i];

            //! Amount of vertices and triangle for the mesh - based on this the arrays
            //! indirectIndices and triangleIndices are created per meshlet.
            uint32_t vertexCount;
            uint32_t triangleCount;
        };

        struct GeneratorVertex
        {
            float px, py, pz;
            float nx, ny, nz;
            float tx, ty;
        };

        struct GeneratorMesh
        {
            std::vector<GeneratorVertex> vertices;
            std::vector<unsigned int> indices;
        };

        struct MeshletsData
        {
            std::vector<meshopt_Meshlet> meshlets;
            std::vector<unsigned int> meshlet_vertices;		// Vertex Index indirection map
            std::vector<unsigned char> meshlet_triangles;	// Meshlet triangles into the vertex index indirection - local to meshlet.
        };

        //! Currently assuming single model without Lods so that the handling of the
        //! meshlet creation and handling of the array is easier. If several meshes or Lods
        //! exist, they will be created as separate models and the last model's instance
        //! will be kept in this class.
        //! To enhance this, add inheritance to lower levels of the model / mesh.
        //! MeshletsModel represents a combined model that can contain an array
        //! of ModelLods.
        //! Each one of the ModelLods contains a vector of meshes, representing possible multiple
        //! element within the mesh.
        class MeshletsRenderObject
        {
        public:
            static uint32_t s_modelNumber;

            MeshletsRenderObject(Data::Asset<RPI::ModelAsset> sourceModelAsset);
            ~MeshletsRenderObject();

            static  Data::Instance<RPI::ShaderResourceGroup> CreateShaderResourceGroup(
                Data::Instance<RPI::Shader> shader,
                const char* shaderResourceGroupId,
                [[maybe_unused]] const char* moduleName
            );

            Data::Instance<RPI::Model> GetMeshletsModel() { return m_meshletsModel; }

            AZStd::string& GetName() { return m_name;  }

        protected:
            bool ProcessBuffersData(float* position, uint32_t vtxNum);

            uint8_t* RetrieveBufferData(
                const RPI::BufferAssetView* bufferView,
                RHI::Format& format,
                uint32_t expectedAmount, uint32_t& existingAmount,
                RHI::BufferViewDescriptor& bufferDesc
            );

            uint32_t CreateMeshlets(GeneratorMesh& mesh);

            uint32_t CreateMeshlets(
                float* positions, float* normals, float* texCoords, uint32_t vtxNum,
                uint16_t* indices, uint32_t idxNum, RHI::Format IndexStreamFormat
            );

            uint32_t CreateMeshletsFromModelAsset(Data::Asset<RPI::ModelAsset> sourceModelAsset);

            uint32_t CreateMeshletsRenderObject(const RPI::ModelLodAsset::Mesh& meshAsset);

            uint32_t GetMehsletsCount() { m_meshletsCount;  }

            void PrepareRenderSrgDescriptors(uint32_t vertexCount, uint32_t indicesCount);
            //! Should be called by CreateAndBindRenderBuffers
            bool CreateAndBindRenderSrg();
            bool CreateAndBindRenderBuffers();

            void PrepareComputeSrgDescriptors(uint32_t meshletsCount, uint32_t indicesCount);
            //! Should be called by CreateAndBindComputeBuffers.
            bool CreateAndBindComputeSrg();
            bool CreateAndBindComputeBuffers();

        private:
            AZStd::string m_name;

            Data::Instance<RPI::Shader> m_meshletsDataPrepComputeShader;

            Aabb m_aabb;    // [Adi] should be per Lod per mesh and not global

            static AZStd::vector<SrgBufferDescriptor> m_srgBufferDescriptors;

            Data::Asset<RPI::ModelAsset> m_sourceModelAsset;

            Data::Instance<RPI::Model> m_meshletsModel;

            // [Adi] - meshlets data should be a vector of meshlets data per lod per mesh
            MeshletsData m_meshletsData;    // the actual mesh meshlets' data

            uint32_t m_meshletsCount = 0;

            using ModelLodDispatchMap = AZStd::unordered_map<RPI::ModelLodAsset::Mesh*, Data::Instance<MeshletsDispatchItem>>;
            using ModelLodDrawPacketMap = AZStd::unordered_map<RPI::ModelLodAsset::Mesh*, RPI::MeshDrawPacket>;

            //! Lod entries dispatch items map - each Lod entry represents a map of dispatches used
            //! for Compute per the different meshes existing within this Lod
            AZStd::unordered_map<Data::Instance<RPI::ModelLod>, ModelLodDispatchMap> m_dispatchItems;

            //! Lod entries draw packets map - each Lod entry represents a map of draw packets for
            //! geometry render per the different meshes existing within this Lod
            AZStd::unordered_map<Data::Instance<RPI::ModelLod>, ModelLodDispatchMap> m_drawPackets;

            //! Render pass data
            Data::Instance<RPI::Shader> m_renderShader;
            AZStd::vector<Data::Instance<RHI::BufferView>> m_renderBuffersViews;
            AZStd::vector<Data::Instance<Meshlets::SharedBufferAllocation>> m_renderBuffersAllocators;
            AZStd::vector<SrgBufferDescriptor> m_renderBuffersDescriptors;
            Data::Instance<RPI::ShaderResourceGroup> m_renderSrg;

            //! Compute render data
            Data::Instance<RPI::Shader> m_computeShader;
            AZStd::vector<Data::Instance<RHI::BufferView>> m_computeBuffersViews;
            AZStd::vector<Data::Instance<Meshlets::SharedBufferAllocation>> m_computeBuffersAllocators;
            AZStd::vector<SrgBufferDescriptor> m_computeBuffersDescriptors;
            Data::Instance<RPI::ShaderResourceGroup> m_computeSrg;

            MeshletsDispatchItem m_dispatchItem;
        };

    } // namespace Meshlets
} // namespace AZ
