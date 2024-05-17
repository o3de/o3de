/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>
#include <Atom/RHI/SingleDeviceDrawPacketBuilder.h>
#include <Atom/RHI/MultiDeviceDrawPacket.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/SingleDeviceStreamBufferView.h>

namespace AZ
{
    class IAllocator;
} // namespace AZ

namespace AZ::RHI
{
    class MultiDeviceDrawPacketBuilder
    {
    public:
        struct MultiDeviceDrawRequest
        {
            MultiDeviceDrawRequest() = default;

            //! Returns the device-specific SingleDeviceDrawRequest for the given index
            SingleDeviceDrawPacketBuilder::SingleDeviceDrawRequest GetDeviceDrawRequest(int deviceIndex);

            //! The filter tag used to direct the draw item.
            DrawListTag m_listTag;

            //! The stencil ref value used for this draw item.
            uint8_t m_stencilRef{};

            //! The array of stream buffers to bind for this draw item.
            AZStd::span<const MultiDeviceStreamBufferView> m_streamBufferViews;

            //! Shader resource group unique for this draw request
            const MultiDeviceShaderResourceGroup* m_uniqueShaderResourceGroup{};

            //! The pipeline state assigned to this draw item.
            const MultiDevicePipelineState* m_pipelineState{};

            //! The sort key assigned to this draw item.
            DrawItemSortKey m_sortKey{};

            //! Mask for filtering the draw item into specific render pipelines.
            //! We use a mask because the same item could be reused in multiple pipelines. For example, a simple
            //! depth pre-pass could be present in multiple pipelines.
            DrawFilterMask m_drawFilterMask = DrawFilterMaskDefaultValue;

            //! A map of all device-specific StreamBufferViews, indexed by the device index
            //! This additional cache is needed since device-specific StreamBufferViews are returned as objects
            //! and the device-specific SingleDeviceDrawItem holds a pointer to it.
            AZStd::unordered_map<int, AZStd::vector<SingleDeviceStreamBufferView>> m_deviceStreamBufferViews;
        };

        explicit MultiDeviceDrawPacketBuilder(RHI::MultiDevice::DeviceMask deviceMask)
            : m_deviceMask{ deviceMask }
        {
            auto deviceCount{ RHI::RHISystemInterface::Get()->GetDeviceCount() };

            for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
            {
                // cast to u8 to prevent warning
                if (RHI::CheckBit(AZStd::to_underlying(m_deviceMask), static_cast<AZ::u8>(deviceIndex)))
                {
                    m_deviceDrawPacketBuilders.emplace(deviceIndex, SingleDeviceDrawPacketBuilder());
                }
            }
        }

        MultiDeviceDrawPacketBuilder(const MultiDeviceDrawPacketBuilder& other);
        MultiDeviceDrawPacketBuilder& operator=(const MultiDeviceDrawPacketBuilder& other);
        AZ_DISABLE_MOVE(MultiDeviceDrawPacketBuilder)

        // NOTE: This is configurable; just used to control the amount of memory held by the builder.
        static const size_t DrawItemCountMax = 16;

        //! Passes the linear allocator to all single-device DrawPacketBuilders and
        //! initializes the multi-device SingleDeviceDrawPacket which will be returned after calling End()
        void Begin(IAllocator* allocator);

        //! Passes the SingleDeviceDrawArguments to all single-device DrawPacketBuilders
        void SetDrawArguments(const MultiDeviceDrawArguments& drawArguments);

        //! Passes the IndexBufferViews to all single-device DrawPacketBuilders
        void SetIndexBufferView(const MultiDeviceIndexBufferView& indexBufferView);

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

        //! Passes the SingleDeviceShaderResourceGroup to all single-device DrawPacketBuilders
        void AddShaderResourceGroup(const MultiDeviceShaderResourceGroup* shaderResourceGroup);

        //! Passes the single-device DrawRequests to all single-device DrawPacketBuilders,
        //! keeps the multi-device SingleDeviceDrawRequest and sets the DrawListMask in the current
        //! multi-device SingleDeviceDrawPacket
        void AddDrawItem(MultiDeviceDrawRequest& request);

        //! Builds all single-device DrawPackets linearly in memory using their allocator
        //! and captures them in the multi-device SingleDeviceDrawPacket, correctly linking the
        //! single-device DrawItems with the corresponding multi-device SingleDeviceDrawItem as well
        RHI::Ptr<MultiDeviceDrawPacket> End();

        //! Clones all single-device DrawPackets and then sets all corresponding pointers
        //! in the multi-device SingleDeviceDrawPacket and SingleDeviceDrawItem objects
        RHI::Ptr<MultiDeviceDrawPacket> Clone(const MultiDeviceDrawPacket* original);

    private:
        RHI::MultiDevice::DeviceMask m_deviceMask{ 0u };
        AZStd::fixed_vector<MultiDeviceDrawRequest, SingleDeviceDrawPacketBuilder::DrawItemCountMax> m_drawRequests;

        RHI::Ptr<MultiDeviceDrawPacket> m_drawPacketInFlight;

        //! A map of single-device SingleDeviceDrawPacketBuilder, indexed by the device index
        AZStd::unordered_map<int, SingleDeviceDrawPacketBuilder> m_deviceDrawPacketBuilders;
    };
} // namespace AZ::RHI
