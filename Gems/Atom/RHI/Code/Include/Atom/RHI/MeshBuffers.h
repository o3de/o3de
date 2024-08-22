/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DrawArguments.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::RHI
{
    class InputStreamLayout;

    class MeshBuffers
    {
    public:

        class StreamBufferIndices
        {
            u8 m_count = 0;
            u8 m_indices[RHI::Limits::Pipeline::StreamCountMax / 2];

        public:
            void AddIndex(u8 index)
            {
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

            u8 GetCount() const { return m_count; }
            void Reset() { m_count = 0; }
        };

        class StreamIterator {
        private:
            const MeshBuffers* m_meshBuffers;
            const StreamBufferIndices& m_indices;
            u8 m_current = 0;
        public:
            StreamIterator(const MeshBuffers* meshBuffers, const StreamBufferIndices& indices)
                : m_meshBuffers(meshBuffers)
                , m_indices(indices)
            {}

            bool HasEnded() const { return m_current >= m_indices.GetCount(); }
            void Reset() { m_current = 0; }

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

            const StreamBufferView& operator*()
            {
                return m_meshBuffers->GetStreamBufferView(m_indices.GetIndex(m_current));
            }

            const StreamBufferView* operator->()
            {
                return &m_meshBuffers->GetStreamBufferView(m_indices.GetIndex(m_current));
            }

            const StreamBufferView& operator[](u16 idx) const
            {
                AZ_Assert((u8)idx < m_indices.GetCount(), "Index %d exceeds number of indices (%d) for stream buffer views",
                    idx, m_indices.GetCount());

                return m_meshBuffers->GetStreamBufferView(m_indices.GetIndex((u8)idx));
            }

        };

        friend class StreamIterator;

        void Reset()
        {
            m_drawArguments = DrawArguments{};
            m_indexBufferView = IndexBufferView{};
            ClearStreamBufferViews();
        }

        void SetDrawArguments(DrawArguments drawArguments) { m_drawArguments = drawArguments; }
        const DrawArguments& GetDrawArguments() const { return m_drawArguments; }

        void SetIndexBufferView(IndexBufferView indexBufferView) { m_indexBufferView = indexBufferView; }
        const IndexBufferView& GetIndexBufferView() const { return m_indexBufferView; }

        void ClearStreamBufferViews()
        {
            m_streamBufferViews.clear();
            m_dummyStreamBufferIndex = InvalidStreamBufferIndex;
        }
        void AddStreamBufferView(StreamBufferView streamBufferView) { m_streamBufferViews.emplace_back(streamBufferView); }
        const StreamBufferView& GetStreamBufferView(u8 idx) const { return m_streamBufferViews[idx]; }
        AZStd::vector<StreamBufferView>& GetStreamBufferViews() { return m_streamBufferViews; }
        const AZStd::vector<StreamBufferView>& GetStreamBufferViews() const { return m_streamBufferViews; }

        bool HasDummyStreamBufferView() const { return m_dummyStreamBufferIndex != InvalidStreamBufferIndex; }
        u8 GetDummyStreamBufferIndex() const { return m_dummyStreamBufferIndex; }

        // Only call after checking HasDummyStreamBufferView()
        StreamBufferView GetDummyStreamBufferView()
        {
            AZ_Assert(HasDummyStreamBufferView(), "Calling GetDummyStreamBufferView but no dummy view is set. Application will likely crash.");
            return m_streamBufferViews[m_dummyStreamBufferIndex];
        }
        void AddDummyStreamBufferView(StreamBufferView streamBufferView)
        {
            AZ_Assert(!HasDummyStreamBufferView(), "Calling AddDummyStreamBufferView but dummy view is already set.");
            m_dummyStreamBufferIndex = (u8)m_streamBufferViews.size();
            m_streamBufferViews.emplace_back(streamBufferView);
        }

        StreamIterator CreateStreamIterator(const StreamBufferIndices& indices) const { return StreamIterator(this, indices); }

        void SetIndexInstanceCount(u32 count) { m_drawArguments.m_indexed.m_instanceCount = count; }

        StreamBufferIndices GetFullStreamBufferIndices() const
        {
            StreamBufferIndices streamIndices;
            for (size_t idx = 0; idx < m_streamBufferViews.size(); ++idx)
            {
                streamIndices.AddIndex((u8)idx);
            }
            return streamIndices;
        }

    private:

        DrawArguments m_drawArguments;
        IndexBufferView m_indexBufferView;
        AZStd::vector<StreamBufferView> m_streamBufferViews;

        static constexpr u8 InvalidStreamBufferIndex = 0xFF;
        u8 m_dummyStreamBufferIndex = InvalidStreamBufferIndex;
    };

    bool ValidateStreamBufferViews(const InputStreamLayout& inputStreamLayout, RHI::MeshBuffers& meshBuffers,
        const RHI::MeshBuffers::StreamBufferIndices& streamIndices);
}
