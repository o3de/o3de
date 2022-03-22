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
#include <MeshletsData.h>

namespace AZ
{
    namespace RPI
    {
        class MeshDrawPacket;
    }

    namespace Meshlets
    {
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
