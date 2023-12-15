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
#include <Atom/RHI/DrawPacketBuilder.h>
#include <Atom/RHI/MultiDeviceDrawPacket.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI/StreamBufferView.h>

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

            //! Returns the device-specific RayTracingTlasDescriptor for the given index
            DrawPacketBuilder::DrawRequest GetDeviceDrawRequest(int deviceIndex);

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
            //! and the device-specific DrawItem holds a pointer to it.
            AZStd::unordered_map<int, AZStd::vector<StreamBufferView>> m_deviceStreamBufferViews;
        };

        explicit MultiDeviceDrawPacketBuilder(RHI::MultiDevice::DeviceMask deviceMask)
            : m_deviceMask{ deviceMask }
        {
            auto deviceCount{ RHI::RHISystemInterface::Get()->GetDeviceCount() };

            for (int deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
            {
                if ((AZStd::to_underlying(m_deviceMask) >> deviceIndex) & 1)
                {
                    m_deviceDrawPacketBuilders.emplace(deviceIndex, DrawPacketBuilder());
                }
            }
        }

        // NOTE: This is configurable; just used to control the amount of memory held by the builder.
        static const size_t DrawItemCountMax = 16;

        void Begin(IAllocator* allocator);

        void SetDrawArguments(const MultiDeviceDrawArguments& drawArguments);

        void SetIndexBufferView(const MultiDeviceIndexBufferView& indexBufferView);

        void SetRootConstants(AZStd::span<const uint8_t> rootConstants);

        void SetScissors(AZStd::span<const Scissor> scissors);

        void SetScissor(const Scissor& scissor);

        void SetViewports(AZStd::span<const Viewport> viewports);

        void SetViewport(const Viewport& viewport);

        void AddShaderResourceGroup(const MultiDeviceShaderResourceGroup* shaderResourceGroup);

        void AddDrawItem(MultiDeviceDrawRequest& request);

        RHI::Ptr<MultiDeviceDrawPacket> End();

        //! Make a copy of an existing DrawPacket.
        //! Note: the copy will reference the same DrawSrg as the original, so it is not possible to vary the DrawSrg values between the
        //! original draw packet and the cloned one. Only settings that can be modified via the DrawPacket interface can be changed
        //! after cloning, such as SetRootConstant and SetInstanceCount
        RHI::Ptr<MultiDeviceDrawPacket> Clone(const MultiDeviceDrawPacket* original);

    private:
        void ClearData();

        RHI::MultiDevice::DeviceMask m_deviceMask{ 0u };
        AZStd::fixed_vector<MultiDeviceDrawRequest, DrawPacketBuilder::DrawItemCountMax> m_drawRequests;

        RHI::Ptr<MultiDeviceDrawPacket> m_drawPacketInFlight;
        // MultiDeviceDrawArguments m_drawArguments;
        // size_t m_streamBufferViewCount{};
        // MultiDeviceIndexBufferView m_indexBufferView;
        // AZStd::vector<MultiDeviceStreamBufferView> m_streamBufferViews;
        // AZStd::fixed_vector<const MultiDeviceShaderResourceGroup*, Limits::Pipeline::ShaderResourceGroupCountMax> m_shaderResourceGroups;
        // AZStd::span<const uint8_t> m_rootConstants;
        // AZStd::fixed_vector<Scissor, Limits::Pipeline::AttachmentColorCountMax> m_scissors;
        // AZStd::fixed_vector<Viewport, Limits::Pipeline::AttachmentColorCountMax> m_viewports;

        //! A map of single-device DrawPacketBuilder, indexed by the device index
        AZStd::unordered_map<int, DrawPacketBuilder> m_deviceDrawPacketBuilders;
    };
} // namespace AZ::RHI
