/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/GeometryView.h>
#include <Atom/RHI/MultiDeviceDrawArguments.h>
#include <Atom/RHI/MultiDeviceIndexBufferView.h>
#include <Atom/RHI/MultiDeviceIndirectBufferView.h>
#include <Atom/RHI/MultiDeviceStreamBufferView.h>
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


    //! MultiDeviceGeometryView is a multi-device class that holds a map of device-specific GeometryView. It also
    //! holds a MultiDeviceDrawArguments, MultiDeviceIndexBufferView and a vector of MultiDeviceStreamBufferViews,
    //! which if edited will set the underlying data on the device-specific GeometryView.
    struct MultiDeviceGeometryView
    {
        inline GeometryView* GetDeviceGeometryView(int deviceIndex)
        {
            if (!m_geometryView.contains(deviceIndex))
            {
                GeometryView newGeometryView;
                newGeometryView.SetDrawArguments( m_multiDrawArguments.GetDeviceDrawArguments(deviceIndex) );
                newGeometryView.SetIndexBufferView( m_multiIndexBufferView.GetDeviceIndexBufferView(deviceIndex) );
                for (MultiDeviceStreamBufferView& stream : m_multiStreamBufferViews)
                {
                    newGeometryView.AddStreamBufferView(stream.GetDeviceStreamBufferView(deviceIndex));
                }

                m_geometryView.emplace(deviceIndex, newGeometryView);
            }

            return &m_geometryView[deviceIndex];
        }

        inline void SetMultiDeviceDrawArguments(MultiDeviceDrawArguments multiDrawArguments)
        {
            m_multiDrawArguments = multiDrawArguments;
            for (auto& [deviceIndex, geometryView] : m_geometryView)
            {
                geometryView.SetDrawArguments( multiDrawArguments.GetDeviceDrawArguments(deviceIndex) );
            }
        }

        inline void SetIndexedArgumentsInstanceCount(uint32_t instanceCount)
        {
            m_multiDrawArguments.m_indexed.m_instanceCount = instanceCount;
            for (auto& [deviceIndex, geometryView] : m_geometryView)
            {
                geometryView.SetIndexInstanceCount(instanceCount);
            }
        }

        inline void SetMultiDeviceIndexBufferView(MultiDeviceIndexBufferView multiIndexBufferView)
        {
            m_multiIndexBufferView = multiIndexBufferView;
            for (auto& [deviceIndex, geometryView] : m_geometryView)
            {
                geometryView.SetIndexBufferView( multiIndexBufferView.GetDeviceIndexBufferView(deviceIndex) );
            }
        }

        inline void SetMultiDeviceStreamBufferViews(const AZStd::vector<MultiDeviceStreamBufferView>& multiStreamBufferViews)
        {
            m_multiStreamBufferViews = multiStreamBufferViews;
            for (auto& [deviceIndex, geometryView] : m_geometryView)
            {
                geometryView.ClearStreamBufferViews();
                for (MultiDeviceStreamBufferView& stream : m_multiStreamBufferViews)
                {
                    geometryView.AddStreamBufferView(stream.GetDeviceStreamBufferView(deviceIndex));
                }
            }
        }

        inline void AddMultiDeviceStreamBufferView(MultiDeviceStreamBufferView multiStreamBufferView)
        {
            m_multiStreamBufferViews.push_back(multiStreamBufferView);
            for (auto& [deviceIndex, geometryView] : m_geometryView)
            {
                geometryView.AddStreamBufferView(multiStreamBufferView.GetDeviceStreamBufferView(deviceIndex));
            }
        }

        //! A map of single-device DrawPackets, index by the device index
        AZStd::unordered_map<int, GeometryView> m_geometryView;

        MultiDeviceDrawArguments m_multiDrawArguments;
        MultiDeviceIndexBufferView m_multiIndexBufferView;
        AZStd::vector<MultiDeviceStreamBufferView> m_multiStreamBufferViews;
    };
}
