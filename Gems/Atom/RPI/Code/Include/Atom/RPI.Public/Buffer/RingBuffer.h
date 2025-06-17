/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/FrameCountMaxRingBuffer.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <Atom/RPI.Public/Buffer/BufferSystemInterface.h>
#include <Atom/RPI.Public/Configuration.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/string/string.h>

namespace AZ::RPI
{
    //! A class which manages a FrameCountMax number of RPI buffers and manages them in a ring buffer structure, meaning that whenever data
    //! needs to be updated, the current buffer index is incremented (mod FrameCountMax) and the data is then written to the new current
    //! buffer, such that the other buffers stay valid.
    class ATOM_RPI_PUBLIC_API RingBuffer : public RHI::FrameCountMaxRingBuffer<Data::Instance<Buffer>>
    {
        AZStd::string m_bufferName;
        CommonBufferPoolType m_bufferPoolType{ CommonBufferPoolType::ReadOnly };
        uint32_t m_elementSize{ 1 };
        RHI::Format m_bufferFormat{ RHI::Format::Unknown };

    public:
        //! Creates a set of buffers with a name and a format. The element size is derived from the format.
        RingBuffer(const AZStd::string& bufferName, CommonBufferPoolType bufferPoolType, RHI::Format bufferFormat);

        //! Creates a set of buffers with a name and an element size. The format is set to unknown.
        RingBuffer(const AZStd::string& bufferName, CommonBufferPoolType bufferPoolType, uint32_t elementSize);

        //! Returns true if the current buffer was already created and is not empty.
        bool IsCurrentBufferValid() const;

        //! Returns the current buffer
        const Data::Instance<Buffer>& GetCurrentBuffer() const;

        //! Returns a RHI view of the current buffer
        const RHI::BufferView* GetCurrentBufferView() const;

        //! Increments the current buffer index, potentially resized the current buffer and updates the data of the current data. This is a
        //! convenience function which calls AdvanceCurrentBuffer(), CreateOrResizeCurrentBuffer(...) and UpdateCurrentBufferData(...)
        void AdvanceCurrentBufferAndUpdateData(const void* data, u64 dataSizeInBytes);

        //! Increments the current buffer index, potentially resized the current buffer and updates the data of the current data. This is a
        //! convenience function which calls AdvanceCurrentBuffer(), CreateOrResizeCurrentBuffer(...) and UpdateCurrentBufferData(...)
        template<typename T>
        void AdvanceCurrentBufferAndUpdateData(AZStd::span<const T> data)
        {
            AdvanceCurrentBufferAndUpdateData(data.data(), data.size() * sizeof(T));
        }

        //! Convenience function which allows automatic conversion from vector/array to span
        template<typename T>
        void AdvanceCurrentBufferAndUpdateData(const T& data)
        {
            AdvanceCurrentBufferAndUpdateData<typename T::value_type>(data);
        }

        //! Increments the current buffer index, potentially resized the current buffer and updates the data of the current data. This is a
        //! convenience function which calls AdvanceCurrentBuffer(), CreateOrResizeCurrentBuffer(...) and UpdateCurrentBufferData(...)
        void AdvanceCurrentBufferAndUpdateData(const AZStd::unordered_map<int, const void*>& data, u64 dataSizeInBytes);

        //! Increments the current buffer index, potentially resized the current buffer and updates the data of the current data. This is a
        //! convenience function which calls AdvanceCurrentBuffer(), CreateOrResizeCurrentBuffer(...) and UpdateCurrentBufferData(...)
        template<typename T>
        void AdvanceCurrentBufferAndUpdateData(AZStd::unordered_map<int, AZStd::span<const T>> data)
        {
            AZStd::unordered_map<int, const void*> rawData;

            for (auto& [deviceIndex, d] : data)
            {
                rawData[deviceIndex] = d.data();
            }

            AdvanceCurrentBufferAndUpdateData(rawData, data.begin()->second.size() * sizeof(T));
        }

        //! Convenience function which allows automatic conversion from vector/array to span
        template<typename T>
        void AdvanceCurrentBufferAndUpdateData(const AZStd::unordered_map<int, T>& data)
        {
            AdvanceCurrentBufferAndUpdateData<typename T::value_type>(data);
        }

        //! Creates or resizes the current buffer to fit the given number of bytes.
        void CreateOrResizeCurrentBuffer(u64 bufferSizeInBytes);

        //! Creates or resizes the current buffer to fit the given number of elements.
        template<typename T>
        void CreateOrResizeCurrentBufferWithElementCount(u64 elementCount)
        {
            CreateOrResizeCurrentBuffer(elementCount * sizeof(T));
        }

        //! Updates the data of the current buffer.
        void UpdateCurrentBufferData(const void* data, u64 dataSizeInBytes, u64 bufferOffsetInBytes);

        //! Updates the data of the current buffer.
        template<typename T>
        void UpdateCurrentBufferData(AZStd::span<const T> data, u64 bufferElementOffset = 0)
        {
            UpdateCurrentBufferData(data.data(), data.size() * sizeof(T), bufferElementOffset * sizeof(T));
        }

        //! Convenience function which allows automatic conversion from vector/array to span
        template<typename T>
        void UpdateCurrentBufferData(const T& data, u64 bufferElementOffset = 0)
        {
            UpdateCurrentBufferData<typename T::value_type>(data, bufferElementOffset);
        }

        //! Updates the data of the current buffer.
        void UpdateCurrentBufferData(const AZStd::unordered_map<int, const void*>& data, u64 dataSizeInBytes, u64 bufferOffsetInBytes);

        //! Updates the data of the current buffer.
        template<typename T>
        void UpdateCurrentBufferData(AZStd::unordered_map<int, AZStd::span<const T>> data, u64 bufferElementOffset = 0)
        {
            AZStd::unordered_map<int, const void*> rawData;

            for (auto& [deviceIndex, d] : data)
            {
                rawData[deviceIndex] = d.data();
            }

            UpdateCurrentBufferData(rawData, data.begin()->second.size() * sizeof(T), bufferElementOffset * sizeof(T));
        }

        //! Convenience function which allows automatic conversion from vector/array to span
        template<typename T>
        void UpdateCurrentBufferData(const AZStd::unordered_map<int, T>& data, u64 bufferElementOffset = 0)
        {
            AZStd::unordered_map<int, const void*> rawData;

            for (auto& [deviceIndex, d] : data)
            {
                rawData[deviceIndex] = d.data();
            }

            UpdateCurrentBufferData(rawData, data.begin()->second.size() * sizeof(typename T::value_type), bufferElementOffset * sizeof(typename T::value_type));
        }
    };
} // namespace AZ::RPI
