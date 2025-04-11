/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Public/Model/UvStreamTangentBitmask.h>

#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/GeometryView.h>

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Limits.h>

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include <AzCore/std/containers/span.h>
#include <AtomCore/std/containers/vector_set.h>

#include <AzCore/std/containers/fixed_vector.h>

namespace AZ
{
    namespace RPI
    {
        //! A map matches the UV shader inputs of this material to the custom UV names from the model.
        using MaterialModelUvOverrideMap = AZStd::unordered_map<RHI::ShaderSemantic, AZ::Name>;

        AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
        class ATOM_RPI_PUBLIC_API ModelLod final
            : public Data::InstanceData
        {
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            friend class ModelSystem;

        public:

            //! Describes a single stream buffer/channel in a single mesh. For example position, normal, or UV.
            //! ModelLod always uses a separate stream buffer for each stream channel (no interleaving) so
            //! this struct contains information about both the stream buffer and the stream channel.
            struct StreamBufferInfo final
            {
                AZ_TYPE_INFO(StreamBufferInfo, "{3B133A3C-2562-46BE-B472-33089420EB68}");

                //! ID of the channel. (e.g. "POSITION", "NORMAL", "UV0", etc.)
                RHI::ShaderSemantic m_semantic;

                //! Specifically used by UV sets for now, to define custom readable name (e.g. Unwrapped) besides semantic (UVi)
                AZ::Name m_customName;
                
                //! Format of the vertex data in this channel
                RHI::Format m_format;
                
                //! Indicates a ModelLod::m_buffers entry
                uint32_t m_bufferIndex;
            };

            using StreamInfoList = AZStd::fixed_vector<StreamBufferInfo, RHI::Limits::Pipeline::StreamCountMax>;

            //! Mesh data associated with a specific material.
            struct Mesh final : public RHI::GeometryView
            {
                StreamInfoList m_streamInfo;

                ModelMaterialSlot::StableId m_materialSlotStableId = ModelMaterialSlot::InvalidStableId;
                AZ::Name m_materialSlotName;
                
                //! The default material assigned to the mesh by the asset.
                Data::Instance<Material> m_material;
            };

            using StreamBufferViewList = AZStd::fixed_vector<RHI::StreamBufferView, RHI::Limits::Pipeline::StreamCountMax>;

            AZ_INSTANCE_DATA(ModelLod, "{3C796FC9-2067-4E0F-A660-269F8254D1D5}");
            AZ_CLASS_ALLOCATOR(ModelLod, AZ::SystemAllocator);

            static Data::Instance<ModelLod> FindOrCreate(const Data::Asset<ModelLodAsset>& lodAsset, const Data::Asset<ModelAsset>& modelAsset);

            ~ModelLod() = default;

            //! Blocks the CPU until pending buffer uploads have completed.
            void WaitForUpload();

            AZStd::span<Mesh> GetMeshes();

            //! Compares a ShaderInputContract to the mesh's available streams, and if any of them are optional, sets the corresponding "*_isBound" shader option.
            //! Call this function to update the ShaderOptionKey before fetching a ShaderVariant, to find a variant that is compatible with this mesh's streams.
            // @param contract the contract that defines the expected inputs for a shader, used to determine which streams are optional.
            // @param meshIndex the index of the mesh to search in.
            // @param materialModelUvMap a map of UV name overrides, which can be supplied to bind a specific mesh stream name to a different material shader stream name.
            // @param materialUvNameMap the UV name map that came from a MaterialTypeAsset, which defines the default set of material shader stream names.
            void CheckOptionalStreams(
                ShaderOptionGroup& shaderOptions,
                const ShaderInputContract& contract,
                size_t meshIndex,
                const MaterialModelUvOverrideMap& materialModelUvMap = {},
                const MaterialUvNameMap& materialUvNameMap = {}) const;

            //! Fills a InputStreamLayout and StreamBufferViewList for the set of streams that satisfy a ShaderInputContract.
            // @param uvStreamTangentBitmaskOut a mask processed during UV stream matching, and later to determine which tangent/bitangent stream to use.
            // @param contract the contract that defines the expected inputs for a shader, used to determine which streams are optional.
            // @param meshIndex the index of the mesh to search in.
            // @param materialModelUvMap a map of UV name overrides, which can be supplied to bind a specific mesh stream name to a different material shader stream name.
            // @param materialUvNameMap the UV name map that came from a MaterialTypeAsset, which defines the default set of material shader stream names.
            bool GetStreamsForMesh(
                RHI::InputStreamLayout& layoutOut,
                RHI::StreamBufferIndices& streamIndicesOut,
                UvStreamTangentBitmask* uvStreamTangentBitmaskOut,
                const ShaderInputContract& contract,
                size_t meshIndex,
                const MaterialModelUvOverrideMap& materialModelUvMap = {},
                const MaterialUvNameMap& materialUvNameMap = {});

            // Releases all the buffer dependencies that were added through TrackBuffer
            void ReleaseTrackedBuffers();

        private:
            ModelLod() = default;

            static Data::Instance<ModelLod> CreateInternal(const Data::Asset<ModelLodAsset>& lodAsset, const AZStd::any* modelAssetAny);
            RHI::ResultCode Init(const Data::Asset<ModelLodAsset>& lodAsset, const Data::Asset<ModelAsset>& modelAsset);

            bool SetMeshInstanceData(
                const ModelLodAsset::Mesh::StreamBufferInfo& streamBufferInfo,
                Mesh& meshInstance);

            StreamInfoList::const_iterator FindFirstUvStreamFromMesh(size_t meshIndex) const;

            StreamInfoList::const_iterator FindDefaultUvStream(size_t meshIndex, const MaterialUvNameMap& materialUvNameMap) const;

            // Finds a mesh vertex input stream that is the best match for a contracted stream channel.
            // @param meshIndex the index of the mesh to search in.
            // @param materialModelUvMap a map of UV name overrides, which can be supplied to bind a specific mesh stream name to a different material shader stream name.
            // @param materialUvNameMap the UV name map that came from a MaterialTypeAsset, which defines the default set of material shader stream names.
            // @param defaultUv the default UV stream to use if a matching UV stream could not be found. Use FindDefaultUvStream() to populate this.
            // @param firstUv the first UV stream from the mesh, which, by design, the tangent/bitangent stream belongs to.
            // @param uvStreamTangentIndex a bitset indicating which tangent/bitangent stream (including generated ones) a UV stream will be using.
            StreamInfoList::const_iterator FindMatchingStream(
                size_t meshIndex,
                const MaterialModelUvOverrideMap& materialModelUvMap,
                const MaterialUvNameMap& materialUvNameMap,
                const ShaderInputContract::StreamChannelInfo& contractStreamChannel,
                StreamInfoList::const_iterator defaultUv,
                StreamInfoList::const_iterator firstUv,
                UvStreamTangentBitmask* uvStreamTangentBitmaskOut) const;

            // Meshes may share index/stream buffers in an LOD or they may have 
            // unique buffers. Often the asset builder will prioritize shared buffers
            // so we need to check if the buffer is already tracked before we add it
            // to the list.
            // @return the index of the buffer in m_buffers
            uint32_t TrackBuffer(const Data::Instance<Buffer>& buffer);

            // Collection of buffers grouped by payload
            // Provides buffer views backed by data in m_buffers;
            AZStd::vector<Mesh> m_meshes;

            // The buffer instances loaded by this ModelLod
            AZStd::vector<Data::Instance<Buffer>> m_buffers;

            // Tracks whether buffers have all been streamed up to the GPU.
            bool m_isUploadPending = false;

            AZStd::mutex m_callbackMutex;
        };
    } // namespace RPI
} // namespace AZ
