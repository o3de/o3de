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
#include <AzCore/std/containers/list.h>

#include <AtomCore/Instance/Instance.h>
#include <AtomCore/Instance/InstanceData.h>

#include <Atom/RHI/SingleDeviceImagePool.h>

#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include "../../External/meshoptimizer.h"

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
        //======================================================================
        //!       Reference Class to Demonstrate Meshlets Prep in CPU
        //! The following class is taking in ModelAsset and based on it, generates
        //! a new Atom Model that now contains enhanced meshlet data.
        //! This class is built to demonstrate and used as reference for using the mesh data
        //! to generate the meshlets on the fly and send them to the regular render.
        //! This is NOT the class that will be used to create indirect Compute and Draw calls.
        //! For this we will be using the MeshletRenderObject class.
        //! 
        //! Currently assuming single model without Lods so that the handling of the
        //! meshlet creation and handling of the array is easier. If several meshes or Lods
        //! exist, they will be created as separate models and the last model's instance
        //! will be kept in this class.
        //! Each one of the ModelLods contains a vector of meshes, representing possible
        //! multiple element within the mesh - to fully represent a mesh, the replication
        //! method will need to run and gather all data, unify it within a single stream
        //! and address from each of the Lods.
        //======================================================================
        class MeshletsModel
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
            Name MeshletsTrianglesName;
            Name MeshletsIndicesLookupName;

            MeshletsModel(Data::Asset<RPI::ModelAsset> sourceModelAsset);
            ~MeshletsModel();

            static  Data::Instance<RPI::ShaderResourceGroup> CreateShaderResourceGroup(
                Data::Instance<RPI::Shader> shader,
                const char* shaderResourceGroupId,
                [[maybe_unused]] const char* moduleName
            );

            Data::Instance<RPI::Model> GetMeshletsModel() { return m_meshletsModel; }

            AZStd::string& GetName() { return m_name;  }

        protected:
            bool ProcessBuffersData(float* position, uint32_t vtxNum);
            void debugMarkMeshletsUVs(GeneratorMesh& mesh);

            uint32_t CreateMeshlets(GeneratorMesh& mesh);

            uint8_t* RetrieveBufferData(
                const RPI::BufferAssetView* bufferView,
                RHI::Format& format,
                uint32_t expectedAmount, uint32_t& existingAmount,
                RHI::BufferViewDescriptor& bufferDesc
            );

            Data::Asset<RPI::BufferAsset> CreateBufferAsset(
                const AZStd::string& bufferName,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                RHI::BufferBindFlags bufferBindFlags,
                const void* data);

            uint32_t CreateMeshlets(
                float* positions, float* normals, float* texCoords, uint32_t vtxNum,
                uint16_t* indices, uint32_t idxNum, RHI::Format IndexStreamFormat
            );
            uint32_t CreateMeshletsFromModelAsset(Data::Asset<RPI::ModelAsset> sourceModelAsset);

            uint32_t CreateMeshletsModel(const RPI::ModelLod& modelLod);
            uint32_t CreateMeshletsModel(const RPI::ModelLodAsset::Mesh& meshAsset);

            uint32_t GetMehsletsAmount() { return m_meshletsAmount;  }

        private:
            AZStd::string m_name;

            Data::Instance<RPI::Shader> m_meshletsDataPrepComputeShader;

            Aabb m_aabb;    // Should be per Lod per mesh and not global

            Data::Asset<RPI::ModelAsset> m_sourceModelAsset;

            Data::Instance<RPI::Model> m_meshletsModel;

            // Meshlets data should be a vector of meshlets data per lod per mesh
            MeshletsData m_meshletsData;    // the actual mesh meshlets' data

            uint32_t m_meshletsAmount = 0;
        };

    } // namespace Meshlets
} // namespace AZ
