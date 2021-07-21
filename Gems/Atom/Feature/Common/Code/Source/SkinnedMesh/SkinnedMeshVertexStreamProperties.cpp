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
                RHI::ShaderSemantic{Name{"UNUSED"}}
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::Normal)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                Name{"SkinnedMeshInputNormals"},
                Name{"m_sourceNormals"},
                RHI::ShaderSemantic{Name{"UNUSED"}}
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::Tangent)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32B32A32_FLOAT,
                sizeof(AZ::Vector4),
                Name{"SkinnedMeshInputTangents"},
                Name{"m_sourceTangents"},
                RHI::ShaderSemantic{Name{"UNUSED"}}
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::BiTangent)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32B32_FLOAT,
                sizeof(AZ::PackedVector3f),
                Name{"SkinnedMeshInputBiTangents"},
                Name{"m_sourceBiTangents"},
                RHI::ShaderSemantic{Name{"UNUSED"}}
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::BlendIndices)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32B32A32_UINT,
                sizeof(AZ::Vector4),
                Name{"SkinnedMeshInputBlendIndices"},
                Name{"m_sourceBlendIndices"},
                RHI::ShaderSemantic{Name{"UNUSED"}}
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::BlendWeights)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32B32A32_FLOAT,
                sizeof(AZ::Vector4),
                Name{"SkinnedMeshInputBlendWeights"},
                Name{"m_sourceBlendWeights"},
                RHI::ShaderSemantic{Name{"UNUSED"}}
            };

            m_inputStreamInfo[static_cast<uint8_t>(SkinnedMeshInputVertexStreams::Color)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32B32A32_FLOAT,
                sizeof(AZ::Vector4),
                Name{"SkinnedMeshInputColors"},
                Name{"m_sourceColors"},
                RHI::ShaderSemantic{Name{"UNUSED"}}
            };

            // Attributes of the vertex buffers that are not used or modified during skinning, but are shared between all target models that share the same source
            m_staticStreamInfo[static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::UV_0)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32_FLOAT,
                sizeof(float[2]),
                Name{"SkinnedMeshStaticUVs"},
                Name{"unused"},
                RHI::ShaderSemantic{Name{"UV"}}
            };

            m_staticStreamInfo[static_cast<uint8_t>(SkinnedMeshStaticVertexStreams::Color)] = SkinnedMeshVertexStreamInfo{
                RHI::Format::R32G32B32A32_FLOAT,
                sizeof(AZ::Vector4),
                Name{"SkinnedMeshStaticColors"},
                Name{"unused"},
                RHI::ShaderSemantic{Name{"COLOR"}}
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

            m_outputStreamInfo[static_cast<uint8_t>(SkinnedMeshOutputVertexStreams::Color)] = SkinnedMeshOutputVertexStreamInfo{
                RHI::Format::R32G32B32A32_FLOAT,
                sizeof(AZ::Vector4),
                Name{"SkinnedMeshOutputColors"},
                Name{"m_targetColors"},
                RHI::ShaderSemantic{Name{"COLOR"}},
                SkinnedMeshInputVertexStreams::Color
            };
            
            {
                auto bufferPoolDesc = AZStd::make_unique<RHI::BufferPoolDescriptor>();
                bufferPoolDesc->m_bindFlags = RHI::BufferBindFlags::ShaderRead;
                // Skinning input buffers have read-only access
                bufferPoolDesc->m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;

                RPI::ResourcePoolAssetCreator creator;
                creator.Begin(Uuid::CreateRandom());
                creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
                creator.SetPoolName("SkinnedMeshInputStreamPool");
                creator.End(m_inputStreamResourcePool);
            }

            {
                auto bufferPoolDesc = AZStd::make_unique<RHI::BufferPoolDescriptor>();
                // Static buffers that don't change during skinning are used strictly as input assembly buffers
                bufferPoolDesc->m_bindFlags = RHI::BufferBindFlags::InputAssembly;
                bufferPoolDesc->m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;

                RPI::ResourcePoolAssetCreator creator;
                creator.Begin(Uuid::CreateRandom());
                creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
                creator.SetPoolName("SkinnedMeshStaticStreamPool");
                creator.End(m_staticStreamResourcePool);
            }

            {
                auto bufferPoolDesc = AZStd::make_unique<RHI::BufferPoolDescriptor>();
                // Output buffers are both written to during skinning and used as input assembly buffers
                bufferPoolDesc->m_bindFlags = RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::ShaderReadWrite;
                bufferPoolDesc->m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;

                RPI::ResourcePoolAssetCreator creator;
                creator.Begin(Uuid::CreateRandom());
                creator.SetPoolDescriptor(AZStd::move(bufferPoolDesc));
                creator.SetPoolName("SkinnedMeshOutputStreamPool");
                creator.End(m_outputStreamResourcePool);
            }
        };

        const SkinnedMeshVertexStreamInfo& SkinnedMeshVertexStreamProperties::GetInputStreamInfo(SkinnedMeshInputVertexStreams stream) const
        {
            return m_inputStreamInfo[static_cast<uint8_t>(stream)];
        }

        const SkinnedMeshVertexStreamInfo& SkinnedMeshVertexStreamProperties::GetStaticStreamInfo(SkinnedMeshStaticVertexStreams stream) const
        {
            return m_staticStreamInfo[static_cast<uint8_t>(stream)];
        }

        const SkinnedMeshOutputVertexStreamInfo& SkinnedMeshVertexStreamProperties::GetOutputStreamInfo(SkinnedMeshOutputVertexStreams stream) const
        {
            return m_outputStreamInfo[static_cast<uint8_t>(stream)];
        }

        Data::Asset<RPI::ResourcePoolAsset> SkinnedMeshVertexStreamProperties::GetInputStreamResourcePool() const
        {
            return m_inputStreamResourcePool;
        }

        Data::Asset<RPI::ResourcePoolAsset> SkinnedMeshVertexStreamProperties::GetStaticStreamResourcePool() const
        {
            return m_staticStreamResourcePool;
        }

        Data::Asset<RPI::ResourcePoolAsset> SkinnedMeshVertexStreamProperties::GetOutputStreamResourcePool() const
        {
            return m_outputStreamResourcePool;
        }

        uint32_t SkinnedMeshVertexStreamProperties::GetMaxSupportedVertexCount() const
        {
            return aznumeric_cast<uint32_t>(std::numeric_limits<uint16_t>::max()) * aznumeric_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
        }

    }// namespace Render
}// namespace AZ
