/*/
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/Buffer.h>
#include <Atom/Feature/Mesh/MeshInfo.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/ShaderSemantic.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Model/UvStreamTangentBitmask.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>


namespace AZ
{
    namespace Render
    {

        using MeshInfoHandle = RHI::Handle<int32_t>;
        using MeshInfoHandleList = AZStd::vector<MeshInfoHandle>;

        // Utility to access the Geometry - Data from a mesh without the input-assembly
        struct BufferViewIndexAndOffset
        {
            // Streambuffer-view and format
            AZ::RHI::StreamBufferView m_streamBufferView;
            RHI::Format m_streamBufferFormat;

            // Buffer-View, offset and bindless Read index for the same data, needed to access the data with the MeshInfo-indices
            RHI::Ptr<AZ::RHI::BufferView> m_bufferView;
            uint32_t m_byteOffset;
            AZStd::unordered_map<int, uint32_t> m_bindlessReadIndex;

            // utility function to create an entry from a generic RHI buffer
            static BufferViewIndexAndOffset Create(const RHI::Buffer* rhiBuffer, const uint32_t byteOffset)
            {
                BufferViewIndexAndOffset result;
                uint32_t byteCount = rhiBuffer->GetDescriptor().m_byteCount;

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
            static BufferViewIndexAndOffset Create(const RHI::StreamBufferView& streamBufferView, const RHI::Format format)
            {
                auto rhiBuffer = streamBufferView.GetBuffer();
                auto result = BufferViewIndexAndOffset::Create(rhiBuffer, streamBufferView.GetByteOffset());
                result.m_streamBufferFormat = format;
                result.m_streamBufferView = streamBufferView;
                return result;
            }
        };

        // Utility to access the Indices for the Geometry - Data from a mesh without the input-assembly
        struct IndexBufferViewIndexAndOffset
        {
            // Indexbuffer-view and format
            AZ::RHI::IndexBufferView m_indexBufferView;
            RHI::IndexFormat m_indexBufferFormat;

            // Buffer-View, offset and bindless Read index for the same data, needed to access the data with the MeshInfo-indices
            RHI::Ptr<AZ::RHI::BufferView> m_bufferView;
            uint32_t m_byteOffset;
            AZStd::unordered_map<int, uint32_t> m_bindlessReadIndex;

            // utility function to create an entry from an IndexBufferView
            static IndexBufferViewIndexAndOffset Create(const AZ::RHI::IndexBufferView& indexBufferView)
            {
                // The 'raw' bufferview is for a ByteAddresBuffer, which has to be R32_UINT.
                RHI::BufferViewDescriptor desc =
                    RHI::BufferViewDescriptor::CreateRaw(0, indexBufferView.GetBuffer()->GetDescriptor().m_byteCount);

                // multi-device buffer bindless read index and offset
                IndexBufferViewIndexAndOffset result{};
                result.m_bufferView = indexBufferView.GetBuffer()->GetBufferView(desc);
                result.m_bindlessReadIndex = result.m_bufferView->GetBindlessReadIndex();
                result.m_byteOffset = indexBufferView.GetByteOffset();
                result.m_indexBufferView = indexBufferView;
                result.m_indexBufferFormat = indexBufferView.GetIndexFormat();

                return result;
            }
        };

        // Data for for the MeshInfo - entries of one Mesh
        struct MeshInfoEntry : public AZStd::intrusive_base
        {
            // Info from the mesh about the geometry buffers
            RPI::UvStreamTangentBitmask m_streamTangentBitmask;
            RPI::ShaderOptionGroup m_optionalInputStreamShaderOptions;

            // Geometry buffers and index buffer
            AZStd::unordered_map<RHI::ShaderSemantic, BufferViewIndexAndOffset> m_meshBuffers;
            IndexBufferViewIndexAndOffset m_indexBuffer;

            // additional data per mesh
            int32_t m_materialTypeId;
            int32_t m_materialInstanceId;
            uint32_t m_lightingChannels;
            uint32_t m_objectIdForTransform;
            bool m_isSkinnedMesh;
        };

    } // namespace Render
} // namespace AZ
