/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/IndirectBufferLayout.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDeviceBuffer.h>
#include <Atom/RHI/MultiDeviceBufferPool.h>
#include <Atom/RHI/MultiDeviceStreamBufferView.h>
#include <Atom/RHI/MultiDeviceIndexBufferView.h>
#include <Atom/RHI/MultiDeviceIndirectBufferSignature.h>
#include <Atom/RHI/MultiDeviceIndirectBufferWriter.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    ResultCode MultiDeviceIndirectBufferWriter::Init(
        MultiDeviceBuffer& buffer,
        size_t byteOffset,
        uint32_t byteStride,
        uint32_t maxCommandSequences,
        const MultiDeviceIndirectBufferSignature& signature)
    {
        if (Validation::IsEnabled())
        {
            if (IsInitialized())
            {
                AZ_Assert(false, "MultiDeviceIndirectBufferWriter cannot be initialized when calling this method.");
                return ResultCode::InvalidOperation;
            }

            if ((byteOffset + maxCommandSequences * byteStride) > buffer.GetDescriptor().m_byteCount)
            {
                AZ_Assert(false, "MultiDeviceBuffer is too small to contain the required commands");
                return ResultCode::InvalidArgument;
            }
        }

        ResultCode result{ ResultCode::Success };
        auto deviceMask{ AZStd::to_underlying(buffer.GetDeviceMask()) };

        for (auto deviceIndex{ 0 }; deviceMask && (deviceIndex < RHI::RHISystemInterface::Get()->GetDeviceCount());
             deviceMask >>= 1, ++deviceIndex)
        {
            if (deviceMask & 1)
            {
                m_deviceIndirectBufferWriter[deviceIndex] = Factory::Get().CreateIndirectBufferWriter();
                auto deviceIndirectBufferSignature{ signature.IsInitialized() ? signature.GetDeviceIndirectBufferSignature(deviceIndex)
                                                                              : Factory::Get().CreateIndirectBufferSignature() };
                result = m_deviceIndirectBufferWriter[deviceIndex]->Init(
                    *buffer.GetDeviceBuffer(deviceIndex).get(),
                    byteOffset,
                    byteStride,
                    maxCommandSequences,
                    *deviceIndirectBufferSignature);

                if (result != ResultCode::Success)
                {
                    break;
                }
            }
        }

        if (result != ResultCode::Success)
        {
            // Reset already initialized device-specific IndirectBufferWriters
            m_deviceIndirectBufferWriter.clear();
        }

        return result;
    }

    ResultCode MultiDeviceIndirectBufferWriter::Init(
        void* memoryPtr, uint32_t byteStride, uint32_t maxCommandSequences, const MultiDeviceIndirectBufferSignature& signature)
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
        auto deviceMask{ AZStd::to_underlying(signature.GetDeviceMask()) };

        for (auto deviceIndex{ 0 }; deviceMask && (deviceIndex < RHI::RHISystemInterface::Get()->GetDeviceCount());
             deviceMask >>= 1, ++deviceIndex)
        {
            if (deviceMask & 1)
            {
                m_deviceIndirectBufferWriter[deviceIndex] = Factory::Get().CreateIndirectBufferWriter();
                auto deviceIndirectBufferSignature{ signature.IsInitialized() ? signature.GetDeviceIndirectBufferSignature(deviceIndex)
                                                                              : Factory::Get().CreateIndirectBufferSignature() };
                result = m_deviceIndirectBufferWriter[deviceIndex]->Init(
                    memoryPtr, byteStride, maxCommandSequences, *deviceIndirectBufferSignature);

                if (result != ResultCode::Success)
                    break;
            }
        }

        if (result != ResultCode::Success)
        {
            // Reset already initialized device-specific IndirectBufferWriters
            m_deviceIndirectBufferWriter.clear();
        }

        return result;
    }

    bool MultiDeviceIndirectBufferWriter::NextSequence()
    {
        auto result{ false };
        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            result = writer->NextSequence();
            if (!result)
            {
                break;
            }
        }
        return result;
    }

    void MultiDeviceIndirectBufferWriter::Shutdown()
    {
        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            writer->Shutdown();
        }
    }

    MultiDeviceIndirectBufferWriter* MultiDeviceIndirectBufferWriter::SetVertexView(uint32_t slot, const MultiDeviceStreamBufferView& view)
    {
        if (Validation::IsEnabled() && !IsInitialized())
        {
            AZ_Assert(false, "MultiDeviceIndirectBufferWriter must be initialized when calling this method.");
        }

        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            writer->SetVertexView(slot, view.GetDeviceStreamBufferView(deviceIndex));
        }

        return this;
    }

    MultiDeviceIndirectBufferWriter* MultiDeviceIndirectBufferWriter::SetIndexView(const MultiDeviceIndexBufferView& view)
    {
        if (Validation::IsEnabled() && !IsInitialized())
        {
            AZ_Assert(false, "MultiDeviceIndirectBufferWriter must be initialized when calling this method.");
        }

        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            writer->SetIndexView(view.GetDeviceIndexBufferView(deviceIndex));
        }

        return this;
    }

    MultiDeviceIndirectBufferWriter* MultiDeviceIndirectBufferWriter::Draw(const DrawLinear& arguments)
    {
        if (Validation::IsEnabled() && !IsInitialized())
        {
            AZ_Assert(false, "MultiDeviceIndirectBufferWriter must be initialized when calling this method.");
        }

        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            writer->Draw(arguments);
        }

        return this;
    }

    MultiDeviceIndirectBufferWriter* MultiDeviceIndirectBufferWriter::DrawIndexed(const RHI::DrawIndexed& arguments)
    {
        if (Validation::IsEnabled() && !IsInitialized())
        {
            AZ_Assert(false, "MultiDeviceIndirectBufferWriter must be initialized when calling this method.");
        }

        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            writer->DrawIndexed(arguments);
        }

        return this;
    }

    MultiDeviceIndirectBufferWriter* MultiDeviceIndirectBufferWriter::Dispatch(const DispatchDirect& arguments)
    {
        if (Validation::IsEnabled() && !IsInitialized())
        {
            AZ_Assert(false, "MultiDeviceIndirectBufferWriter must be initialized when calling this method.");
        }

        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            writer->Dispatch(arguments);
        }

        return this;
    }

    MultiDeviceIndirectBufferWriter* MultiDeviceIndirectBufferWriter::SetRootConstants(const uint8_t* data, uint32_t byteSize)
    {
        if (Validation::IsEnabled() && !IsInitialized())
        {
            AZ_Assert(false, "MultiDeviceIndirectBufferWriter must be initialized when calling this method.");
        }

        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            writer->SetRootConstants(data, byteSize);
        }

        return this;
    }

    bool MultiDeviceIndirectBufferWriter::Seek(const uint32_t sequenceIndex)
    {
        auto result{ false };
        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            result = writer->Seek(sequenceIndex);
            if (!result)
            {
                break;
            }
        }
        return result;
    }

    void MultiDeviceIndirectBufferWriter::Flush()
    {
        // Unmap the buffer to force a flush changes into the buffer.
        // The buffer will be remap before writing new commands.
        // We don't remap here because we can't leave a buffer mapped during the
        // whole frame execution.
        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            writer->Flush();
        }
    }

    bool MultiDeviceIndirectBufferWriter::IsInitialized() const
    {
        auto result{ false };
        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            result = writer->IsInitialized();
            if (!result)
            {
                break;
            }
        }
        return result;
    }

    AZStd::vector<uint32_t> MultiDeviceIndirectBufferWriter::GetCurrentSequenceIndex() const
    {
        AZStd::vector<uint32_t> currentSequenceIndex;

        for (const auto& [deviceIndex, writer] : m_deviceIndirectBufferWriter)
        {
            currentSequenceIndex.emplace_back(writer->GetCurrentSequenceIndex());
        }

        return currentSequenceIndex;
    }
} // namespace AZ::RHI
