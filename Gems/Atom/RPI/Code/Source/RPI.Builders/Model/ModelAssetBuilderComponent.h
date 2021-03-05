/*
 * All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution(the "License").All use of this software is governed by the License,
 *or, if provided, by the license below or the license accompanying this file.Do not
 * remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
 *WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Base.h>

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>

#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexBitangentData.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

#include <Model/ModelExporterContexts.h>

#include <AzCore/std/containers/map.h>

namespace AZ
{
    namespace RPI
    {
        using MeshData = AZ::SceneAPI::DataTypes::IMeshData;
        using TransformData = AZ::SceneAPI::DataTypes::ITransform;
        using UVData = AZ::SceneAPI::DataTypes::IMeshVertexUVData;
        using ColorData = AZ::SceneAPI::DataTypes::IMeshVertexColorData;
        using MaterialData = AZ::SceneAPI::DataTypes::IMaterialData;
        using TangentData = AZ::SceneAPI::DataTypes::IMeshVertexTangentData;
        using BitangentData = AZ::SceneAPI::DataTypes::IMeshVertexBitangentData;

        class Stream;
        class ModelLodAssetCreator;
        class BufferAssetCreator;

        
        //! Component responsible for building Atom's AzModel from SceneAPI input.
        //! 
        //! The current strategy in this builder is to merge data as much as possible
        //! while keeping streams separate.
        //! This may change for different platforms in the future.
        //! 
        //! Currently Meshes are merged as much as possible at the ModelLod level.
        //! An input SceneAPI Model with 3 meshes in an LOD are going to get merged
        //! into stream buffers owned by the ModelLod. The Meshes in the ModelLod
        //! are then just going to act as views into the ModelLod's buffers with
        //! an associated Material.        
        class ModelAssetBuilderComponent
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(
                ModelAssetBuilderComponent,
                "{FE21DEEB-F9E6-487E-B9F0-88478FC2F52F}",
                AZ::SceneAPI::SceneCore::ExportingComponent);
            static void Reflect(AZ::ReflectContext* context);

            ModelAssetBuilderComponent();
            ~ModelAssetBuilderComponent() override = default;

            AZ::SceneAPI::Events::ProcessingResult BuildModel(ModelAssetBuilderContext& context);

        private:
            using MaterialUid = uint64_t;

            //! Describes the source SceneAPI data that makes up a "Mesh" as understood by Atom.
            struct SourceMeshContent
            {
                AZ::Name m_name;
                AZStd::shared_ptr<const MeshData> m_meshData;
                SceneAPI::DataTypes::MatrixType m_worldTransform = SceneAPI::DataTypes::MatrixType::CreateIdentity();
                AZStd::shared_ptr<const TangentData> m_meshTangents;
                AZStd::shared_ptr<const BitangentData> m_meshBitangents;
                AZStd::vector<AZStd::shared_ptr<const UVData>> m_meshUVData;
                AZStd::vector<AZStd::shared_ptr<const ColorData>> m_meshColorData;
                AZStd::vector<MaterialUid> m_materials;
                MaterialUid GetMaterialUniqueId(uint32_t index) const;
            };
            using SourceMeshContentList = AZStd::vector<SourceMeshContent>;

            //! Describes the data needed to produce the buffers that make up a Mesh.
            struct ProductMeshContent
            {
                AZ::Name m_name;
                AZStd::vector<uint32_t> m_indices;
                AZStd::vector<float> m_positions;
                AZStd::vector<float> m_normals;
                AZStd::vector<float> m_tangents;
                AZStd::vector<float> m_bitangents;
                AZStd::vector<AZStd::vector<float>> m_uvSets;
                AZStd::vector<AZ::Name> m_uvCustomNames;
                AZStd::vector<AZStd::vector<float>> m_colorSets;
                AZStd::vector<AZ::Name> m_colorCustomNames;

                MaterialUid m_materialUid;
            };
            using ProductMeshContentList = AZStd::vector<ProductMeshContent>;

            //! Used to accumulate info about how much space to allocate when creating a ProductMeshContent structure.
            //! 
            //! This is needed to batch memory allocations together. Otherwise
            //! hundreds of thousands of small allocations when building a ProductMeshContent
            //! structure can cause the allocators to fragment heavily and waste large
            //! amounts of memory. 2GB of model memory can become 30+GB of fragmented memory in
            //! the allocator.
            struct ProductMeshContentAllocInfo
            {
                size_t m_indexCount = 0;
                size_t m_positionsFloatCount = 0;
                size_t m_normalsFloatCount = 0;
                size_t m_tangentsFloatCount = 0;
                size_t m_bitangentsFloatCount = 0;
                AZStd::vector<size_t> m_uvSetFloatCounts;
                AZStd::vector<size_t> m_colorSetFloatCounts;
            };

            //! Describes a view into data described in a ProductMeshContent structure.
            //! 
            //! As described by the strategy in the class comment, Meshes are going to
            //! just be views into the ModelLod-wide buffers.
            //! If a ProductMeshContent structure describes all the buffers in a ModelLod,
            //! a ProductMeshView describes each Mesh's view into those ModelLod buffers.
            struct ProductMeshView
            {
                AZ::Name m_name;
                RHI::BufferViewDescriptor m_indexView;
                RHI::BufferViewDescriptor m_positionView;
                RHI::BufferViewDescriptor m_normalView;
                AZStd::vector<RHI::BufferViewDescriptor> m_uvSetViews;
                AZStd::vector<AZ::Name> m_uvCustomNames;
                AZStd::vector<RHI::BufferViewDescriptor> m_colorSetViews;
                AZStd::vector<AZ::Name> m_colorCustomNames;
                RHI::BufferViewDescriptor m_tangentView;
                RHI::BufferViewDescriptor m_bitangentView;

                MaterialUid m_materialUid;
            };
            using ProductMeshViewList = AZStd::vector<ProductMeshView>;

            //! Takes an abstract graph object and tries to add it to the given SourceMeshContent object.
            void AddToMeshContent(
                const AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IGraphObject>& data,
                SourceMeshContent& content);

            //! Takes in a list of SourceMeshContent and outputs list of ProductMeshContent.
            //! This product list is not 1:1 but it is as close as we can get with minimal processing.
            //! Since a SouceMeshContent object may have faces that have multiple materials we have to break 
            //! that into a ProductMeshContent object for each material. No further processing is done
            ProductMeshContentList SourceMeshListToProductMeshList(
                const SourceMeshContentList& sourceMeshContentList);

            //! Takes in a ProductMeshContentList and merges all elements that share the same MaterialUid.
            ProductMeshContentList MergeMeshesByMaterialUid(
                const ProductMeshContentList& productMeshList);

            //! Simple helper to create a MeshView that views an entire given ProductMeshContent object as one mesh.
            ProductMeshView CreateViewToEntireMesh(const ProductMeshContent& mesh);

            //! Takes a ProductMeshContentList and merges all elements into a single ProductMeshContent object.
            //! This also produces a ProductMeshViewList that contains views to all
            //! the original meshes described in the lodMeshList collection.
            void MergeMeshesToCommonBuffers(
                const ProductMeshContentList& lodMeshList,
                ProductMeshContent& lodMeshContent,
                ProductMeshViewList& meshViewsPerLodBuffer);

            enum IndicesOperation
            {
                PreserveIndices,
                RemapIndices
            };

            //! Helper method for MergeMeshesByMaterialUid and MergeAllMeshes.
            //! Takes a given ProductMeshContentList and outputs a single
            //! ProductMeshContent object.
            //! 
            //! If preserveIndices is true the values of the indices will not be rescaled
            //! to treat the merged mesh as one continuous mesh. Instead each mesh will retain
            //! its indices as they were.
            ProductMeshContent MergeMeshList(
                const ProductMeshContentList& productMeshList,
                IndicesOperation indicesOp);

            //! Takes a ProductMeshContent object, produces BufferAsset objects for each
            //! stream and then applies those to the given ModelLodAssetCreator as the
            //! lod-wide buffers. It also returns the index and stream buffer data so that it
            //! can be referenced later.
            //! 
            //! Returns false if an error occurs
            bool CreateModelLodBuffers(
                const ProductMeshContent& lodBufferContent,
                BufferAssetView& outIndexBuffer,
                AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& outStreamBuffers,
                ModelLodAssetCreator& lodAssetCreator);

            //! Takes a ProductMeshView and the buffers that it is supposed to be a view
            //! into and adds that data as a Mesh onto the given ModelLodAssetCreator.
            //! 
            //! Returns false if an error occurs
            bool CreateMesh(
                const ProductMeshView& meshView,
                const BufferAssetView& lodIndexBuffer,
                const AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& lodStreamBuffers,
                ModelLodAssetCreator& lodAssetCreator,
                const MaterialAssetsByUid& materialAssetsByUid);

            //! Takes in a pointer to data with a given element count and format and creates a BufferAsset.
            Outcome<Data::Asset<BufferAsset>> CreateBufferAsset(
                const void* data, const size_t elementCount, RHI::Format format, const AZStd::string& bufferName);

            //! Helper method for CreateMesh.
            //! Searches lodStreamBuffers for the given semantic and if found takes
            //! the BufferAsset in that StreamBufferInfo and pairs it with the given
            //! bufferViewDescriptor to add a Mesh stream buffer to the given lodAssetCreator.
            bool SetMeshStreamBufferById(
                const RHI::ShaderSemantic& semantic,
                const AZ::Name& customName,
                const RHI::BufferViewDescriptor& bufferViewDescriptor,
                const AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& lodStreamBuffers,
                ModelLodAssetCreator& lodAssetCreator);

            // Create stable asset id from an unique name
            Data::AssetId CreateAssetId(const AZStd::string& assetName);

            // Get asset full name for different product assets based on their type 
            // For buffer asset, it needs to provide a buffer name
            AZStd::string GetAssetFullName(const TypeId& assetType, const AZStd::string& bufferName = {});

            //! Calculates the AABB of the SubMesh.
            //! This should be called when a position stream is added
            //! 
            //! @param[in] bufferViewDesc The buffer view descriptor used to examine the given bufferAsset
            //! and determine the range of the buffer to use to calculate the AABB from.
            //! @param[in] bufferAsset The BufferAsset to calculate the AABB from. Expected to
            //! be a Position stream
            //! @param[out] aabb The AABB that encompasses the given stream
            //! @return True if the AABB was successfully calculated
            static bool CalculateAABB(const RHI::BufferViewDescriptor& bufferViewDesc, const BufferAsset& bufferAsset, AZ::Aabb& aabb);
            
            //! Helper method for CreateMesh. 
            //! Finds a buffer in the given streamBufferInfoList that matches the given stream id 
            //! and returns it as part of outStreamBufferInfo.
            //! Returns false if no StreamBufferInfo is found.
            bool FindStreamBufferById(
                const AZStd::vector<ModelLodAsset::Mesh::StreamBufferInfo>& streamBufferInfoList,
                const RHI::ShaderSemantic& streamSemantic,
                ModelLodAsset::Mesh::StreamBufferInfo& outStreamBufferInfo);

            Uuid m_sourceUuid;

            // cached names for asset id generation
            AZStd::string m_modelName;
            AZStd::string m_lodName;
            AZStd::string m_meshName;

            AZStd::set<uint32_t> m_createdSubId;

            // NOTE: This is explicitly fetched from a filename. In the future, this should be fetched from the RPI system
            // configuration data.
            Data::AssetId m_systemInputAssemblyBufferPoolId;

            //! Calculates the world transform of the node given all of its parent nodes
            SceneAPI::DataTypes::MatrixType GetWorldTransform(const SceneAPI::Containers::SceneGraph& sceneGraph, SceneAPI::Containers::SceneGraph::NodeIndex node);
        };
    } // namespace RPI
} // namespace AZ
