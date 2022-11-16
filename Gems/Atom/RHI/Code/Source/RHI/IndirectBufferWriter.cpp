/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/IndirectBufferWriter.h>
#include <Atom/RHI/IndirectBufferSignature.h>
#include <Atom/RHI.Reflect/IndirectBufferLayout.h>

namespace AZ
{
    namespace RHI
    {
        ResultCode IndirectBufferWriter::Init(Buffer& buffer, size_t byteOffset, uint32_t byteStride, uint32_t maxCommandSequences, const IndirectBufferSignature& signature)
        {
            if (!ValidateInitializedState(ValidateInitializedStateExpect::NotInitialized))
            {
                return ResultCode::InvalidOperation;
            }

            if (!ValidateArguments(byteStride, maxCommandSequences, signature))
            {
                return ResultCode::InvalidArgument;
            }

            if (Validation::IsEnabled())
            {
                if ((byteOffset + maxCommandSequences * byteStride) > buffer.GetDescriptor().m_byteCount)
                {
                    AZ_Assert(false, "Buffer is too small to contain the required commands");
                    return ResultCode::InvalidArgument;
                }
            }

            m_buffer = &buffer;
            m_signature = &signature;
            m_bufferOffset = byteOffset;
            m_sequenceStride = byteStride;
            m_maxSequences = maxCommandSequences;
            m_currentSequenceIndex = 0;

            auto result = MapBuffer();
            return result;
        }

        ResultCode IndirectBufferWriter::Init(void* memoryPtr, uint32_t byteStride, uint32_t maxCommandSequences, const IndirectBufferSignature& signature)
        {
            if (!ValidateInitializedState(ValidateInitializedStateExpect::NotInitialized))
            {
                return ResultCode::InvalidOperation;
            }

            if (!ValidateArguments(byteStride, maxCommandSequences, signature))
            {
                return ResultCode::InvalidArgument;
            }

            if (Validation::IsEnabled())
            {
                if (!memoryPtr)
                {
                    AZ_Assert(false, "Null target memory");
                    return ResultCode::InvalidArgument;
                }
            }

            m_buffer = nullptr;
            m_targetMemory = static_cast<uint8_t*>(memoryPtr);
            m_signature = &signature;
            m_sequenceStride = byteStride;
            m_maxSequences = maxCommandSequences;
            m_currentSequenceIndex = 0;

            return ResultCode::Success;
        }

        bool IndirectBufferWriter::NextSequence()
        {
            return Seek(m_currentSequenceIndex + 1);
        }

        void IndirectBufferWriter::Shutdown()
        {
            if (m_buffer && m_targetMemory)
            {
                UnmapBuffer();
            }

            m_buffer = nullptr;
            m_targetMemory = nullptr;
            m_signature = nullptr;
            m_sequenceStride = 0;
            m_maxSequences = 0;
            m_currentSequenceIndex = static_cast<uint32_t>(-1);
        }

        bool IndirectBufferWriter::ValidateArguments(uint32_t byteStride, uint32_t maxCommandSequences, const IndirectBufferSignature& signature) const
        {
            if (Validation::IsEnabled())
            {
                if (!signature.GetLayout().IsFinalized())
                {
                    AZ_Assert(false, "Layout is not finalized");
                    return false;
                }

                if (signature.GetByteStride() > byteStride)
                {
                    AZ_Assert(false, "Byte stride (%d) is smaller than the minimum required stride (%d)", int(byteStride), int(signature.GetByteStride()));
                    return false;
                }

                if (byteStride == 0)
                {
                    AZ_Assert(false, "Invalid sequence stride");
                    return false;
                }

                if (maxCommandSequences == 0)
                {
                    AZ_Assert(false, "Invalid max sequences count");
                    return false;
                }
            }

            return true;
        }

        bool IndirectBufferWriter::ValidateRootConstantsCommand(IndirectCommandIndex index, uint32_t byteSize) const
        {
            if (Validation::IsEnabled())
            {
                // First we calculate the stride of the inline constants command.
                const auto& layout = m_signature->GetLayout();
                bool isLastCommand = index.GetIndex() >= layout.GetCommands().size() - 1;
                uint32_t nextCommandOffset = isLastCommand ? m_signature->GetByteStride() : m_signature->GetOffset(IndirectCommandIndex(index.GetIndex() + 1));
                uint32_t commandStride = nextCommandOffset - m_signature->GetOffset(index);
                // Check that we are writing the correct amount of data.
                if (commandStride != byteSize)
                {
                    AZ_Assert(false, "Size of inline constants command doesn't match the expected size from the signature.");
                    return false;
                }
            }

            return true;
        }

        bool IndirectBufferWriter::PrepareWriting(IndirectCommandIndex commandIndex)
        {
            if (Validation::IsEnabled())
            {
                if (commandIndex.IsNull())
                {
                    AZ_Assert(false, "Command is not included in the Indirect Buffer Layout");
                    return false;
                }
            }

            // Check if we need to map the buffer before writing commands.
            if (m_buffer && !m_targetMemory)
            {
                auto result = MapBuffer();
                if (result != ResultCode::Success)
                {
                    return false;
                }
            }
            return true;
        }

        AZ::RHI::ResultCode IndirectBufferWriter::MapBuffer()
        {
            if (Validation::IsEnabled())
            {
                if (!m_buffer || m_targetMemory)
                {
                    AZ_Assert(false, "Could not map buffer because it's already mapped or invalid buffer");
                    return ResultCode::InvalidOperation;
                }
            }

            // Map the buffer so the implementations can write to it.
            BufferPool* pool = static_cast<BufferPool*>(m_buffer->GetPool());
            BufferMapRequest request = {};
            BufferMapResponse response = {};
            request.m_buffer = m_buffer;
            request.m_byteCount = m_maxSequences * m_sequenceStride;
            request.m_byteOffset = m_bufferOffset;

            RHI::ResultCode result = pool->MapBuffer(request, response);
            if (result != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "Failed to map buffer for IndirectBufferWriter");
                return result;
            }

            m_targetMemory = static_cast<uint8_t*>(response.m_data);
            return ResultCode::Success;
        }

        void IndirectBufferWriter::UnmapBuffer()
        {
            if (Validation::IsEnabled())
            {
                if (!m_buffer || !m_targetMemory)
                {
                    AZ_Assert(false, "Could not unmap buffer because of invalid buffer or buffer was not mapped.");
                    return;
                }
            }

            static_cast<BufferPool*>(m_buffer->GetPool())->UnmapBuffer(*m_buffer);
            m_targetMemory = nullptr;
        }

        bool IndirectBufferWriter::ValidateInitializedState(ValidateInitializedStateExpect expect) const
        {
            if (Validation::IsEnabled())
            {
                if (expect == ValidateInitializedStateExpect::Initialized && !IsInitialized())
                {
                    AZ_Assert(false, "IndirectBufferWriter must be initialized when calling this method.");
                    return false;
                }
                else if (expect == ValidateInitializedStateExpect::NotInitialized && IsInitialized())
                {
                    AZ_Assert(false, "IndirectBufferWriter cannot be initialized when calling this method.");
                    return false;
                }
            }
            return true;
        }

        uint8_t* IndirectBufferWriter::GetTargetMemory() const
        {
            return m_targetMemory;
        }

        IndirectBufferWriter* IndirectBufferWriter::SetVertexView(uint32_t slot, const StreamBufferView& view)
        {
            if (ValidateInitializedState(ValidateInitializedStateExpect::Initialized))
            {
                IndirectCommandIndex index = m_signature->GetLayout().FindCommandIndex(IndirectCommandDescriptor(IndirectBufferViewArguments{ slot }));
                if (PrepareWriting(index))
                {
                    SetVertexViewInternal(index, view);
                }
            }

            return this;
        }

        IndirectBufferWriter* IndirectBufferWriter::SetIndexView(const IndexBufferView& view)
        {
            if (ValidateInitializedState(ValidateInitializedStateExpect::Initialized))
            {
                IndirectCommandIndex index = m_signature->GetLayout().FindCommandIndex(IndirectCommandType::IndexBufferView);
                if (PrepareWriting(index))
                {
                    SetIndexViewInternal(index, view);
                }
            }
            
            return this;
        }

        IndirectBufferWriter* IndirectBufferWriter::Draw(const DrawLinear& arguments)
        {
            if (ValidateInitializedState(ValidateInitializedStateExpect::Initialized))
            {
                IndirectCommandIndex index = m_signature->GetLayout().FindCommandIndex(IndirectCommandType::Draw);
                if (PrepareWriting(index))
                {
                    DrawInternal(index, arguments);
                }
            }

            return this;
        }

        IndirectBufferWriter* IndirectBufferWriter::DrawIndexed(const RHI::DrawIndexed& arguments)
        {
            if (ValidateInitializedState(ValidateInitializedStateExpect::Initialized))
            {
                IndirectCommandIndex index = m_signature->GetLayout().FindCommandIndex(IndirectCommandType::DrawIndexed);
                if (PrepareWriting(index))
                {
                    DrawIndexedInternal(index, arguments);
                }
            }
            
            return this;
        }

        IndirectBufferWriter* IndirectBufferWriter::Dispatch(const DispatchDirect& arguments)
        {
            if (ValidateInitializedState(ValidateInitializedStateExpect::Initialized))
            {
                IndirectCommandIndex index = m_signature->GetLayout().FindCommandIndex(IndirectCommandType::Dispatch);
                if (PrepareWriting(index))
                {
                    DispatchInternal(index, arguments);
                }
            }
            
            return this;
        }

        IndirectBufferWriter* IndirectBufferWriter::SetRootConstants(const uint8_t* data, uint32_t byteSize)
        {
            if (ValidateInitializedState(ValidateInitializedStateExpect::Initialized))
            {
                IndirectCommandIndex index = m_signature->GetLayout().FindCommandIndex(IndirectCommandType::RootConstants);
                if (PrepareWriting(index) && ValidateRootConstantsCommand(index, byteSize))
                {
                    SetRootConstantsInternal(index, data, byteSize);
                }
            }
            
            return this;
        }

        bool IndirectBufferWriter::Seek(const uint32_t sequenceIndex)
        {
            if (sequenceIndex >= m_maxSequences)
            {
                return false;
            }

            m_currentSequenceIndex = sequenceIndex;
            return true;
        }

        void IndirectBufferWriter::Flush()
        {
            if (ValidateInitializedState(ValidateInitializedStateExpect::Initialized) &&
                m_buffer && m_targetMemory)
            {
                // Unmap the buffer to force a flush changes into the buffer.
                // The buffer will be remap before writing new commands.
                // We don't remap here because we can't leave a buffer mapped during the
                // whole frame execution.
                UnmapBuffer();
            }
        }

        bool IndirectBufferWriter::IsInitialized() const
        {
            return !!m_signature;
        }

        uint32_t IndirectBufferWriter::GetCurrentSequenceIndex() const
        {
            return m_currentSequenceIndex;
        }
    }
}
