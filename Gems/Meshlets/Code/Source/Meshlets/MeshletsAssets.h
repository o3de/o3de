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


#include "../../External/Include/meshoptimizer.h""
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
        //! meshlet creation and handling of the array is easier.
        //! To enhance this, add inheritance to lower levels of the model / mesh.
        //! MeshletModel represents a combined model that can contain an array
        //! of ModelLods.
        //! Each one of the ModelLods contains a vector of meshes, representing possible multiple
        //! element within the mesh.
        class MeshletModel
        {
        public:
            Name IndicesSemanticName;

            Name PositionSemanticName;
            Name NormalSemanticName;
            Name TangentSemanticName;
            Name BiTangentSemanticName;
            Name UVSemanticName;

            Name MeshletsDescriptorsName;
            Name MeshletsVertexIndicesName;
            Name MeshletsIndicesName;

            MeshletModel();
            ~MeshletModel();

            Data::Instance<RPI::Model> GetAtomModel() { return m_atomModel; }
            Data::Asset<RPI::ModelAsset> GetAtomModelAsset() { return m_atomModelAsset; }

            AZStd::string& GetName() { return m_name;  }

        protected:

            void CreateMeshlest(GeneratorMesh& mesh);

            uint8_t* RetrieveBufferData(
                const RPI::ModelLodAsset::Mesh& meshAsset,
                const Name& semanticName,
                RHI::Format& format,
                uint32_t expectedAmount, uint32_t& existingAmount);

            Data::Asset<RPI::BufferAsset> CreateBufferAsset(
                const AZStd::string& bufferName,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                const void* data);

            bool CreateMeshlets(
                float* positions, float* normals, float* texCoords, uint32_t vtxNum,
                uint16_t* indices, uint32_t idxNum, RHI::Format IndexStreamFormat);
            bool CreateMeshletsFromModel();
            bool CreateMeshletsFromModelAsset();

//            bool ProcessBuffersData();
//            bool CreateAtomModel();

        private:
            AZStd::string m_name;

            Data::Instance<RPI::Model> m_atomModel;
            Data::Asset<RPI::ModelAsset> m_atomModelAsset;

//            RPI::MeshDrawPacket m_drawPacket;
//            AZStd::vector<RPI::MeshDrawPacket> m_drawPackets;
        };

    } // namespace AtomSceneStream
} // namespace AZ
