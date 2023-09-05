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
#include <Atom/RHI/IndirectBufferWriter.h>
#include <Atom/RHI/Object.h>

namespace AZ::RHI
{
    class MultiDeviceBuffer;
    class MultiDeviceIndexBufferView;
    class MultiDeviceStreamBufferView;
    class MultiDeviceIndirectBufferSignature;

    //! MultiDeviceIndirectBufferWriter is a helper class to write indirect commands
    //! to a buffer or a memory location in a platform independent way. Different APIs may
    //! have different layouts for the arguments of an indirect command. This class provides
    //! a secure and simple way to write the commands without worrying about API differences.
    //!
    //! It also provides basic checks, like trying to write more commands than allowed, or
    //! writing commands that are not specified in the layout.
    class MultiDeviceIndirectBufferWriter : public Object
    {
        using Base = Object;

    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceIndirectBufferWriter, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceIndirectBufferWriter, "{096CBDFF-AB05-4E8D-9EC1-04F12CFCD85D}");
        virtual ~MultiDeviceIndirectBufferWriter() = default;

        //! Returns the device-specific IndirectBufferWriter for the given index
        inline Ptr<IndirectBufferWriter> GetDeviceIndirectBufferWriter(int deviceIndex) const
        {
            AZ_Error(
                "MultiDeviceIndirectBufferWriter",
                m_deviceIndirectBufferWriter.find(deviceIndex) != m_deviceIndirectBufferWriter.end(),
                "No IndirectBufferWriter found for device index %d\n",
                deviceIndex);
            return m_deviceIndirectBufferWriter.at(deviceIndex);
        }

        //! Initialize the MultiDeviceIndirectBufferWriter to write commands into a buffer.
        //! @param buffer The buffer where to write the commands. Any previous values for the specified range will be overwritten.
        //!               The buffer must be big enough to contain the max number of sequences.
        //! @param byteOffset The offset into the buffer.
        //! @param byteStride The stride between command sequences. Must be larger than the stride calculated from the signature.
        //! @param maxCommandSequences The max number of sequences that the MultiDeviceIndirectBufferWriter can write.
        //! @param signature Signature of the indirect buffer.
        //! @return A result code denoting the status of the call. If successful, the MultiDeviceIndirectBufferWriter is considered
        //!      initialized and is able to service write requests. If failure, the MultiDeviceIndirectBufferWriter remains
        //!      uninitialized.
        ResultCode Init(
            MultiDeviceBuffer& buffer,
            size_t byteOffset,
            uint32_t byteStride,
            uint32_t maxCommandSequences,
            const MultiDeviceIndirectBufferSignature& signature);

        //! Initialize the MultiDeviceIndirectBufferWriter to write commands into a memory location.
        //! @param memoryPtr The memory location where the commands will be written. Must not be null.
        //! @param byteStride The stride between command sequences. Must be larger than the stride calculated from the signature.
        //! @param maxCommandSequences The max number of sequences that the MultiDeviceIndirectBufferWriter can write.
        //! @param signature Signature of the indirect buffer.
        //! @return A result code denoting the status of the call. If successful, the MultiDeviceIndirectBufferWriter is considered
        //!      initialized and is able to service write requests. If failure, the MultiDeviceIndirectBufferWriter remains
        //!      uninitialized.
        ResultCode Init(
            void* memoryPtr, uint32_t byteStride, uint32_t maxCommandSequences, const MultiDeviceIndirectBufferSignature& signature);

        //! Writes a vertex buffer view command into the current sequence.
        //! @param slot The stream buffer slot that the view will set.
        //! @param view The MultiDeviceStreamBufferView that will be set.
        //! @return A pointer to the MultiDeviceIndirectBufferWriter object (this).
        MultiDeviceIndirectBufferWriter* SetVertexView(uint32_t slot, const MultiDeviceStreamBufferView& view);

        //! Writes an index buffer view command into the current sequence.
        //! @param view The MultiDeviceIndexBufferView that will be set.
        //! @return A pointer to the MultiDeviceIndirectBufferWriter object (this).
        MultiDeviceIndirectBufferWriter* SetIndexView(const MultiDeviceIndexBufferView& view);

        //! Writes a draw command into the current sequence.
        //! @param arguments The draw arguments that will be written.
        //! @return A pointer to the MultiDeviceIndirectBufferWriter object (this).
        MultiDeviceIndirectBufferWriter* Draw(const DrawLinear& arguments);

        //! Writes a draw indexed command into the current sequence.
        //! @param arguments The draw indexed arguments that will be written.
        //! @return A pointer to the MultiDeviceIndirectBufferWriter object (this).
        MultiDeviceIndirectBufferWriter* DrawIndexed(const DrawIndexed& arguments);

        //! Writes a dispatch command into the current sequence.
        //! @param arguments The dispatch arguments that will be written.
        //! @return A pointer to the MultiDeviceIndirectBufferWriter object (this).
        MultiDeviceIndirectBufferWriter* Dispatch(const DispatchDirect& arguments);

        //! Writes an inline constants command into the current sequence. This command will set
        //! the values of all inline constants of the Pipeline.
        //! @param data A pointer to the data that contains the values that will be written.
        //! @param byteSize The size of the data that will be written.
        //! @return A pointer to the MultiDeviceIndirectBufferWriter object (this).
        MultiDeviceIndirectBufferWriter* SetRootConstants(const uint8_t* data, uint32_t byteSize);

        //! Advance the current sequence index by 1.
        //! @return True if the sequence index was increased correctly. False otherwise.
        bool NextSequence();

        //! Move the current sequence index to a specified position.
        //! @param sequenceIndex The index where the sequence index will be moved. Must be less than maxCommandSequences.
        //! @return True if the sequence index was updated correctly. False otherwise and the current sequence index is not modified.
        bool Seek(const uint32_t sequenceIndex);

        //! Flush changes into the destination buffer. Only valid when using a buffer.
        void Flush();

        bool IsInitialized() const;

        AZStd::vector<uint32_t> GetCurrentSequenceIndex() const;

        void Shutdown() override;

    private:
        //! A map of all device-specific IndirectBufferWriter, indexed by the device index
        AZStd::unordered_map<int, Ptr<IndirectBufferWriter>> m_deviceIndirectBufferWriter;
    };
} // namespace AZ::RHI
