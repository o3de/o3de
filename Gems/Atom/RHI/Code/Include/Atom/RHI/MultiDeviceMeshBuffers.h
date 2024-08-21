/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI/MeshBuffers.h>
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


    struct MultiDeviceMeshBuffers
    {
        inline MeshBuffers* GetDeviceMeshBuffers(int deviceIndex)
        {
            if (!m_meshBuffers.contains(deviceIndex))
            {
                MeshBuffers newMeshBuffers;
                newMeshBuffers.SetDrawArguments( m_multiDrawArguments.GetDeviceDrawArguments(deviceIndex) );
                newMeshBuffers.SetIndexBufferView( m_multiIndexBufferView.GetDeviceIndexBufferView(deviceIndex) );
                for (MultiDeviceStreamBufferView& stream : m_multiStreamBufferViews)
                {
                    newMeshBuffers.AddStreamBufferView(stream.GetDeviceStreamBufferView(deviceIndex));
                }

                m_meshBuffers.emplace(deviceIndex, newMeshBuffers);
            }

            return &m_meshBuffers[deviceIndex];
        }

        inline void SetMultiDeviceDrawArguments(MultiDeviceDrawArguments multiDrawArguments)
        {
            m_multiDrawArguments = multiDrawArguments;
            for (auto& [deviceIndex, meshBuffers] : m_meshBuffers)
            {
                meshBuffers.SetDrawArguments( multiDrawArguments.GetDeviceDrawArguments(deviceIndex) );
            }
        }

        inline void SetIndexedArgumentsInstanceCount(uint32_t instanceCount)
        {
            m_multiDrawArguments.m_indexed.m_instanceCount = instanceCount;
            for (auto& [deviceIndex, meshBuffers] : m_meshBuffers)
            {
                meshBuffers.SetIndexInstanceCount(instanceCount);
            }
        }

        inline void SetMultiDeviceIndexBufferView(MultiDeviceIndexBufferView multiIndexBufferView)
        {
            m_multiIndexBufferView = multiIndexBufferView;
            for (auto& [deviceIndex, meshBuffers] : m_meshBuffers)
            {
                meshBuffers.SetIndexBufferView( multiIndexBufferView.GetDeviceIndexBufferView(deviceIndex) );
            }
        }

        inline void SetMultiDeviceStreamBufferViews(const AZStd::vector<MultiDeviceStreamBufferView>& multiStreamBufferViews)
        {
            m_multiStreamBufferViews = multiStreamBufferViews;
            for (auto& [deviceIndex, meshBuffers] : m_meshBuffers)
            {
                meshBuffers.ClearStreamBufferViews();
                for (MultiDeviceStreamBufferView& stream : m_multiStreamBufferViews)
                {
                    meshBuffers.AddStreamBufferView(stream.GetDeviceStreamBufferView(deviceIndex));
                }
            }
        }

        inline void AddMultiDeviceStreamBufferView(MultiDeviceStreamBufferView multiStreamBufferView)
        {
            m_multiStreamBufferViews.push_back(multiStreamBufferView);
            for (auto& [deviceIndex, meshBuffers] : m_meshBuffers)
            {
                meshBuffers.AddStreamBufferView(multiStreamBufferView.GetDeviceStreamBufferView(deviceIndex));
            }
        }

        //! A map of single-device DrawPackets, index by the device index
        AZStd::unordered_map<int, MeshBuffers> m_meshBuffers;

        MultiDeviceDrawArguments m_multiDrawArguments;
        MultiDeviceIndexBufferView m_multiIndexBufferView;
        AZStd::vector<MultiDeviceStreamBufferView> m_multiStreamBufferViews;
    };
}
