/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/SingleDeviceDrawPacket.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>

namespace AZ
{
    class IAllocator;
}

namespace AZ::RHI
{
    class SingleDeviceDrawPacketBuilder
    {
    public:
        struct SingleDeviceDrawRequest
        {
            SingleDeviceDrawRequest() = default;

            //! The filter tag used to direct the draw item.
            DrawListTag m_listTag;

            //! The stencil ref value used for this draw item.
            uint8_t m_stencilRef = 0;

            //! The array of stream buffers to bind for this draw item.
            AZStd::span<const SingleDeviceStreamBufferView> m_streamBufferViews;

            //! Shader resource group unique for this draw request
            const SingleDeviceShaderResourceGroup* m_uniqueShaderResourceGroup = nullptr;

            //! The pipeline state assigned to this draw item.
            const SingleDevicePipelineState* m_pipelineState = nullptr;

            //! The sort key assigned to this draw item.
            DrawItemSortKey m_sortKey = 0;

            //! Mask for filtering the draw item into specific render pipelines.
            //! We use a mask because the same item could be reused in multiple pipelines. For example, a simple
            //! depth pre-pass could be present in multiple pipelines.
            DrawFilterMask m_drawFilterMask = DrawFilterMaskDefaultValue;
        };

        // NOTE: This is configurable; just used to control the amount of memory held by the builder.
        static const size_t DrawItemCountMax = 16;

        void Begin(IAllocator* allocator);

        void SetDrawArguments(const SingleDeviceDrawArguments& drawArguments);

        void SetIndexBufferView(const SingleDeviceIndexBufferView& indexBufferView);

        void SetRootConstants(AZStd::span<const uint8_t> rootConstants);

        void SetScissors(AZStd::span<const Scissor> scissors);

        void SetScissor(const Scissor& scissor);

        void SetViewports(AZStd::span<const Viewport> viewports);

        void SetViewport(const Viewport& viewport);

        void AddShaderResourceGroup(const SingleDeviceShaderResourceGroup* shaderResourceGroup);

        void AddDrawItem(const SingleDeviceDrawRequest& request);

        SingleDeviceDrawPacket* End();

        //! Make a copy of an existing SingleDeviceDrawPacket.
        //! Note: the copy will reference the same DrawSrg as the original, so it is not possible to vary the DrawSrg values between the
        //! original draw packet and the cloned one. Only settings that can be modified via the SingleDeviceDrawPacket interface can be changed
        //! after cloning, such as SetRootConstant and SetInstanceCount
        SingleDeviceDrawPacket* Clone(const SingleDeviceDrawPacket* original);

    private:
        void ClearData();

        IAllocator* m_allocator = nullptr;
        SingleDeviceDrawArguments m_drawArguments;
        DrawListMask m_drawListMask = 0;
        size_t m_streamBufferViewCount = 0;
        SingleDeviceIndexBufferView m_indexBufferView;
        AZStd::fixed_vector<SingleDeviceDrawRequest, DrawItemCountMax> m_drawRequests;
        AZStd::fixed_vector<const SingleDeviceShaderResourceGroup*, Limits::Pipeline::ShaderResourceGroupCountMax> m_shaderResourceGroups;
        AZStd::span<const uint8_t> m_rootConstants;
        AZStd::fixed_vector<Scissor, Limits::Pipeline::AttachmentColorCountMax> m_scissors;
        AZStd::fixed_vector<Viewport, Limits::Pipeline::AttachmentColorCountMax> m_viewports;
    };
}
