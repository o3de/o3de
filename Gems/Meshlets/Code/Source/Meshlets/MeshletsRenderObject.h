/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project. 
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
* 
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <AzCore/base.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/array.h>

#include <AtomCore/Instance/Instance.h>
#include <AtomCore/Instance/InstanceData.h>

#include <Atom/RPI.MultiDeviceStreamBufferViewclude <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include <Atom/Feature/TransformService/TransformServiceFeatureProcessor.h>

#include "../../External/meshoptimizer.h"

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
        const uint32_t maxVerticesPerMeshlet = 64;     // matching wave/warp groups size multiplier
//        const uint32_t maxTrianglesPerMeshlet = 124; // NVidia-recommended 126, rounded down to a multiple of 4
        const uint32_t maxTrianglesPerMeshlet = 64;    // Set it to 64 per inspection of both GPU threads / generated data

        class MeshletsFeatureProcessor;

        // The following structure holds the per object data - currently with no support
        // for instancing.
        // To support instancing move the dispatch item, draw packet and object Id to
        // a separate instance data structure.
        // The data here should only represent the object render/compute data without
        // having any instance data (matrices, Id, etc..)
        struct MeshRenderData
        {
            Render::TransformServiceFeatureProcessorInterface::ObjectId ObjectId;   // should be per instance

            uint32_t MeshletsCount = 0;

            // Used by the direct Draw stage only - should be changed for indirect culled render.
            uint32_t IndexCount = 0;  

             //! Compute render data
            Data::Instance<RPI::ShaderResourceGroup> ComputeSrg;          // Per object Compute data - can be shared across instances
            AZStd::vector<SrgBufferDescriptor> ComputeBuffersDescriptors;
            AZStd::vector<Data::Instance<RHI::BufferView>> ComputeBuffersViews;
            AZStd::vector<Data::Instance<Meshlets::SharedBufferAllocation>> ComputeBuffersAllocators;
            AZStd::vector <Data::Instance<RPI::Buffer>> ComputeBuffers;   // stand alone non shared buffers
            MeshletsDispatchItem MeshDispatchItem;

            //! Render pass data
            Data::Instance<RPI::ShaderResourceGroup> RenderObjectSrg;     // Per object render data - includes instanceId and vertex buffers
            AZStd::vector<SrgBufferDescriptor> RenderBuffersDescriptors;
            RHI::IndexBufferView IndexBufferView;
            AZStd::vector<Data::Instance<RHI::BufferView>> RenderBuffersViews;
            AZStd::vector <Data::Instance<RPI::Buffer>> RenderBuffers;    // stand alone non shared buffers

            const RHI::DrawPacket* MeshDrawPacket = nullptr;    // Should be moved to the instance data structure
        };
        using ModelLodDataArray = AZStd::vector<MeshRenderData*>;    // MeshRenderData per mesh in the Lod

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

            Name s_textureCoordinatesName;
            Name s_indicesName;

            MeshletsRenderObject(Data::Asset<RPI::ModelAsset> sourceModelAsset, MeshletsFeatureProcessor* meshletsFeatureProcessor);
            ~MeshletsRenderObject();

            static  Data::Instance<RPI::ShaderResourceGroup> CreateShaderResourceGroup(
                Data::Instance<RPI::Shader> shader,
                const char* shaderResourceGroupId,
                [[maybe_unused]] const char* moduleName
            );

            AZStd::string& GetName() { return m_name;  }

            ModelLodDataArray& GetMeshletsRenderData(uint32_t lodIdx = 0)
            {
                AZ_Assert(m_modelRenderData.size(), "Meshlets - model does not contain any render data");
                return m_modelRenderData[AZStd::max(lodIdx, (uint32_t)m_modelRenderData.size() - 1)];
            }


            // This method is binding the buffers to the Srg and is separated from
            // the creation method to allow frame sync when the data is compiled
            static bool CreateAndBindComputeSrgAndDispatch(Data::Instance<RPI::Shader> computeShader, MeshRenderData& meshRenderData);

            uint32_t GetMeshletsCount() { return m_meshletsCount; }

            // The prep of this data should be used to create the shared buffer alignment 
            static void PrepareRenderSrgDescriptors(MeshRenderData &meshRenderData, uint32_t vertexCount, uint32_t indicesCount);

        protected:
            bool ProcessBuffersData(float* position, uint32_t vtxNum);

            uint8_t* RetrieveBufferData(
                const RPI::BufferAssetView* bufferView,
                RHI::Format& format,
                uint32_t expectedAmount, uint32_t& existingAmount,
                RHI::BufferViewDescriptor& bufferDesc
            );

            bool RetrieveSourceMeshData(
                const RPI::ModelLodAsset::Mesh& meshAsset,
                MeshRenderData& meshRenderData,
                uint32_t vertexCount, uint32_t indexCount);

            uint32_t CreateMeshlets(GeneratorMesh& mesh);

            uint32_t CreateMeshlets(
                float* positions, float* normals, float* texCoords, uint32_t vtxNum,
                uint16_t* indices, uint32_t idxNum, RHI::Format IndexStreamFormat
            );

            uint32_t CreateMeshletsFromModelAsset(Data::Asset<RPI::ModelAsset> sourceModelAsset);

            uint32_t CreateMeshletsRenderObject(const RPI::ModelLodAsset::Mesh& meshAsset, MeshRenderData &meshRenderData);


            bool BuildDrawPacket( RHI::DrawPacketBuilder::DrawRequest& drawRequest, MeshRenderData& meshRenderData);

            bool CreateAndBindRenderBuffers(MeshRenderData &meshRenderData);

            void PrepareComputeSrgDescriptors(MeshRenderData &meshRenderData, uint32_t vertexCount, uint32_t indexCount);

            bool CreateComputeBuffers(MeshRenderData &meshRenderData);

            bool SetShaders();

        private:
            MeshletsFeatureProcessor* m_featureProcessor = nullptr;
            AZStd::string m_name;

            Aabb m_aabb;    // Should be per Lod per mesh and not global

            static AZStd::vector<SrgBufferDescriptor> m_srgBufferDescriptors;

            // [To Do] - meshlets data should be a vector of meshlets data per lod per mesh
            // This should be fairly easy to do once LOD are properly supported - set it
            // in the MeshRenderData.
            MeshletsData m_meshletsData;    // the actual mesh meshlets' data

            uint32_t m_meshletsCount = 0;

            //------------------------------------------------------------------
            // Remarks:
            // 1. Moving to indirect compute, all the buffer views will need to either
            // become offsets passed as part of each mesh dispatch, or bindless resources.
            // Having the first approach does not require bindless mechanism in place.  
            //------------------------------------------------------------------
            Data::Instance<RPI::Shader> m_renderShader;
            Data::Instance<RPI::Shader> m_computeShader;

            AZStd::vector<ModelLodDataArray> m_modelRenderData;         // Render data array of Lods.
        };

    } // namespace Meshlets
} // namespace AZ
