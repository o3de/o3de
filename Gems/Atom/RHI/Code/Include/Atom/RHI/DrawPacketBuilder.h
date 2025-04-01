/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/GeometryView.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI/DeviceDrawPacketBuilder.h>
#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI/DeviceStreamBufferView.h>

namespace AZ
{
    class IAllocator;
} // namespace AZ

namespace AZ::RHI
{
    //! DrawPacketBuilder builds a DrawPacket and its DrawItems contiguously in memory for cache efficiency
    //! Start by calling DrawPacketBuilder::Begin( )
    //! Then Set the necessary data and add a DrawRequest for each DrawItem
    //! Finalize the DrawPacket with a call to DrawPacketBuilder::End( )
    class DrawPacketBuilder
    {
    public:
        struct DrawRequest
        {
            DrawRequest() = default;

            //! Returns the device-specific DeviceDrawRequest for the given index
            DeviceDrawPacketBuilder::DeviceDrawRequest GetDeviceDrawRequest(int deviceIndex);

            //! The filter tag used to direct the draw item.
            DrawListTag m_listTag;

            //! The stencil ref value used for this draw item.
            u8 m_stencilRef = 0;

            //! Indices of the StreamBufferViews the DrawItem will use
            RHI::StreamBufferIndices m_streamIndices;

            //! Shader resource group unique for this draw request
            const ShaderResourceGroup* m_uniqueShaderResourceGroup{};

            //! The pipeline state assigned to this draw item.
            const PipelineState* m_pipelineState{};

            //! The sort key assigned to this draw item.
            DrawItemSortKey m_sortKey{};

            //! Mask for filtering the draw item into specific render pipelines.
            //! We use a mask because the same item could be reused in multiple pipelines. For example, a simple
            //! depth pre-pass could be present in multiple pipelines.
            DrawFilterMask m_drawFilterMask = DrawFilterMaskDefaultValue;
        };

        explicit DrawPacketBuilder(RHI::MultiDevice::DeviceMask deviceMask)
            : m_deviceMask{ deviceMask }
        {
            MultiDeviceObject::IterateDevices(
                m_deviceMask,
                [this](int deviceIndex)
                {
                    m_deviceDrawPacketBuilders.emplace(deviceIndex, DeviceDrawPacketBuilder());
                    return true;
                });
        }

        DrawPacketBuilder(const DrawPacketBuilder& other);
        DrawPacketBuilder& operator=(const DrawPacketBuilder& other);
        AZ_DISABLE_MOVE(DrawPacketBuilder)

        // NOTE: This is configurable; just used to control the amount of memory held by the builder.
        static const size_t DrawItemCountMax = 16;

        //! Passes the linear allocator to all single-device DrawPacketBuilders and
        //! initializes the multi-device DeviceDrawPacket which will be returned after calling End()
        void Begin(IAllocator* allocator);

        //! Passes the GeometryView to all single-device DrawPacketBuilders
        void SetGeometryView(GeometryView* geometryView);

        //! Passes the DrawInstanceArguments to all single-device DrawPacketBuilders
        void SetDrawInstanceArguments(DrawInstanceArguments drawInstanceArguments);

        //! Passes the RootConstants to all single-device DrawPacketBuilders
        void SetRootConstants(AZStd::span<const uint8_t> rootConstants);

        //! Passes the Scissors to all single-device DrawPacketBuilders
        void SetScissors(AZStd::span<const Scissor> scissors);

        //! Passes a Scissor to all single-device DrawPacketBuilders
        void SetScissor(const Scissor& scissor);

        //! Passes the Viewports to all single-device DrawPacketBuilders
        void SetViewports(AZStd::span<const Viewport> viewports);

        //! Passes a Viewport to all singl-device DrawPacketBuilders
        void SetViewport(const Viewport& viewport);

        //! Passes the DeviceShaderResourceGroup to all single-device DrawPacketBuilders
        void AddShaderResourceGroup(const ShaderResourceGroup* shaderResourceGroup);

        //! Passes the single-device DrawRequests to all single-device DrawPacketBuilders,
        //! keeps the multi-device DeviceDrawRequest and sets the DrawListMask in the current
        //! multi-device DeviceDrawPacket
        void AddDrawItem(DrawRequest& request);

        //! Builds all single-device DrawPackets linearly in memory using their allocator
        //! and captures them in the multi-device DeviceDrawPacket, correctly linking the
        //! single-device DrawItems with the corresponding multi-device DeviceDrawItem as well
        RHI::Ptr<DrawPacket> End();

        //! Clones all single-device DrawPackets and then sets all corresponding pointers
        //! in the multi-device DeviceDrawPacket and DeviceDrawItem objects
        RHI::Ptr<DrawPacket> Clone(const DrawPacket* original);

    private:
        RHI::MultiDevice::DeviceMask m_deviceMask{ 0u };
        AZStd::fixed_vector<DrawRequest, DeviceDrawPacketBuilder::DrawItemCountMax> m_drawRequests;

        RHI::Ptr<DrawPacket> m_drawPacketInFlight;

        //! A map of single-device DeviceDrawPacketBuilder, indexed by the device index
        AZStd::unordered_map<int, DeviceDrawPacketBuilder> m_deviceDrawPacketBuilders;
    };
} // namespace AZ::RHI
