/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/SingleDeviceDispatchItem.h>
#include <Atom/RHI/SingleDeviceDrawItem.h>
#include <Atom/RHI/Object.h>
#include <Atom/RHI.Reflect/IndirectBufferLayout.h>

namespace AZ::RHI
{
    class SingleDeviceBuffer;
    struct IndirectBufferLayout;
    class SingleDeviceIndexBufferView;
    class SingleDeviceStreamBufferView;
    class SingleDeviceIndirectBufferSignature;

    //! SingleDeviceIndirectBufferWriter is a helper class to write indirect commands
    //! to a buffer or a memory location in a platform independent way. Different APIs may
    //! have different layouts for the arguments of an indirect command. This class provides
    //! a secure and simple way to write the commands without worrying about API differences.
    //!
    //! It also provides basic checks, like trying to write more commands than allowed, or
    //! writing commands that are not specified in the layout.
    class SingleDeviceIndirectBufferWriter :
        public Object
    {
        using Base = Object;
    public:
        AZ_RTTI(SingleDeviceIndirectBufferWriter, "{7F569E74-382B-44EC-B0C5-89C07A184B47}");
        virtual ~SingleDeviceIndirectBufferWriter() = default;

        //! Initialize the SingleDeviceIndirectBufferWriter to write commands into a buffer.
        //! @param buffer The buffer where to write the commands. Any previous values for the specified range will be overwritten.
        //!               The buffer must be big enough to contain the max number of sequences.
        //! @param byteOffset The offset into the buffer.
        //! @param byteStride The stride between command sequences. Must be larger than the stride calculated from the signature.
        //! @param maxCommandSequences The max number of sequences that the SingleDeviceIndirectBufferWriter can write.
        //! @param signature Signature of the indirect buffer.
        //! @return A result code denoting the status of the call. If successful, the SingleDeviceIndirectBufferWriter is considered
        //!      initialized and is able to service write requests. If failure, the SingleDeviceIndirectBufferWriter remains uninitialized.
        ResultCode Init(SingleDeviceBuffer& buffer, size_t byteOffset, uint32_t byteStride, uint32_t maxCommandSequences, const SingleDeviceIndirectBufferSignature& signature);

        //! Initialize the SingleDeviceIndirectBufferWriter to write commands into a memory location.
        //! @param memoryPtr The memory location where the commands will be written. Must not be null.
        //! @param byteStride The stride between command sequences. Must be larger than the stride calculated from the signature.
        //! @param maxCommandSequences The max number of sequences that the SingleDeviceIndirectBufferWriter can write.
        //! @param signature Signature of the indirect buffer.
        //! @return A result code denoting the status of the call. If successful, the SingleDeviceIndirectBufferWriter is considered
        //!      initialized and is able to service write requests. If failure, the SingleDeviceIndirectBufferWriter remains uninitialized.
        ResultCode Init(void* memoryPtr, uint32_t byteStride, uint32_t maxCommandSequences, const SingleDeviceIndirectBufferSignature& signature);

        //! Writes a vertex buffer view command into the current sequence.
        //! @param slot The stream buffer slot that the view will set.
        //! @param view The SingleDeviceStreamBufferView that will be set.
        //! @return A pointer to the SingleDeviceIndirectBufferWriter object (this).
        SingleDeviceIndirectBufferWriter* SetVertexView(uint32_t slot, const SingleDeviceStreamBufferView& view);

        //! Writes an index buffer view command into the current sequence.
        //! @param view The SingleDeviceIndexBufferView that will be set.
        //! @return A pointer to the SingleDeviceIndirectBufferWriter object (this).
        SingleDeviceIndirectBufferWriter* SetIndexView(const SingleDeviceIndexBufferView& view);

        //! Writes a draw command into the current sequence.
        //! @param arguments The draw arguments that will be written.
        //! @return A pointer to the SingleDeviceIndirectBufferWriter object (this).
        SingleDeviceIndirectBufferWriter* Draw(const DrawLinear& arguments);

        //! Writes a draw indexed command into the current sequence.
        //! @param arguments The draw indexed arguments that will be written.
        //! @return A pointer to the SingleDeviceIndirectBufferWriter object (this).
        SingleDeviceIndirectBufferWriter* DrawIndexed(const DrawIndexed& arguments);

        //! Writes a dispatch command into the current sequence.
        //! @param arguments The dispatch arguments that will be written.
        //! @return A pointer to the SingleDeviceIndirectBufferWriter object (this).
        SingleDeviceIndirectBufferWriter* Dispatch(const DispatchDirect& arguments);

        //! Writes an inline constants command into the current sequence. This command will set
        //! the values of all inline constants of the Pipeline.
        //! @param data A pointer to the data that contains the values that will be written.
        //! @param byteSize The size of the data that will be written.
        //! @return A pointer to the SingleDeviceIndirectBufferWriter object (this).
        SingleDeviceIndirectBufferWriter* SetRootConstants(const uint8_t* data, uint32_t byteSize);

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

        uint32_t GetCurrentSequenceIndex() const;

        void Shutdown() override;

    private:
        bool ValidateArguments(uint32_t byteStride, uint32_t maxCommandSequences, const SingleDeviceIndirectBufferSignature& signature) const;
        bool ValidateRootConstantsCommand(IndirectCommandIndex index, uint32_t byteSize) const;
        bool PrepareWriting(IndirectCommandIndex commandIndex);

        ResultCode MapBuffer();
        void UnmapBuffer();

        enum class ValidateInitializedStateExpect
        {
            NotInitialized = 0,
            Initialized
        };

        bool ValidateInitializedState(ValidateInitializedStateExpect expect) const;

        uint32_t m_currentSequenceIndex = 0;
        uint8_t* m_targetMemory = nullptr;

    protected:
        //////////////////////////////////////////////////////////////////////////
        // Platform API

        /// Called when writing a vertex view command.
        virtual void SetVertexViewInternal(IndirectCommandIndex index, const SingleDeviceStreamBufferView& view) = 0;
        /// Called when writing an index view command.
        virtual void SetIndexViewInternal(IndirectCommandIndex index, const SingleDeviceIndexBufferView& view) = 0;
        /// Called when writing a draw command.
        virtual void DrawInternal(IndirectCommandIndex index, const DrawLinear& arguments) = 0;
        /// Call when writing a draw indexed command.
        virtual void DrawIndexedInternal(IndirectCommandIndex index, const RHI::DrawIndexed& arguments) = 0;
        /// Call when writing a dispatch command.
        virtual void DispatchInternal(IndirectCommandIndex index, const DispatchDirect& arguments) = 0;
        /// Call when writing an inline constants command.
        virtual void SetRootConstantsInternal(IndirectCommandIndex index, const uint8_t* data, uint32_t byteSize) = 0;

        //////////////////////////////////////////////////////////////////////////

        uint8_t* GetTargetMemory() const;

        SingleDeviceBuffer* m_buffer = 0;
        const SingleDeviceIndirectBufferSignature* m_signature = nullptr;
        uint32_t m_maxSequences = 0;
        uint32_t m_sequenceStride = 0;
        size_t m_bufferOffset = 0;
    };
}
