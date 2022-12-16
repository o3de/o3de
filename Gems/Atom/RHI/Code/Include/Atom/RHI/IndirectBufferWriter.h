/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/IndirectBufferLayout.h>
#include <Atom/RHI/DeviceIndirectBufferWriter.h>
#include <Atom/RHI/DispatchItem.h>
#include <Atom/RHI/DrawItem.h>
#include <Atom/RHI/Object.h>

namespace AZ
{
    namespace RHI
    {
        class Buffer;
        class IndexBufferView;
        class StreamBufferView;
        class IndirectBufferSignature;

        //! IndirectBufferWriter is a helper class to write indirect commands
        //! to a buffer or a memory location in a platform independent way. Different APIs may
        //! have different layouts for the arguments of an indirect command. This class provides
        //! a secure and simple way to write the commands without worrying about API differences.
        //!
        //! It also provides basic checks, like trying to write more commands than allowed, or
        //! writing commands that are not specified in the layout.
        class IndirectBufferWriter : public Object
        {
            using Base = Object;

        public:
            AZ_CLASS_ALLOCATOR(IndirectBufferWriter, AZ::SystemAllocator, 0);
            AZ_RTTI(IndirectBufferWriter, "{EA0F19CD-CD76-4F9A-9001-F9DD3BFE36EB}");
            IndirectBufferWriter() = default;
            virtual ~IndirectBufferWriter() = default;

            //! Initialize the IndirectBufferWriter to write commands into a buffer.
            //! @param buffer The buffer where to write the commands. Any previous values for the specified range will be overwritten.
            //!               The buffer must be big enough to contain the max number of sequences.
            //! @param byteOffset The offset into the buffer.
            //! @param byteStride The stride between command sequences. Must be larger than the stride calculated from the signature.
            //! @param maxCommandSequences The max number of sequences that the IndirectBufferWriter can write.
            //! @param signature Signature of the indirect buffer.
            //! @return A result code denoting the status of the call. If successful, the IndirectBufferWriter is considered
            //!      initialized and is able to service write requests. If failure, the IndirectBufferWriter remains uninitialized.
            ResultCode Init(
                Buffer& buffer,
                size_t byteOffset,
                uint32_t byteStride,
                uint32_t maxCommandSequences,
                const IndirectBufferSignature& signature);

            //! Initialize the IndirectBufferWriter to write commands into a memory location.
            //! @param memoryPtr The memory location where the commands will be written. Must not be null.
            //! @param byteStride The stride between command sequences. Must be larger than the stride calculated from the signature.
            //! @param maxCommandSequences The max number of sequences that the IndirectBufferWriter can write.
            //! @param signature Signature of the indirect buffer.
            //! @return A result code denoting the status of the call. If successful, the IndirectBufferWriter is considered
            //!      initialized and is able to service write requests. If failure, the IndirectBufferWriter remains uninitialized.
            ResultCode Init(void* memoryPtr, uint32_t byteStride, uint32_t maxCommandSequences, const IndirectBufferSignature& signature);

            //! Writes a vertex buffer view command into the current sequence.
            //! @param slot The stream buffer slot that the view will set.
            //! @param view The StreamBufferView that will be set.
            //! @return A pointer to the IndirectBufferWriter object (this).
            IndirectBufferWriter* SetVertexView(uint32_t slot, const StreamBufferView& view);

            //! Writes an index buffer view command into the current sequence.
            //! @param view The IndexBufferView that will be set.
            //! @return A pointer to the IndirectBufferWriter object (this).
            IndirectBufferWriter* SetIndexView(const IndexBufferView& view);

            //! Writes a draw command into the current sequence.
            //! @param arguments The draw arguments that will be written.
            //! @return A pointer to the IndirectBufferWriter object (this).
            IndirectBufferWriter* Draw(const DrawLinear& arguments);

            //! Writes a draw indexed command into the current sequence.
            //! @param arguments The draw indexed arguments that will be written.
            //! @return A pointer to the IndirectBufferWriter object (this).
            IndirectBufferWriter* DrawIndexed(const DrawIndexed& arguments);

            //! Writes a dispatch command into the current sequence.
            //! @param arguments The dispatch arguments that will be written.
            //! @return A pointer to the IndirectBufferWriter object (this).
            IndirectBufferWriter* Dispatch(const DispatchDirect& arguments);

            //! Writes an inline constants command into the current sequence. This command will set
            //! the values of all inline constants of the Pipeline.
            //! @param data A pointer to the data that contains the values that will be written.
            //! @param byteSize The size of the data that will be written.
            //! @return A pointer to the IndirectBufferWriter object (this).
            IndirectBufferWriter* SetRootConstants(const uint8_t* data, uint32_t byteSize);

            //! Advance the current sequence index by 1.
            //! @return True if the sequence index was increased correctly. False otherwise.
            bool NextSequence();

            //! Move the current sequence index to a specified position.
            //! @param sequenceIndex The index where the sequence index will be moved. Must be less than maxCommandSequences.
            //! @return True if the sequence index was updated correctly. False otherwise and the current sequence index is not modified.
            bool Seek(const uint32_t sequenceIndex);

            /// Flush changes into the destination buffer. Only valid when using a buffer.
            void Flush();

            bool IsInitialized() const;

            AZStd::vector<uint32_t> GetCurrentSequenceIndex() const;

            void Shutdown() override;

        private:
            AZStd::unordered_map<int, Ptr<DeviceIndirectBufferWriter>> m_deviceIndirectBufferWriter;
        };
    } // namespace RHI
} // namespace AZ
