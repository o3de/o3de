/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkinnedMesh/SkinnedMeshVertexStreamProperties.h>

#include <Atom/RPI.Reflect/ResourcePoolAssetCreator.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RHI/Factory.h>

#include <AzCore/Math/PackedVector3.h>

namespace AZ
{
    namespace Render
    {
        SkinnedMeshVertexStreamProperties::SkinnedMeshVertexStreamProperties()
        {
            // Attributes of the input buffers used for skinning
            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::Position)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                Name{"SkinnedMeshInputPositions"},
                Name{"m_sourcePositions"},
                RHI::ShaderSemantic{Name{"POSITION"}},
                false, // isOptional
                SkinnedMeshInputVertexStreams::Position
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::Normal)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                Name{"SkinnedMeshInputNormals"},
                Name{"m_sourceNormals"},
                RHI::ShaderSemantic{Name{"NORMAL"}},
                false, // isOptional
                SkinnedMeshInputVertexStreams::Normal
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::Tangent)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32B32A32_FLOAT,
                sizeof(AZ::Vector4),
                Name{"SkinnedMeshInputTangents"},
                Name{"m_sourceTangents"},
                RHI::ShaderSemantic{Name{"TANGENT"}},
                false, // isOptional
                SkinnedMeshInputVertexStreams::Tangent
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::BiTangent)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                Name{"SkinnedMeshInputBiTangents"},
                Name{"m_sourceBiTangents"},
                RHI::ShaderSemantic{Name{"BITANGENT"}},
                false, // isOptional
                SkinnedMeshInputVertexStreams::BiTangent
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::BlendIndices)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32_UINT,
                sizeof(AZ::Vector4),
                Name{"SkinnedMeshInputBlendIndices"},
                Name{"m_sourceBlendIndices"},
                RHI::ShaderSemantic{Name{"SKIN_JOINTINDICES"}},
                true, // isOptional
                SkinnedMeshInputVertexStreams::BlendIndices
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::BlendWeights)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32_FLOAT,
                sizeof(AZ::Vector4),
                Name{"SkinnedMeshInputBlendWeights"},
                Name{"m_sourceBlendWeights"},
                RHI::ShaderSemantic{Name{"SKIN_WEIGHTS"}},
                true, // isOptional
                SkinnedMeshInputVertexStreams::BlendWeights
            };

            // Attributes of the vertex buffers that are not used or modified during skinning, but are shared between all target models that share the same source
            m_staticStreamInfo[static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::UV_0)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32_FLOAT,
                sizeof(float[2]),
                Name{"SkinnedMeshStaticUVs"},
                Name{"unused"},
                RHI::ShaderSemantic{Name{"UV"}}
            };

            // Attributes of the vertex streams of the target model that is written to during skinning
            m_outputStreamInfo[static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Position)] = SkinnedMeshOutputVertexStreamInfo{
                RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                Name{"SkinnedMeshOutputPositions"},
                Name{"m_targetPositions"},
                RHI::ShaderSemantic{Name{"POSITION"}},
                SkinnedMeshInputVertexStreams::Position
            };

            m_outputStreamInfo[static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Normal)] = SkinnedMeshOutputVertexStreamInfo{
                RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                Name{"SkinnedMeshOutputNormals"},
                Name{"m_targetNormals"},
                RHI::ShaderSemantic{Name{"NORMAL"}},
                SkinnedMeshInputVertexStreams::Normal
            };

            m_outputStreamInfo[static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Tangent)] = SkinnedMeshOutputVertexStreamInfo{
                RHI::Format::R32G32B32A32_FLOAT,
                sizeof(AZ::Vector4),
                Name{"SkinnedMeshOutputTangents"},
                Name{"m_targetTangents"},
                RHI::ShaderSemantic{Name{"TANGENT"}},
                SkinnedMeshInputVertexStreams::Tangent
            };

            m_outputStreamInfo[static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::BiTangent)] = SkinnedMeshOutputVertexStreamInfo{
                RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                Name{"SkinnedMeshOutputBiTangents"},
                Name{"m_targetBiTangents"},
                RHI::ShaderSemantic{Name{"BITANGENT"}},
                SkinnedMeshInputVertexStreams::BiTangent
            };

            {
                auto bufferPoolDesc = AZStd::make_unique<RHI::BufferPoolDescriptor>();
                // Output buffers are both written to during skinning and used as input assembly buffers
                bufferPoolDesc->m_bindFlags = RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::ShaderReadWrite |
                    RHI::BufferBindFlags::CopyRead | RHI::BufferBindFlags::CopyWrite;
                bufferPoolDesc->m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;

                RPI::ResourcePoolAssetCreator creator;
                creator.Begin(Uuid::CreateRandom());
                creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
                creator.SetPoolName("SkinnedMeshOutputStreamPool");
                creator.End(m_outputStreamResourcePool);
            }

            // Set the required and optional input streams for the skinned mesh compute shader
            // in a ShaderInputContract for retrieving the streams from the model
            for (const SkinnedMeshVertexStreamInfo& inputStreamInfo : m_inputStreamInfo)
            {
                RPI::ShaderInputContract::StreamChannelInfo channelInfo;
                channelInfo.m_semantic = inputStreamInfo.m_semantic;
                channelInfo.m_isOptional = inputStreamInfo.m_isOptional;
                channelInfo.m_componentCount = RHI::GetFormatComponentCount(inputStreamInfo.m_elementFormat);

                m_computeShaderInputContract.m_streamChannels.push_back(channelInfo);
            }
        };

        const SkinnedMeshVertexStreamInfo* SkinnedMeshVertexStreamProperties::GetInputStreamInfo(const RHI::ShaderSemantic& shaderSemantic) const
        {
            auto FindVertexStreamInfo = [&shaderSemantic](const SkinnedMeshVertexStreamInfo& vertexStreamInfo)
            {
                return shaderSemantic == vertexStreamInfo.m_semantic;
            };

            if (auto foundIt = AZStd::find_if(m_inputStreamInfo.begin(), m_inputStreamInfo.end(), FindVertexStreamInfo);
                foundIt != m_inputStreamInfo.end())
            {
                return foundIt;
            }
            else
            {
                return nullptr;
            }
        }

        const SkinnedMeshVertexStreamInfo& SkinnedMeshVertexStreamProperties::GetInputStreamInfo(SkinnedMeshInputVertexStreams stream) const
        {
            return m_inputStreamInfo[static_cast<uint8_t>(stream)];
        }

        const SkinnedMeshVertexStreamInfo& SkinnedMeshVertexStreamProperties::GetStaticStreamInfo(SkinnedMeshStaticVertexStreams stream) const
        {
            return m_staticStreamInfo[static_cast<uint8_t>(stream)];
        }

        const SkinnedMeshOutputVertexStreamInfo* SkinnedMeshVertexStreamProperties::GetOutputStreamInfo(const RHI::ShaderSemantic& shaderSemantic) const
        {
            auto FindVertexStreamInfo = [&shaderSemantic](const SkinnedMeshOutputVertexStreamInfo& vertexStreamInfo)
            {
                return shaderSemantic == vertexStreamInfo.m_semantic;
            };

            if (auto foundIt = AZStd::find_if(m_outputStreamInfo.begin(), m_outputStreamInfo.end(), FindVertexStreamInfo);
                foundIt != m_outputStreamInfo.end())
            {
                return foundIt;
            }
            else
            {
                return nullptr;
            }
        }

        const SkinnedMeshOutputVertexStreamInfo& SkinnedMeshVertexStreamProperties::GetOutputStreamInfo(SkinnedMeshOutputVertexStreams stream) const
        {
            return m_outputStreamInfo[static_cast<uint8_t>(stream)];
        }

        Data::Asset<RPI::ResourcePoolAsset> SkinnedMeshVertexStreamProperties::GetOutputStreamResourcePool() const
        {
            return m_outputStreamResourcePool;
        }

        uint32_t SkinnedMeshVertexStreamProperties::GetMaxSupportedVertexCount() const
        {
            return aznumeric_cast<uint32_t>(std::numeric_limits<uint16_t>::max()) * aznumeric_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
        }

        const RPI::ShaderInputContract& SkinnedMeshVertexStreamProperties::GetComputeShaderInputContract() const
        {
            return m_computeShaderInputContract;
        }

    }// namespace Render
}// namespace AZ
