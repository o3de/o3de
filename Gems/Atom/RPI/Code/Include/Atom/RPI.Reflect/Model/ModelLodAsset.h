/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Asset/AssetHandler.h>
#include <Atom/RPI.Reflect/Configuration.h>
#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>

#include <Atom/RHI.Reflect/Limits.h>

#include <Atom/RPI.Reflect/Buffer/BufferAssetView.h>
#include <Atom/RPI.Reflect/Buffer/BufferAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Model/ModelMaterialSlot.h>

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
        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_REFLECT_API ModelLodAsset final
            : public Data::AssetData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class ModelLodAssetCreator;
            friend class ModelAsset;

        public:
            static constexpr size_t LodCountMax = 10;

            static constexpr const char* DisplayName{ "ModelLodAsset" };
            static constexpr const char* Group{ "Model" };
            static constexpr const char* Extension{ "azlod" };

            AZ_RTTI(ModelLodAsset, "{65B5A801-B9B9-4160-9CB4-D40DAA50B15C}", Data::AssetData);
            AZ_CLASS_ALLOCATOR(ModelLodAsset, AZ::SystemAllocator);

            static void Reflect(AZ::ReflectContext* context);

            ModelLodAsset() = default;
            ~ModelLodAsset() = default;

            //! Associates stream views (vertex buffer views and an index buffer view) with material data.
            //! A ModelLodAsset::Mesh can have many Streams but only one material id.
            class ATOM_RPI_REFLECT_API Mesh final
            {
                friend class ModelLodAssetCreator;
                friend class ModelLodAsset;

            public:
                AZ_TYPE_INFO(Mesh, "{55A91F9A-2F71-4B75-B2F7-565087DD2DBD}");
                AZ_CLASS_ALLOCATOR(Mesh, AZ::SystemAllocator);

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

                //! Returns the ID of the material slot used by this mesh.
                //! This maps into the ModelAsset's material slot list.
                ModelMaterialSlot::StableId GetMaterialSlotId() const;

                //! Returns the name of this mesh
                const AZ::Name& GetName() const;

                //! Returns the model-space axis-aligned bounding box of the mesh
                const AZ::Aabb& GetAabb() const;

                //! Returns a reference to the index buffer used by this mesh
                const BufferAssetView& GetIndexBufferAssetView() const;

                //! A helper method for returning this mesh's index buffer using a specific type for the elements.
                //! @note It's the caller's responsibility to choose the right type for the buffer.
                template<class T>
                AZStd::span<const T> GetIndexBufferTyped() const;

                //! Return an array view of the list of all stream buffer info (not including the index buffer)
                AZStd::span<const StreamBufferInfo> GetStreamBufferInfoList() const;

                //! A helper method for returning a specific buffer asset view.
                //! It will return nullptr if the semantic buffer is not found.
                //! For example, to get a position buffer for a mesh with AZ::Name("POSITION").
                //! In perf loop, re-use AZ::Name instance.
                const BufferAssetView* GetSemanticBufferAssetView(const AZ::Name& semantic) const;

                //! A helper method for returning this mesh's buffer using a specific type for the elements.
                //! For example, to get a position buffer for a mesh with AZ::Name("POSITION") and T=float.
                //! In perf loop, re-use AZ::Name instance.
                //! @note It's the caller's responsibility to choose the right type for the buffer.
                template<class T>
                AZStd::span<const T> GetSemanticBufferTyped(const AZ::Name& semantic) const;

            private:
                template<class T>
                AZStd::span<const T> GetBufferTyped(const BufferAssetView& bufferAssetView) const;
                                
                // Load/Release all the buffer assets referenced by this mesh
                void LoadBufferAssets();
                void ReleaseBufferAssets();

                AZ::Name m_name;
                AZ::Aabb m_aabb = AZ::Aabb::CreateNull();

                // Identifies the material slot that is used by this mesh.
                // References material slot in the ModelAsset that owns this mesh; see ModelAsset::FindMaterialSlot().
                ModelMaterialSlot::StableId m_materialSlotId = ModelMaterialSlot::InvalidStableId;

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
            AZStd::span<const Mesh> GetMeshes() const;

            //! Returns the model-space axis-aligned bounding box of all meshes in the lod
            const AZ::Aabb& GetAabb() const;

            Data::Asset<BufferAsset> GetIndexBufferAsset() const { return m_indexBuffer; }

            //! A helper method for returning a specific buffer asset view related to mesh associated with mesh index.
            const BufferAssetView* GetSemanticBufferAssetView(const AZ::Name& semantic, uint32_t meshIndex = 0) const;

        private:
            // AssetData overrides...
            bool HandleAutoReload() override
            {
                // Automatic asset reloads via the AssetManager are disabled for Atom models and their dependent assets because reloads
                // need to happen in a specific order to refresh correctly. They require more complex code than what the default
                // AssetManager reloading provides. See ModelReloader() for the actual handling of asset reloads.
                // Models need to be loaded via the MeshFeatureProcessor to reload correctly, and reloads can be listened
                // to by using MeshFeatureProcessor::ConnectModelChangeEventHandler().
                return false;
            }

            // Load/release all BufferAssets used by this ModelLodAsset
            void LoadBufferAssets();
            void ReleaseBufferAssets();
            
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

        template<class T>
        AZStd::span<const T> ModelLodAsset::Mesh::GetIndexBufferTyped() const
        {
            return GetBufferTyped<T>(GetIndexBufferAssetView());
        }

        template<class T>
        AZStd::span<const T> ModelLodAsset::Mesh::GetSemanticBufferTyped(const AZ::Name& semantic) const
        {
            const BufferAssetView* bufferAssetView = GetSemanticBufferAssetView(semantic);
            return bufferAssetView ? GetBufferTyped<T>(*bufferAssetView) : AZStd::span<const T>{};
        }

        template<class T>
        AZStd::span<const T> ModelLodAsset::Mesh::GetBufferTyped(const BufferAssetView& bufferAssetView) const
        {
            if (const BufferAsset* bufferAsset = bufferAssetView.GetBufferAsset().Get())
            {
                const AZStd::span<const uint8_t> rawBuffer = bufferAsset->GetBuffer();
                if (!rawBuffer.empty())
                {
                    const auto& bufferViewDescriptor = bufferAssetView.GetBufferViewDescriptor();

                    const uint8_t* beginMeshRawBuffer = rawBuffer.data() + (bufferViewDescriptor.m_elementOffset * bufferViewDescriptor.m_elementSize);
                    const uint8_t* endMeshRawBuffer = beginMeshRawBuffer + (bufferViewDescriptor.m_elementCount * bufferViewDescriptor.m_elementSize);

                    AZ_Assert((endMeshRawBuffer - beginMeshRawBuffer) % sizeof(T) == 0,
                        "Size of buffer (%d) is not a multiple of the type's size specified (%d)",
                        endMeshRawBuffer - beginMeshRawBuffer, sizeof(T));

                    return AZStd::span<const T>(reinterpret_cast<const T*>(beginMeshRawBuffer), reinterpret_cast<const T*>(endMeshRawBuffer));
                }
            }
            return {};
        }
    } // namespace RPI
} // namespace AZ
