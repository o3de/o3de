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


#include "meshoptimizer.h"
//#include "../../External/Include/meshoptimizer.h"
//#include <meshoptimizer.h>
//#include <Mesh/EditorMeshComponent.h>

namespace AZ
{
    namespace RPI
    {
        class MeshDrawPacket;
    }

    namespace Meshlets
    {
        //======================================================================
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
            //!     localIndex_0 = meshletTriangles[triangleOffset + 0];
            //!     localIndex_1 = meshletTriangles[triangleOffset + 1];
            //!     localIndex_2 = meshletTriangles[triangleOffset + 2];
            //!     vertexIndex_0 =  indirectIndices[currentMeshlet.vertexOffset + localIndex_0];

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

//            bool Create();
//            bool ExportToOBJ(const char* path);
        };

        //! Currently assuming single model without Lods so that the handling of the
        //! meshlet creation and handling of the array is easier. If several meshes or Lods
        //! exist, they will be created as separate models and the last model's instance
        //! will be kept in this class.
        //! To enhance this, add inheritance to lower levels of the model / mesh.
        //! MeshletModel represents a combined model that can contain an array
        //! of ModelLods.
        //! Each one of the ModelLods contains a vector of meshes, representing possible multiple
        //! element within the mesh.
        class MeshletModel
        {
        public:
            static uint32_t s_modelNumber;

            Name IndicesSemanticName;

            Name PositionSemanticName;
            Name NormalSemanticName;
            Name TangentSemanticName;
            Name BiTangentSemanticName;
            Name UVSemanticName;

            Name MeshletsDescriptorsName;
            Name MeshletsVertexIndicesName;
            Name MeshletsIndicesName;

            MeshletModel(Data::Asset<RPI::ModelAsset> sourceModelAsset);
            ~MeshletModel();

            Data::Instance<RPI::Model> GetMeshletModel() { return m_meshletModel; }
//            Data::Asset<RPI::ModelAsset> GetAtomModelAsset() { return m_atomModelAsset; }

            AZStd::string& GetName() { return m_name;  }

        protected:

            bool ProcessBuffersData(float* position, uint32_t vtxNum);

            uint32_t CreateMeshlest(GeneratorMesh& mesh);

            uint8_t* RetrieveBufferData(
                const RPI::BufferAssetView* bufferView,
                RHI::Format& format,
                uint32_t expectedAmount, uint32_t& existingAmount,
                RHI::BufferViewDescriptor& bufferDesc
            );

            Data::Asset<RPI::BufferAsset> CreateBufferAsset(
                const AZStd::string& bufferName,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                const void* data);

            uint32_t CreateMeshlets(
                float* positions, float* normals, float* texCoords, uint32_t vtxNum,
                uint16_t* indices, uint32_t idxNum, RHI::Format IndexStreamFormat);
            uint32_t CreateMeshletsFromModel(Data::Instance<RPI::Model>);
            uint32_t CreateMeshletsFromModelAsset(Data::Asset<RPI::ModelAsset> sourceModelAsset);

            uint32_t CreateMeshletModel(const RPI::ModelLodAsset::Mesh& meshAsset);
//            bool ProcessBuffersData();
//            bool CreateAtomModel();

        private:
            AZStd::string m_name;
            Aabb m_aabb;

            Data::Asset<RPI::ModelAsset> m_sourceModelAsset;
//            Data::Instance<RPI::Model> m_sourceModel;
            Data::Instance<RPI::Model> m_meshletModel;

//            RPI::MeshDrawPacket m_drawPacket;
//            AZStd::vector<RPI::MeshDrawPacket> m_drawPackets;
        };

    } // namespace AtomSceneStream
} // namespace AZ
