/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/DeviceGeometryView.h>
#include <Atom/RHI/DrawArguments.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/IndirectBufferView.h>
#include <Atom/RHI/StreamBufferView.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>

namespace AZ::RHI
{
    //! GeometryView is a multi-device class that holds a map of device-specific GeometryView. It also
    //! holds a DrawArguments, IndexBufferView and a vector of StreamBufferViews,
    //! which if edited will set the underlying data on the device-specific GeometryView.
    class GeometryView
    {
        //! A map of single-device DrawPackets, index by the device index
        AZStd::unordered_map<int, DeviceGeometryView> m_geometryViews;

        DrawArguments m_drawArguments;
        IndexBufferView m_indexBufferView;
        AZStd::vector<StreamBufferView> m_streamBufferViews;

        //! Dummy StreamBufferView is used when shader requires an options stream that has not been provided by the user
        static constexpr u8 InvalidStreamBufferIndex = 0xFF;
        u8 m_dummyStreamBufferIndex = InvalidStreamBufferIndex;

    public:
        friend class StreamIterator<GeometryView, StreamBufferView>;

        inline DeviceGeometryView* GetDeviceGeometryView(int deviceIndex)
        {
            auto iter = m_geometryViews.find(deviceIndex);
            if (iter == m_geometryViews.end())
            {
                DeviceGeometryView newGeometryView;
                newGeometryView.SetDrawArguments( m_drawArguments.GetDeviceDrawArguments(deviceIndex) );
                if (m_indexBufferView.IsValid())
                {
                    newGeometryView.SetIndexBufferView(m_indexBufferView.GetDeviceIndexBufferView(deviceIndex));
                }
                for (StreamBufferView& stream : m_streamBufferViews)
                {
                    newGeometryView.AddStreamBufferView(stream.GetDeviceStreamBufferView(deviceIndex));
                }
                newGeometryView.m_dummyStreamBufferIndex = m_dummyStreamBufferIndex;
                m_geometryViews.emplace(deviceIndex, std::move(newGeometryView));
                return &m_geometryViews[deviceIndex];
            }
            else
            {
                return &iter->second;
            }
        }

        void Reset()
        {
            m_drawArguments = DrawArguments{};
            m_indexBufferView = IndexBufferView{};
            m_streamBufferViews.clear();
            m_dummyStreamBufferIndex = InvalidStreamBufferIndex;
            for (auto& [deviceIndex, geometryView] : m_geometryViews)
            {
                geometryView.Reset();
            }
        }

        // --- Draw Arguments ---

        inline const DrawArguments& GetDrawArguments() const { return m_drawArguments; }

        inline void SetDrawArguments(DrawArguments drawArguments)
        {
            m_drawArguments = drawArguments;
            for (auto& [deviceIndex, geometryView] : m_geometryViews)
            {
                geometryView.SetDrawArguments( drawArguments.GetDeviceDrawArguments(deviceIndex) );
            }
        }

        // --- Index Buffer View ---

        inline const IndexBufferView& GetIndexBufferView() const { return m_indexBufferView; }

        inline void SetIndexBufferView(IndexBufferView indexBufferView)
        {
            m_indexBufferView = indexBufferView;
            for (auto& [deviceIndex, geometryView] : m_geometryViews)
            {
                geometryView.SetIndexBufferView( indexBufferView.GetDeviceIndexBufferView(deviceIndex) );
            }
        }

        // --- Stream Buffer Views ---

        const StreamBufferView& GetStreamBufferView(u8 idx) const { return m_streamBufferViews[idx]; }
        AZStd::vector<StreamBufferView>& GetStreamBufferViews() { return m_streamBufferViews; }
        const AZStd::vector<StreamBufferView>& GetStreamBufferViews() const { return m_streamBufferViews; }

        inline void SetStreamBufferView(u8 idx, StreamBufferView streamBufferView)
        {
            m_streamBufferViews[idx] = streamBufferView;
            for (auto& [deviceIndex, geometryView] : m_geometryViews)
            {
                geometryView.SetStreamBufferView(idx, streamBufferView.GetDeviceStreamBufferView(deviceIndex));
            }
        }

        inline void SetStreamBufferViews(const AZStd::vector<StreamBufferView>& streamBufferViews)
        {
            m_streamBufferViews = streamBufferViews;
            for (auto& [deviceIndex, geometryView] : m_geometryViews)
            {
                geometryView.ClearStreamBufferViews();
                for (StreamBufferView& stream : m_streamBufferViews)
                {
                    geometryView.AddStreamBufferView(stream.GetDeviceStreamBufferView(deviceIndex));
                }
            }
        }

        inline void AddStreamBufferView(StreamBufferView streamBufferView)
        {
            m_streamBufferViews.push_back(streamBufferView);
            for (auto& [deviceIndex, geometryView] : m_geometryViews)
            {
                geometryView.AddStreamBufferView(streamBufferView.GetDeviceStreamBufferView(deviceIndex));
            }
        }

        void ClearStreamBufferViews()
        {
            m_streamBufferViews.clear();
            m_dummyStreamBufferIndex = InvalidStreamBufferIndex;
            for (auto& [deviceIndex, geometryView] : m_geometryViews)
            {
                geometryView.ClearStreamBufferViews();
            }
        }

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
        StreamIterator<GeometryView, StreamBufferView> CreateStreamIterator(const StreamBufferIndices& indices) const
        {
            return StreamIterator<GeometryView, StreamBufferView>(this, indices);
        }

        // --- Dummy StreamBufferView ---

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
            m_dummyStreamBufferIndex = static_cast<u8>(m_streamBufferViews.size());
            m_streamBufferViews.emplace_back(streamBufferView);
            for (auto& [deviceIndex, geometryView] : m_geometryViews)
            {
                geometryView.AddDummyStreamBufferView(streamBufferView.GetDeviceStreamBufferView(deviceIndex));
            }
        }
    };

    //! Validates the stream buffer views in a GeometryView
    bool ValidateStreamBufferViews(const InputStreamLayout& inputStreamLayout, RHI::GeometryView& geometryView,
        const RHI::StreamBufferIndices& streamIndices);

}
