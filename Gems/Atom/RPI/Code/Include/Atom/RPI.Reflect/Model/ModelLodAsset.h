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

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>

#include <Atom/RHI.Reflect/Limits.h>

#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>
#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Name/Name.h>

namespace AZ
{
    namespace RPI
    {
        //! Contains a set of ModelLodAsset::Mesh objects and BufferAsset objects, representing the data a single level-of-detail for a Model.
        //! Serialized to a .azlod file.
        //! Actual vertex and index buffer data is stored in the BufferAssets.
        class ModelLodAsset final
            : public Data::AssetData
        {
            friend class ModelLodAssetCreator;

        public:
            static constexpr size_t LodCountMax = 10;

            static const char* DisplayName;
            static const char* Extension;
            static const char* Group;

            AZ_RTTI(ModelLodAsset, "{65B5A801-B9B9-4160-9CB4-D40DAA50B15C}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(ModelLodAsset, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* context);

            ModelLodAsset() = default;
            ~ModelLodAsset() = default;

            //! Associates stream views (vertex buffer views and an index buffer view) with material data.
            //! A ModelLodAsset::Mesh can have many Streams but only one material id.
            class Mesh final
            {
                friend class ModelLodAssetCreator;
            public:
                AZ_TYPE_INFO(Mesh, "{55A91F9A-2F71-4B75-B2F7-565087DD2DBD}");
                AZ_CLASS_ALLOCATOR(Mesh, AZ::SystemAllocator, 0);

                static void Reflect(AZ::ReflectContext* context);

                Mesh() = default;
                ~Mesh() = default;

                //! Describes a single stream buffer/channel in a single mesh. 
                //! For example position, normal, or UV.
                //! ModelLodAsset always uses a separate stream buffer for each stream channel (no interleaving) so
                //! this struct is used to describe both the stream buffer and the stream channel.
                struct StreamBufferInfo final
                {
                    AZ_TYPE_INFO(StreamBufferInfo, "{362FB05F-D059-41B8-B3CB-EE0D9F855139}");

                    static void Reflect(AZ::ReflectContext* context);

                    RHI::ShaderSemantic m_semantic;
                    //! Specifically used by UV sets for now, to define custom readable name (e.g. Unwrapped) besides semantic (UVi)
                    AZ::Name m_customName;
                    BufferAssetView m_bufferAssetView;
                };

                //! Returns the number of vertices in this mesh
                uint32_t GetVertexCount() const;

                //! Returns the number of indices in this mesh
                uint32_t GetIndexCount() const;

                //! Returns the reference to material asset used by this mesh
                const Data::Asset <MaterialAsset>& GetMaterialAsset() const;

                //! Returns the name of this mesh
                const AZ::Name& GetName() const;

                //! Returns the model-space axis-aligned bounding box of the mesh
                const AZ::Aabb& GetAabb() const;

                //! Returns a reference to the index buffer used by this mesh
                const BufferAssetView& GetIndexBufferAssetView() const;

                //! Return an array view of the list of all stream buffer info (not including the index buffer)
                AZStd::array_view<StreamBufferInfo> GetStreamBufferInfoList() const;

                //! A helper method for returning a specific buffer asset view.
                //! It will return nullptr if the semantic buffer is not found.
                //! For example, to get a position buffer for a mesh with AZ::Name("POSITION").
                //! In perf loop, re-use AZ::Name instance.
                const BufferAssetView* GetSemanticBufferAssetView(const AZ::Name& semantic) const;

                //! A helper method for returning a specific mesh buffer.
                //! For example, to get a position buffer for a mesh with AZ::Name("POSITION").
                //! In perf loop, re-use AZ::Name instance.
                AZStd::array_view<uint8_t> GetSemanticBuffer(const AZ::Name& semantic) const;

            private:
                AZ::Name m_name;
                AZ::Aabb m_aabb = AZ::Aabb::CreateNull();

                Data::Asset<MaterialAsset> m_materialAsset{ Data::AssetLoadBehavior::PreLoad };

                // Both the buffer in m_indexBufferAssetView and the buffers in m_streamBufferInfo 
                // may point to either unique buffers for the mesh or to consolidated 
                // buffers owned by the lod.

                BufferAssetView m_indexBufferAssetView;

                // These stream buffers are not ordered. If a specific ordering is required it's 
                // expected that the user calls GetStreamBufferInfo with the required semantics
                // and pieces the layout together themselves.
                AZStd::fixed_vector<StreamBufferInfo, RHI::Limits::Pipeline::StreamCountMax> m_streamBufferInfo;
            };

            //! Returns an array view into the collection of meshes owned by this lod
            AZStd::array_view<Mesh> GetMeshes() const;

            //! Returns the model-space axis-aligned bounding box of all meshes in the lod
            const AZ::Aabb& GetAabb() const;

        private:
            AZStd::vector<Mesh> m_meshes;
            AZ::Aabb m_aabb = AZ::Aabb::CreateNull();

            // These buffers owned by the lod are the consolidated super buffers. 
            // Meshes may either have views into these buffers or they may own 
            // their own buffers.

            Data::Asset<BufferAsset> m_indexBuffer;
            AZStd::vector<Data::Asset<BufferAsset>> m_streamBuffers;

            void AddMesh(const Mesh& mesh);

            void SetReady();
        };

        using ModelLodAssetHandler = AssetHandler<ModelLodAsset>;
    } // namespace RPI
} // namespace AZ
