/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/IndirectBufferLayout.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/IndirectBufferSignature.h>
#include <Atom/RHI/IndirectBufferWriter.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    namespace RHI
    {
        ResultCode IndirectBufferWriter::Init(
            Buffer& buffer, size_t byteOffset, uint32_t byteStride, uint32_t maxCommandSequences, const IndirectBufferSignature& signature)
        {
            if (Validation::IsEnabled())
            {
                if ((byteOffset + maxCommandSequences * byteStride) > buffer.GetDescriptor().m_byteCount)
                {
                    AZ_Assert(false, "Buffer is too small to contain the required commands");
                    return ResultCode::InvalidArgument;
                }
            }

            ResultCode result{ ResultCode::Success };
            auto deviceMask{ buffer.GetDeviceMask() };

            for (auto deviceIndex{ 0 }; deviceMask && (deviceIndex < RHI::RHISystemInterface::Get()->GetDeviceCount());
                 deviceMask >>= 1, ++deviceIndex)
            {
                if (deviceMask & 1)
                {
                    m_deviceIndirectBufferWriter[deviceIndex] = Factory::Get().CreateIndirectBufferWriter();
                    result = m_deviceIndirectBufferWriter[deviceIndex]->Init(
                        buffer.GetDeviceBuffer(deviceIndex).get(),
                        byteOffset,
                        byteStride,
                        maxCommandSequences,
                        *signature.GetDeviceIndirectBufferSignature(deviceIndex).get());

                    if (result != ResultCode::Success)
                        break;
                }
            }

            return result;
        }

        ResultCode IndirectBufferWriter::Init(
            void* memoryPtr, uint32_t byteStride, uint32_t maxCommandSequences, const IndirectBufferSignature& signature)
        {
            if (Validation::IsEnabled())
            {
                if (!memoryPtr)
                {
                    AZ_Assert(false, "Null target memory");
                    return ResultCode::InvalidArgument;
                }
            }

            ResultCode result{ ResultCode::Success };
            auto deviceMask{ signature.GetDeviceMask() };

            for (auto deviceIndex{ 0 }; deviceMask && (deviceIndex < RHI::RHISystemInterface::Get()->GetDeviceCount());
                 deviceMask >>= 1, ++deviceIndex)
            {
                if (deviceMask & 1)
                {
                    m_deviceIndirectBufferWriter[deviceIndex] = Factory::Get().CreateIndirectBufferWriter();
                    result = m_deviceIndirectBufferWriter[deviceIndex]->Init(
                        memoryPtr, byteStride, maxCommandSequences, *signature.GetDeviceIndirectBufferSignature(deviceIndex).get());
                    if (result != ResultCode::Success)
                        break;
                }
            }

            return result;
        }

        bool IndirectBufferWriter::NextSequence()
        {
            auto result{ false };
            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
            {
                result = writer->NextSequence();
                if (!result)
                    break;
            }
            return result;
        }

        void IndirectBufferWriter::Shutdown()
        {
            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
                writer->Shutdown();
        }

        IndirectBufferWriter* IndirectBufferWriter::SetVertexView(uint32_t slot, const StreamBufferView& view)
        {
            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
                writer->SetVertexView(slot, view.GetDeviceStreamBufferView(deviceIndex));

            return this;
        }

        IndirectBufferWriter* IndirectBufferWriter::SetIndexView(const IndexBufferView& view)
        {
            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
                writer->SetIndexView(view.GetDeviceIndexBufferView(deviceIndex));

            return this;
        }

        IndirectBufferWriter* IndirectBufferWriter::Draw(const DrawLinear& arguments)
        {
            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
                writer->Draw(arguments);

            return this;
        }

        IndirectBufferWriter* IndirectBufferWriter::DrawIndexed(const RHI::DrawIndexed& arguments)
        {
            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
                writer->DrawIndexed(arguments);

            return this;
        }

        IndirectBufferWriter* IndirectBufferWriter::Dispatch(const DispatchDirect& arguments)
        {
            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
                writer->Dispatch(arguments);

            return this;
        }

        IndirectBufferWriter* IndirectBufferWriter::SetRootConstants(const uint8_t* data, uint32_t byteSize)
        {
            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
                writer->SetRootConstants(data, byteSize);

            return this;
        }

        bool IndirectBufferWriter::Seek(const uint32_t sequenceIndex)
        {
            auto result{ false };
            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
            {
                result = writer->Seek(sequenceIndex);
                if (!result)
                    break;
            }
            return result;
        }

        void IndirectBufferWriter::Flush()
        {
            // Unmap the buffer to force a flush changes into the buffer.
            // The buffer will be remap before writing new commands.
            // We don't remap here because we can't leave a buffer mapped during the
            // whole frame execution.
            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
                writer->Flush();
        }

        bool IndirectBufferWriter::IsInitialized() const
        {
            auto result{ false };
            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
            {
                result = writer->IsInitialized();
                if (!result)
                    break;
            }
            return result;
        }

        AZStd::vector<uint32_t> IndirectBufferWriter::GetCurrentSequenceIndex() const
        {
            AZStd::vector<uint32_t> currentSequenceIndex;

            for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
                currentSequenceIndex.emplace_back(writer->GetCurrentSequenceIndex());

            return currentSequenceIndex;
        }
    } // namespace RHI
} // namespace AZ
