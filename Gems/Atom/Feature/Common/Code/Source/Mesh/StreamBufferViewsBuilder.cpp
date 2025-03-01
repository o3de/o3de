/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StreamBufferViewsBuilder.h"

namespace AZ
{
    namespace Render
    {
#define DEBUG_LOG_BUFFERVIEWS (0)

        class ShaderStreamBufferViews final : public ShaderStreamBufferViewsInterface
        {
        public:
            AZ_RTTI(
                AZ::Render::ShaderStreamBufferViews,
                "{35C88638-C8F8-4124-B7AD-269ED7BFE6BE}",
                AZ::Render::ShaderStreamBufferViewsInterface);
            ShaderStreamBufferViews() = delete;
            explicit ShaderStreamBufferViews(uint32_t lodIndex, uint32_t meshIndex)
                : m_lodIndex(lodIndex), m_meshIndex(meshIndex)
            {
            }
            ~ShaderStreamBufferViews() = default;

            ////////////////////////////////////////////////////////////
            // ShaderStreamBufferViewsInterface overrides Start...
            const AZ::RHI::Ptr<AZ::RHI::BufferView>& GetVertexIndicesBufferView() const override
            {
                return m_vertexIndicesBufferView;
            }

            const AZ::RHI::IndexBufferView& GetVertexIndicesIndexBufferView() const override
            {
                return m_vertexIndicesIndexBufferView;
            }

            uint32_t GetVertexIndicesBindlessReadIndex(int deviceIndex) const override
            {
                if (!m_vertexIndicesBufferView)
                {
                    return AZ::RHI::DeviceBufferView::InvalidBindlessIndex;
                }
                return m_vertexIndicesBufferView->GetBindlessIndices(deviceIndex);
            }

            const AZ::RHI::Ptr<AZ::RHI::BufferView>& GetBufferView(const AZ::RHI::ShaderSemantic& shaderSemantic) const override
            {
                static AZ::RHI::Ptr<AZ::RHI::BufferView> InvalidBufferView;
                if (!m_bufferViewsBySemantic.contains(shaderSemantic))
                {
                    return InvalidBufferView;
                }
                return m_bufferViewsBySemantic.at(shaderSemantic);
            }

            const AZ::RHI::Ptr<AZ::RHI::BufferView>& GetBufferView(const char* semanticName) const override
            {
                return GetBufferView(AZ::RHI::ShaderSemantic::Parse(semanticName));
            }

            const AZ::RHI::StreamBufferView* GetStreamBufferView(const AZ::RHI::ShaderSemantic& shaderSemantic) const override
            {
                if (!m_streamBufferViewsBySemantic.contains(shaderSemantic))
                {
                    return nullptr;
                }
                return &(m_streamBufferViewsBySemantic.at(shaderSemantic));
            }

            const AZ::RHI::StreamBufferView* GetStreamBufferView(const char* semanticName) const override
            {
                return GetStreamBufferView(AZ::RHI::ShaderSemantic::Parse(semanticName));
            }

            uint32_t GetStreamBufferViewBindlessReadIndex(int deviceIndex, const AZ::RHI::ShaderSemantic& shaderSemantic) const override
            {
                if (!m_bufferViewsBySemantic.contains(shaderSemantic))
                {
                    return AZ::RHI::DeviceBufferView::InvalidBindlessIndex;
                }
                return m_bufferViewsBySemantic.at(shaderSemantic)->GetBindlessIndices(deviceIndex);
            }

            uint32_t GetStreamBufferViewBindlessReadIndex(int deviceIndex, const char* semanticName) const override
            {
                return GetStreamBufferViewBindlessReadIndex(deviceIndex, AZ::RHI::ShaderSemantic::Parse(semanticName));
            }

            uint32_t GetLodIndex() const override
            {
                return m_lodIndex;
            }

            uint32_t GetMeshIndex() const override
            {
                return m_meshIndex;
            }
            // ShaderStreamBufferViewsInterface overrides End...
            ////////////////////////////////////////////////////////////

        private:
            friend class ShaderStreamBufferViewsBuilder;
            uint32_t m_lodIndex; //Kept for informational purposes.
            uint32_t m_meshIndex; //Kept for informational purposes.

            // This represents a View to the buffer that contains all the vertex indices
            // loaded in GPU memory. If the BindlessSrg is enabled, which typically is the case,
            // you can query its BindlessSrg Read index.
            // REMARK: Typically this BufferView will be the same for all subMeshes because
            // we always map the whole range as defined in its AZ::RHI::Buffer, and this is done
            // because most RHI require the Offset of a bufferView to be aligned to 16 bytes, which
            // is not often the case for a SubMesh. The workaround is to map the whole buffer with offset 0,
            // and the developer must feed the offset as a shader constant. The actual offset can be queried
            // from @m_indexStreamBufferView.
            AZ::RHI::Ptr<AZ::RHI::BufferView> m_vertexIndicesBufferView;
            // This works as a descriptor of Offset, number of bytes, etc, which is necessary
            // when (@m_meshIndex > 0) because the Offset within @m_indexBufferView is different than zero.
            // Typically the offset returned by @m_indexStreamBufferView.GetOffset() must be loaded as a shader constant
            // so the shader code knows how to read the indices at the correct offset from a ByteAddressBuffer.
            AZ::RHI::IndexBufferView m_vertexIndicesIndexBufferView;

            // This is the main dictionary of BufferViews which typically will be the same across all sub meshes.
            // The reason it is the same across all sub meshes is because most RHI require the Offset of a bufferView to be aligned to 16 bytes, which
            // is not often the case for a SubMesh. The workaround is to map the whole buffer with offset 0,
            // and the developer must feed the offset as a shader constant. The actual offset can be queried
            // from @m_streamBufferViewsBySemantic.
            using BufferViewsBySemantic = AZStd::unordered_map<AZ::RHI::ShaderSemantic, AZ::RHI::Ptr<AZ::RHI::BufferView>>;
            BufferViewsBySemantic m_bufferViewsBySemantic;

            // This works as a descriptor of Offset, number of bytes, etc, which is necessary
            // when (@m_meshIndex > 0) because the Offset within @m_bufferViewsBySemantic is different than zero.
            // Typically the offset returned by @m_streamBufferViewsBySemantic.GetOffset() must be loaded as a shader constant
            // so the shader code knows how to read the indices at the correct offset from a ByteAddressBuffer.
            using StreamBufferViewsBySemantic = AZStd::unordered_map<AZ::RHI::ShaderSemantic, AZ::RHI::StreamBufferView>;
            StreamBufferViewsBySemantic m_streamBufferViewsBySemantic;
        }; // class ShaderStreamBufferViews


        ShaderStreamBufferViewsBuilder::ShaderStreamBufferViewsBuilder(const AZ::Render::MeshFeatureProcessorInterface::MeshHandle& meshHandle)
            : m_meshHandle(&meshHandle)
        {
        }

        ////////////////////////////////////////////////////////////
        // StreamBufferViewsBuilderInterface overrides Start...
        bool ShaderStreamBufferViewsBuilder::AddStream(const char* semanticName, AZ::RHI::Format streamFormat, bool isOptional)
        {
            if (m_shaderInputContract)
            {
                AZ_Error(LogWindow, false, "Can not add stream '%s' because the ShaderInputContract was finalized!", semanticName);
                return false;
            }
            auto it = std::find_if(
                m_streamsList.cbegin(),
                m_streamsList.cend(),
                [semanticName](const StreamInfo& item) -> bool
                {
                    return strcmp(item.m_semanticName, semanticName) == 0;
                });
            if (it != m_streamsList.end())
            {
                AZ_Error(LogWindow, false, "%s. A stream with name '%s' already exists!", __FUNCTION__, semanticName);
                return false;
            }
            m_streamsList.emplace_back(semanticName, streamFormat, isOptional);
            return true;
        }

        AZ::u8 ShaderStreamBufferViewsBuilder::GetStreamCount() const
        {
            return aznumeric_caster(m_streamsList.size());
        }

        AZStd::unique_ptr<ShaderStreamBufferViewsInterface> ShaderStreamBufferViewsBuilder::BuildShaderStreamBufferViews(
            uint32_t lodIndex, uint32_t meshIndex)
        {
            if (!m_shaderInputContract)
            {
                FinalizeShaderInputContract();
            }

            AZStd::unique_ptr<ShaderStreamBufferViews> shaderStreamBufferViews = AZStd::make_unique<ShaderStreamBufferViews>(lodIndex, meshIndex);

            const auto& modelInstance = (*m_meshHandle)->GetModel();
            if (!modelInstance)
            {
                // A valid MeshHandle, when the Mesh Asset is being loaded, may temporary not have a model instance.
                return shaderStreamBufferViews;
            }

            const auto& modelLods = modelInstance->GetLods();
            const auto& modelLod = modelLods[lodIndex];
            const auto& modelLodMeshList = modelLod->GetMeshes();
            const auto& modelLodMesh = modelLodMeshList[meshIndex];

            shaderStreamBufferViews->m_vertexIndicesBufferView =
                BuildShaderIndexBufferView(modelLodMesh, shaderStreamBufferViews->m_vertexIndicesIndexBufferView);

            // retrieve the material
            const AZ::Render::CustomMaterialId customMaterialId(lodIndex, modelLodMesh.m_materialSlotStableId);
            // This is a private function!
            const auto& customMaterialInfo = (*m_meshHandle)->GetCustomMaterialWithFallback(customMaterialId);
            const auto& material = customMaterialInfo.m_material ? customMaterialInfo.m_material : modelLodMesh.m_material;

            // retrieve vertex/index buffers
            AZ::RHI::InputStreamLayout inputStreamLayout;
            AZ::RHI::StreamBufferIndices streamIndices;

            [[maybe_unused]] bool result = modelLod->GetStreamsForMesh(
                inputStreamLayout,
                streamIndices,
                nullptr,
                *m_shaderInputContract.get(),
                meshIndex,
                customMaterialInfo.m_uvMapping,
                material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap());
            AZ_Assert(result, "Failed to retrieve mesh stream buffer views");

            const AZ::u8 streamCount = GetStreamCount();
            auto streamIter = modelLodMesh.CreateStreamIterator(streamIndices);
            auto streamChannels = inputStreamLayout.GetStreamChannels();
            for (AZ::u8 streamIdx = 0; streamIdx < streamCount; streamIdx++)
            {
                auto shaderSemantic = streamChannels[streamIdx].m_semantic;
                AZ::RHI::Buffer* rhiBuffer = const_cast<AZ::RHI::Buffer*>(streamIter[streamIdx].GetBuffer());

                // REMARK: The reason we are not doing this, is that most RHIs need the Offset in a BufferView to be aligned to 16 bytes,
                // which is not case for most sub meshes. To avoid this potential error, we map the whole buffer,
                // and expect the developer to use ShaderStreamBufferViews::GetStreamBufferView() to get the actual offset and feed it as a shader constant.
                // const uint32_t streamByteOffset = streamIter[streamIdx].GetByteOffset();
                // const uint32_t streamByteCount = streamIter[streamIdx].GetByteCount();
                const uint32_t streamByteOffset = 0;
                const uint32_t streamByteCount = static_cast<uint32_t>(rhiBuffer->GetDescriptor().m_byteCount);

                auto bufferViewDescriptor = AZ::RHI::BufferViewDescriptor::CreateRaw(streamByteOffset, streamByteCount);
                auto ptrBufferView = rhiBuffer->GetBufferView(bufferViewDescriptor);

                shaderStreamBufferViews->m_bufferViewsBySemantic.emplace(shaderSemantic, ptrBufferView);
                shaderStreamBufferViews->m_streamBufferViewsBySemantic.emplace(shaderSemantic, streamIter[streamIdx]);

#if DEBUG_LOG_BUFFERVIEWS
                AZ_Printf(
                    "ShaderStreamBufferViewsBuilder",
                    "subMesh[%u] semantic[%s] viewByteOffset=%u, viewByteCount=%u, bufferByteCount=%u.\n",
                    meshIndex,
                    shaderSemantic.ToString().c_str(),
                    streamIter[streamIdx].GetByteOffset(),
                    streamIter[streamIdx].GetByteCount(),
                    streamByteCount);
#endif
            }

            return shaderStreamBufferViews;
        }

        const MeshFeatureProcessorInterface::MeshHandle& ShaderStreamBufferViewsBuilder::GetMeshHandle() const
        {
            return *m_meshHandle;
        }
        // StreamBufferViewsBuilderInterface overrides End...
        ////////////////////////////////////////////////////////////

        void ShaderStreamBufferViewsBuilder::FinalizeShaderInputContract()
        {
            AZ_Assert(!m_shaderInputContract, "ShaderInputContract was already finalized.");
            m_shaderInputContract = AZStd::make_unique<AZ::RPI::ShaderInputContract>();
            for (const auto& streamInfo : m_streamsList)
            {
                AZ::RPI::ShaderInputContract::StreamChannelInfo streamChannelInfo;
                streamChannelInfo.m_semantic = AZ::RHI::ShaderSemantic::Parse(streamInfo.m_semanticName);
                streamChannelInfo.m_componentCount = AZ::RHI::GetFormatComponentCount(streamInfo.m_streamFormat);
                streamChannelInfo.m_isOptional = streamInfo.m_isOptional;
                m_shaderInputContract->m_streamChannels.emplace_back(streamChannelInfo);
            }
        }

        AZ::RHI::Ptr<AZ::RHI::BufferView> ShaderStreamBufferViewsBuilder::BuildShaderIndexBufferView(
            const AZ::RPI::ModelLod::Mesh& modelLodMesh, AZ::RHI::IndexBufferView& indexBufferViewOut) const
        {
            AZ_Assert(modelLodMesh.GetDrawArguments().m_type == AZ::RHI::DrawType::Indexed, "We only support indexed geometry!");
            const AZ::RHI::IndexBufferView& indexBufferView = modelLodMesh.GetIndexBufferView();
            indexBufferViewOut = indexBufferView;

            uint32_t indexElementSize = indexBufferView.GetIndexFormat() == AZ::RHI::IndexFormat::Uint16 ? 2 : 4;

            // REMARK: The reason we are not doing this, is that most RHIs need the Offset in a BufferView to be aligned to 16 bytes,
            // which is not case for most sub meshes. To avoid this potential error, we map the whole buffer,
            // and expect the developer to use @indexBufferViewOut to get the actual offset and feed it as a shader constant.
            // uint32_t indexElementOffset = indexBufferView.GetByteOffset() / indexElementSize;
            // uint32_t indexElementCount = indexBufferView.GetByteCount() / indexElementSize;
            uint32_t indexElementOffset = 0;
            uint32_t indexElementCount = (uint32_t)indexBufferView.GetBuffer()->GetDescriptor().m_byteCount / indexElementSize;

            AZ::RHI::BufferViewDescriptor indexBufferDescriptor;
            indexBufferDescriptor.m_elementOffset = indexElementOffset;
            indexBufferDescriptor.m_elementCount = indexElementCount;
            indexBufferDescriptor.m_elementSize = indexElementSize;
            indexBufferDescriptor.m_elementFormat =
                indexBufferView.GetIndexFormat() == AZ::RHI::IndexFormat::Uint16 ? AZ::RHI::Format::R16_UINT : AZ::RHI::Format::R32_UINT;
            auto* rhiBuffer = const_cast<AZ::RHI::Buffer*>(indexBufferView.GetBuffer());

#if DEBUG_LOG_BUFFERVIEWS
            AZ_Printf(
                "ShaderStreamBufferViewsBuilder",
                "Index buffer viewByteOffset=%u, viewByteCount=%u, elementCount=%u, elementSize=%u.\n",
                indexBufferView.GetByteOffset(),
                indexBufferView.GetByteCount(),
                indexElementCount,
                indexElementSize);
#endif

            return rhiBuffer->GetBufferView(indexBufferDescriptor);
        }

    } // namespace Render
} // namespace AZ
