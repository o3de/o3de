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
            const AZ::RHI::Ptr<AZ::RHI::BufferView>& GetIndexBufferView() const override
            {
                return m_indexBufferView;
            }

            const AZ::RHI::Ptr<AZ::RHI::BufferView>& GetStreamBufferView(const AZ::RHI::ShaderSemantic& shaderSemantic) const override
            {
                static AZ::RHI::Ptr<AZ::RHI::BufferView> InvalidBufferView;
                if (!m_streamViewsBySemantic.contains(shaderSemantic))
                {
                    return InvalidBufferView;
                }
                return m_streamViewsBySemantic.at(shaderSemantic);
            }

            const AZ::RHI::Ptr<AZ::RHI::BufferView>& GetStreamBufferView(const char* semanticName) const override
            {
                return GetStreamBufferView(AZ::RHI::ShaderSemantic::Parse(semanticName));
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
            AZ::RHI::Ptr<AZ::RHI::BufferView> m_indexBufferView;
            using StreamBufferViewsBySemantic = AZStd::unordered_map<AZ::RHI::ShaderSemantic, AZ::RHI::Ptr<AZ::RHI::BufferView>>;
            StreamBufferViewsBySemantic m_streamViewsBySemantic;
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

            shaderStreamBufferViews->m_indexBufferView = BuildShaderIndexBufferView(modelLodMesh);

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
                uint32_t streamByteCount = static_cast<uint32_t>(rhiBuffer->GetDescriptor().m_byteCount);
                auto bufferViewDescriptor = AZ::RHI::BufferViewDescriptor::CreateRaw(0, streamByteCount);
                auto ptrBufferView = rhiBuffer->BuildBufferView(bufferViewDescriptor);
                shaderStreamBufferViews->m_streamViewsBySemantic.emplace(AZStd::move(shaderSemantic), ptrBufferView);
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
            const AZ::RPI::ModelLod::Mesh& modelLodMesh) const
        {
            AZ_Assert(modelLodMesh.GetDrawArguments().m_type == AZ::RHI::DrawType::Indexed, "We only support indexed geometry!");
            const AZ::RHI::IndexBufferView& indexBufferView = modelLodMesh.GetIndexBufferView();

            uint32_t indexElementSize = indexBufferView.GetIndexFormat() == AZ::RHI::IndexFormat::Uint16 ? 2 : 4;
            uint32_t indexElementCount = (uint32_t)indexBufferView.GetBuffer()->GetDescriptor().m_byteCount / indexElementSize;
            AZ::RHI::BufferViewDescriptor indexBufferDescriptor;
            indexBufferDescriptor.m_elementOffset = 0;
            indexBufferDescriptor.m_elementCount = indexElementCount;
            indexBufferDescriptor.m_elementSize = indexElementSize;
            indexBufferDescriptor.m_elementFormat =
                indexBufferView.GetIndexFormat() == AZ::RHI::IndexFormat::Uint16 ? AZ::RHI::Format::R16_UINT : AZ::RHI::Format::R32_UINT;
            auto* rhiBuffer = const_cast<AZ::RHI::Buffer*>(indexBufferView.GetBuffer());

            return rhiBuffer->BuildBufferView(indexBufferDescriptor);
        }

    } // namespace Render
} // namespace AZ
