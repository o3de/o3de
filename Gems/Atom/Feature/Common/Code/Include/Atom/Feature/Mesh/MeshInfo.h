/*/
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/IndexFormat.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RHI.Reflect/VertexFormat.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Model/UvStreamTangentBitmask.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

namespace AZ::Render
{
    using MeshInfoHandle = RHI::Handle<int32_t>;
    using MeshInfoHandleList = AZStd::vector<MeshInfoHandle>;

    // Utility to access the Geometry - Data from a mesh without the input-assembly
    struct BufferViewIndexAndOffset
    {
        // Streambuffer-view and format
        AZ::RHI::StreamBufferView m_streamBufferView{};
        RHI::VertexFormat m_vertexFormat = RHI::VertexFormat::Unknown;

        // Buffer-View, offset and bindless Read index for the same data, needed to access the data with the MeshInfo-indices
        RHI::Ptr<AZ::RHI::BufferView> m_bufferView = nullptr;
        uint32_t m_byteOffset = 0;
        AZStd::unordered_map<int, uint32_t> m_bindlessReadIndex = {};

        // utility function to create an entry from a generic RHI buffer
        static BufferViewIndexAndOffset Create(RHI::Buffer* rhiBuffer, const uint32_t byteOffset)
        {
            BufferViewIndexAndOffset result;
            uint32_t byteCount = static_cast<uint32_t>(rhiBuffer->GetDescriptor().m_byteCount);

            RHI::BufferViewDescriptor desc = RHI::BufferViewDescriptor::CreateRaw(0, byteCount);
            if (byteCount > 0)
            {
                result.m_bufferView = rhiBuffer->GetBufferView(desc);
                result.m_bindlessReadIndex = result.m_bufferView->GetBindlessReadIndex();
                result.m_byteOffset = byteOffset;
            }
            return result;
        };
        // utility function to create an entry from a Streambuffer
        static BufferViewIndexAndOffset Create(RHI::StreamBufferView& streamBufferView, const RHI::VertexFormat vertexFormat)
        {
            auto rhiBuffer = const_cast<RHI::Buffer*>(streamBufferView.GetBuffer());
            auto result = BufferViewIndexAndOffset::Create(rhiBuffer, streamBufferView.GetByteOffset());
            result.m_vertexFormat = vertexFormat;
            result.m_streamBufferView = streamBufferView;
            return result;
        }
    };

    // Utility to access the Indices for the Geometry - Data from a mesh without the input-assembly
    struct IndexBufferViewIndexAndOffset
    {
        // Indexbuffer-view and format
        AZ::RHI::IndexBufferView m_indexBufferView{};
        RHI::IndexFormat m_indexFormat = RHI::IndexFormat::Unknown;

        // Buffer-View, offset and bindless Read index for the same data, needed to access the data with the MeshInfo-indices
        RHI::Ptr<AZ::RHI::BufferView> m_bufferView = nullptr;
        uint32_t m_byteOffset = 0;
        AZStd::unordered_map<int, uint32_t> m_bindlessReadIndex = {};

        // utility function to create an entry from an IndexBufferView
        static IndexBufferViewIndexAndOffset Create(AZ::RHI::IndexBufferView& indexBufferView)
        {
            uint32_t byteCount = static_cast<uint32_t>(indexBufferView.GetBuffer()->GetDescriptor().m_byteCount);
            // The 'raw' bufferview is for a ByteAddresBuffer, which has to be R32_UINT.
            RHI::BufferViewDescriptor desc = RHI::BufferViewDescriptor::CreateRaw(0, byteCount);

            auto* rhiBuffer = const_cast<RHI::Buffer*>(indexBufferView.GetBuffer());

            // multi-device buffer bindless read index and offset
            IndexBufferViewIndexAndOffset result{};
            result.m_bufferView = rhiBuffer->GetBufferView(desc);
            result.m_bindlessReadIndex = result.m_bufferView->GetBindlessReadIndex();
            result.m_byteOffset = indexBufferView.GetByteOffset();
            result.m_indexBufferView = indexBufferView;
            result.m_indexFormat = indexBufferView.GetIndexFormat();

            return result;
        }
    };

    // Data for for the MeshInfo - entries of one Mesh
    struct MeshInfoEntry : public AZStd::intrusive_base
    {
        // Info from the mesh about the geometry buffers
        RPI::UvStreamTangentBitmask m_streamTangentBitmask{};
        RPI::ShaderOptionGroup m_optionalInputStreamShaderOptions{};

        // Geometry buffers and index buffer
        AZStd::unordered_map<RHI::ShaderSemantic, BufferViewIndexAndOffset> m_meshBuffers{};
        IndexBufferViewIndexAndOffset m_indexBuffer{};

        // additional data per mesh
        int32_t m_materialTypeId = -1;
        int32_t m_materialInstanceId = -1;
        uint32_t m_lightingChannels = 0;
        uint32_t m_objectIdForTransform = 0;
        bool m_isSkinnedMesh = false;
    };
} // namespace AZ::Render
