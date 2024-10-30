/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DeviceDrawArguments.h>
#include <Atom/RHI/DeviceIndexBufferView.h>
#include <Atom/RHI/DeviceStreamBufferView.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::RHI
{
    class InputStreamLayout;

    //! This container holds a list of indices into a GeometryView's stream buffer views
    //! This allows DrawItems to only the stream buffers it needs (and in the order its shader needs them)
    //! Each index is four bits, such that each u8 in this class holds two indices for better memory efficiency
    //! To iterate through stream buffer views with these indices, use the StreamIterator class below
    class StreamBufferIndices
    {
        u8 m_count = 0;
        u8 m_indices[RHI::Limits::Pipeline::StreamCountMax / 2];

    public:
        void AddIndex(u8 index)
        {
            AZ_Assert(index < (1 << 4), "Provided index [%d] is larger than 4 bits, which breaks bit packing for StreamBufferIndices", index);
            AZ_Assert(index < RHI::Limits::Pipeline::StreamCountMax, "Adding index [%d], which is greater or equal to the the StreamCountMax %d",
                index, RHI::Limits::Pipeline::StreamCountMax);
            AZ_Assert(m_count < RHI::Limits::Pipeline::StreamCountMax, "Adding %d stream buffer indices, but the max count only allows for %d",
                m_count, RHI::Limits::Pipeline::StreamCountMax);

            u8 newPosition = m_count / 2;
            bool needsShift = (m_count % 2) == 1;

            if (needsShift)
            {
                m_indices[newPosition] |= index << 4;
            }
            else
            {
                m_indices[newPosition] = index & 0xF;
            }
            ++m_count;
        }

        u8 GetIndex(u8 position) const
        {
            AZ_Assert(position < m_count, "Accessing index %d but only have %d indices", position, m_count);

            bool needsShift = (position % 2) == 1;
            u8 fullValue = m_indices[position / 2];
            u8 halfValue = needsShift ? fullValue >> 4 : fullValue;
            halfValue &= 0xF;
            return halfValue;
        }

        bool operator==(const StreamBufferIndices& other) const
        {
            if (m_count != other.m_count)
            { return false; }

            bool same = true;
            for (u8 i = 0; i < m_count; ++i)
            {
                same = same && (GetIndex(i) == other.GetIndex(i));
            }
            return same;
        }

        u8 Size() const { return m_count; }
        void Reset() { m_count = 0; }
    };

    //! Helper class to conveniently iterate through stream buffer views using the StreamBufferIndices class above
    //! Takes as input GeometryView* and StreamBufferIndices, then can be used to iterate with ++ operators
    //! It also supports direct indexing via myStreamIter[idx] syntax.
    //! This class is templated to support both GeometryView and DeviceGeometryView
    template<typename GeometryViewType, typename StreamBufferViewType>
    class StreamIterator
    {
        const GeometryViewType* m_geometryView;
        const StreamBufferIndices& m_indices;
        u8 m_current = 0;

    public:
        StreamIterator(const GeometryViewType* geometryView, const StreamBufferIndices& indices)
            : m_geometryView(geometryView)
            , m_indices(indices)
        {}

        //! Use this in your for loop to check if the iterator has reached the end
        bool HasEnded() const { return m_current >= m_indices.Size(); }

        //! Can be used to reset the iterator if you want to use it for subsequent loops
        void Reset() { m_current = 0; }

        //! Used to check if the current item is a valid buffer, useful when checking dummy buffers
        bool IsValid()
        {
            return m_indices.GetIndex(m_current) < m_geometryView->GetStreamBufferViews().size();
        }

        StreamIterator& operator++()
        {
            if (!HasEnded()) {
                ++m_current;
            }
            return *this;
        }
        StreamIterator operator++(int)
        {
            StreamIterator temp = *this;
            ++(*this);
            return temp;
        }

        //! Used to access the current StreamBufferView
        const StreamBufferViewType& operator*()
        {
            return m_geometryView->GetStreamBufferView(m_indices.GetIndex(m_current));
        }

        //! Used to access the current StreamBufferView
        const StreamBufferViewType* operator->()
        {
            return &m_geometryView->GetStreamBufferView(m_indices.GetIndex(m_current));
        }

        //! Used for direct indexing, irrespective of the current iterator progress
        const StreamBufferViewType& operator[](u16 idx) const
        {
            AZ_Assert(static_cast<u8>(idx) < m_indices.Size(), "Passed index %d exceeds number of indices (%d) for stream buffer views",
                idx, m_indices.Size());

            return m_geometryView->GetStreamBufferView(m_indices.GetIndex(static_cast<u8>(idx)));
        }
    };

    //! GeometryViews hold draw arguments and geometry index/stream buffer views used for rendering DrawPackets/DrawItems
    class DeviceGeometryView
    {
        DeviceDrawArguments m_drawArguments;
        DeviceIndexBufferView m_indexBufferView;
        AZStd::vector<DeviceStreamBufferView> m_streamBufferViews;

        //! Dummy StreamBufferView is used when shader requires an options stream that has not been provided by the user
        static constexpr u8 InvalidStreamBufferIndex = 0xFF;
        u8 m_dummyStreamBufferIndex = InvalidStreamBufferIndex;

    public:
        friend class GeometryView;
        friend class StreamIterator<DeviceGeometryView, DeviceStreamBufferView>;

        void Reset()
        {
            m_drawArguments = DeviceDrawArguments{};
            m_indexBufferView = DeviceIndexBufferView{};
            ClearStreamBufferViews();
        }

        // --- DeviceDrawArguments ---

        void SetDrawArguments(DeviceDrawArguments drawArguments) { m_drawArguments = drawArguments; }
        const DeviceDrawArguments& GetDrawArguments() const { return m_drawArguments; }

        // --- IndexBufferView ---

        void SetIndexBufferView(DeviceIndexBufferView indexBufferView) { m_indexBufferView = indexBufferView; }
        const DeviceIndexBufferView& GetIndexBufferView() const { return m_indexBufferView; }

        // --- StreamBufferView ---

        void ClearStreamBufferViews()
        {
            m_streamBufferViews.clear();
            m_dummyStreamBufferIndex = InvalidStreamBufferIndex;
        }
        void AddStreamBufferView(DeviceStreamBufferView streamBufferView) { m_streamBufferViews.emplace_back(streamBufferView); }
        void SetStreamBufferView(u8 idx, DeviceStreamBufferView streamBufferView) { m_streamBufferViews[idx] = streamBufferView; }
        const DeviceStreamBufferView& GetStreamBufferView(u8 idx) const { return m_streamBufferViews[idx]; }

        AZStd::vector<DeviceStreamBufferView>& GetStreamBufferViews() { return m_streamBufferViews; }
        const AZStd::vector<DeviceStreamBufferView>& GetStreamBufferViews() const { return m_streamBufferViews; }

        //! Helper function that provides indices to all the StreamBufferViews. Useful when GeometryViews are created purposely for a single DrawItem
        StreamBufferIndices GetFullStreamBufferIndices() const
        {
            StreamBufferIndices streamIndices;
            for (size_t idx = 0; idx < m_streamBufferViews.size(); ++idx)
            {
                streamIndices.AddIndex(static_cast<u8>(idx));
            }
            return streamIndices;
        }

        //! Helper function to conveniently create a StreamIterator
        StreamIterator<DeviceGeometryView, DeviceStreamBufferView>
            CreateStreamIterator(const StreamBufferIndices& indices) const
        {
            return StreamIterator<DeviceGeometryView, DeviceStreamBufferView>(this, indices);
        }

        // --- Dummy StreamBufferView ---

        bool HasDummyStreamBufferView() const { return m_dummyStreamBufferIndex != InvalidStreamBufferIndex; }
        u8 GetDummyStreamBufferIndex() const { return m_dummyStreamBufferIndex; }

        // Only call after checking HasDummyStreamBufferView()
        DeviceStreamBufferView GetDummyStreamBufferView()
        {
            AZ_Assert(HasDummyStreamBufferView(), "Calling GetDummyStreamBufferView but no dummy view is set. Application will likely crash.");
            return m_streamBufferViews[m_dummyStreamBufferIndex];
        }
        void AddDummyStreamBufferView(DeviceStreamBufferView streamBufferView)
        {
            AZ_Assert(!HasDummyStreamBufferView(), "Calling AddDummyStreamBufferView but dummy view is already set.");
            m_dummyStreamBufferIndex = static_cast<u8>(m_streamBufferViews.size());
            m_streamBufferViews.emplace_back(streamBufferView);
        }
    };
}
