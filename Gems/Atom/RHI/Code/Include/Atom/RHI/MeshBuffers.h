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
#include <Atom/RHI/IndirectBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::RHI
{
    class PipelineState;
    class ShaderResourceGroup;
    struct Scissor;
    struct Viewport;
    struct DefaultNamespaceType;
    // Forward declaration to
    template <typename T , typename NamespaceType>
    struct Handle;


    class MeshBuffers
    {
    public:

        struct Interval
        {
            Interval() = default;
            Interval(u8 st, u8 en)
                : start(st)
                , end(en)
            { }
            u8 start = 0xFF;
            u8 end = 0;
            u8 size() { return end - start; }
            bool IsValid() { return start <= end; }
        };

        class StreamIterator {
        private:
            const MeshBuffers* m_meshBuffers;
            Interval m_interval;
            u8 m_current;
        public:
            StreamIterator(const MeshBuffers* meshBuffers, Interval interval)
                : m_meshBuffers(meshBuffers)
                , m_interval(interval)
                , m_current(interval.start)
            {}

            bool HasEnded() const {  return m_current >= m_interval.end; }
            void Reset() { m_current = m_interval.start; }

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
                return m_meshBuffers->GetStreamBufferViewUsingIndirection(m_current);
            }

            const StreamBufferView* operator->()
            {
                return &m_meshBuffers->GetStreamBufferViewUsingIndirection(m_current);
            }

            const StreamBufferView& operator[](u8 idx) const
            {
                AZ_Assert(idx < (m_interval.end - m_interval.start), "Index %d exceeds interval. Interval Start [%d] - Interval End [%d] - Interval Range [%]",
                    idx, m_interval.start, m_interval.end, m_interval.end - m_interval.start);

                return m_meshBuffers->GetStreamBufferViewUsingIndirection(m_interval.start + idx);
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
            m_streamViewIndirection.clear();
        }
        void AddStreamBufferView(StreamBufferView streamBufferView) { m_streamBufferViews.emplace_back(streamBufferView); }
        const StreamBufferView& GetStreamBufferView(u8 idx) const { return m_streamBufferViews[idx]; }
        AZStd::vector<StreamBufferView>& GetStreamBufferViews() { return m_streamBufferViews; }
        const AZStd::vector<StreamBufferView>& GetStreamBufferViews() const { return m_streamBufferViews; }

        const StreamBufferView& GetStreamBufferViewUsingIndirection(u8 indirectionIdx) const
        {
            u8 streamBufferViewIdx = m_streamViewIndirection[indirectionIdx];
            return m_streamBufferViews[streamBufferViewIdx];
        }

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

        u8 GetNextIndirectionIndex() { return (u8)m_streamViewIndirection.size(); }
        void AddIndirectionIndex(u8 indirectionIndex) { m_streamViewIndirection.push_back(indirectionIndex); }

        StreamIterator CreateStreamIterator(Interval interval) const { return StreamIterator(this, interval); }

        void SetIndexInstanceCount(u32 count) { m_drawArguments.m_indexed.m_instanceCount = count; }

    private:

        DrawArguments m_drawArguments;
        IndexBufferView m_indexBufferView;
        AZStd::vector<StreamBufferView> m_streamBufferViews;
        AZStd::vector<u8> m_streamViewIndirection;

        static constexpr u8 InvalidStreamBufferIndex = 0xFF;
        u8 m_dummyStreamBufferIndex = InvalidStreamBufferIndex;
    };

    bool ValidateStreamBufferViews(const InputStreamLayout& inputStreamLayout, RHI::MeshBuffers& meshBuffers,
        RHI::MeshBuffers::Interval streamIndexInterval);
}
