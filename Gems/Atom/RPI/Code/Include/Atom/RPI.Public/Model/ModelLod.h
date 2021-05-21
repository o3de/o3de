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

#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Material/Material.h>

#include <Atom/RHI/DrawItem.h>

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Limits.h>

#include <Atom/RPI.Reflect/Model/ModelLodAsset.h>

#include <AtomCore/std/containers/array_view.h>
#include <AtomCore/std/containers/vector_set.h>

#include <AzCore/std/containers/fixed_vector.h>

namespace AZ
{
    namespace RPI
    {
        //! A map matches the UV shader inputs of this material to the custom UV names from the model.
        using MaterialModelUvOverrideMap = AZStd::unordered_map<RHI::ShaderSemantic, AZ::Name>;

        class UvStreamTangentBitmask;

        class ModelLod final
            : public Data::InstanceData
        {
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
                
                //! Indicates a range within the ModelLod::m_buffers entry (because each buffer contains vertex data for all meshes in the LOD)
                uint32_t m_byteOffset;           
                uint32_t m_byteCount;
                
                //! Number of bytes in one element of the stream. This corresponds to m_format.
                uint32_t m_stride;
            };

            using StreamInfoList = AZStd::fixed_vector<StreamBufferInfo, RHI::Limits::Pipeline::StreamCountMax>;

            //! Mesh data associated with a specific material.
            struct Mesh final
            {
                RHI::DrawArguments m_drawArguments;
                RHI::IndexBufferView m_indexBufferView;

                StreamInfoList m_streamInfo;
                
                //! The default material assigned to the mesh by the asset.
                Data::Instance<Material> m_material;
            };

            using StreamBufferViewList = AZStd::fixed_vector<RHI::StreamBufferView, RHI::Limits::Pipeline::StreamCountMax>;

            AZ_INSTANCE_DATA(ModelLod, "{3C796FC9-2067-4E0F-A660-269F8254D1D5}");
            AZ_CLASS_ALLOCATOR(ModelLod, AZ::SystemAllocator, 0);

            static Data::Instance<ModelLod> FindOrCreate(const Data::Asset<ModelLodAsset>& lodAsset);

            ~ModelLod() = default;

            //! Blocks the CPU until pending buffer uploads have completed.
            void WaitForUpload();

            AZStd::array_view<Mesh> GetMeshes() const;

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
                ModelLod::StreamBufferViewList& streamBufferViewsOut,
                UvStreamTangentBitmask* uvStreamTangentBitmaskOut,
                const ShaderInputContract& contract,
                size_t meshIndex,
                const MaterialModelUvOverrideMap& materialModelUvMap = {},
                const MaterialUvNameMap& materialUvNameMap = {}) const;

        private:
            ModelLod() = default;

            static Data::Instance<ModelLod> CreateInternal(ModelLodAsset& lodAsset);
            RHI::ResultCode Init(ModelLodAsset& lodAsset);

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

            uint32_t m_loadedBuffersCount = 0;

            // Tracks whether buffers have all been streamed up to the GPU.
            bool m_isUploadPending = false;

            AZStd::mutex m_callbackMutex;
        };

        //! An encoded bitmask for tangent used by UV streams.
        //! It contains the information about number of UV streams and which tangent/bitangent is used by each UV stream.
        //! See m_mask for more details.
        //! The mask will be passed through per draw SRG.
        class UvStreamTangentBitmask
        {
        public:
            //! Get the full mask including number of UVs and tangent/bitangent assignment to each UV.
            uint32_t GetFullTangentBitmask() const;

            //! Get number of UVs that have tangent/bitangent assigned.
            uint32_t GetUvStreamCount() const;

            //! Get tangent/bitangent assignment to the specified UV in the material.
            //! @param uvIndex the index of the UV from the material, in default order as in the shader code.
            uint32_t GetTangentAtUv(uint32_t uvIndex) const;

            //! Apply the tangent to the next UV, whose index is the same as GetUvStreamCount.
            //! @param tangent the tangent/bitangent to be assigned. Ranged in [0, 0xF)
            //!        It comes from the model in order, e.g. 0 means the first available tangent stream from the model.
            //!        Specially, value 0xF(=UnassignedTangent) means generated tangent/bitangent will be used in shader.
            //!        If ranged out of definition, unassigned tangent will be applied.
            void ApplyTangent(uint32_t tangent);

            //! Reset the bitmask to clear state.
            void Reset();

            //! The bit mask indicating generated tangent/bitangent will be used.
            static constexpr uint32_t UnassignedTangent = 0b1111u;

            //! The variable name defined in the SRG shader code.
            static constexpr const char* SrgName = "m_uvStreamTangentBitmask";
        private:
            //! Mask composition:
            //! The number of UV slots (highest 4 bits) + tangent mask (4 bits each) * 7
            //! e.g. 0x200000F0 means there are 2 UV streams,
            //!      the first UV stream uses 0th tangent stream (0x0),
            //!      the second UV stream uses the generated tangent stream (0xF).
            uint32_t m_mask = 0;

            //! Bit size in the mask composition.
            static constexpr uint32_t BitsPerTangent = 4;
            static constexpr uint32_t BitsForUvIndex = 4;

        public:
            //! Max UV slots available in this bit mask.
            static constexpr uint32_t MaxUvSlots = (sizeof(m_mask) * CHAR_BIT - BitsForUvIndex) / BitsPerTangent;
        };
    } // namespace RPI
} // namespace AZ
