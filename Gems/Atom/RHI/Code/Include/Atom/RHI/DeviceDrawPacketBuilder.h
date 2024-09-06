/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DeviceDrawPacket.h>
#include <Atom/RHI/DeviceGeometryView.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>

namespace AZ
{
    class IAllocator;
}

namespace AZ::RHI
{
    class DeviceDrawPacketBuilder
    {
    public:
        struct DeviceDrawRequest
        {
            DeviceDrawRequest() = default;

            //! The filter tag used to direct the draw item.
            DrawListTag m_listTag;

            //! The stencil ref value used for this draw item.
            uint8_t m_stencilRef = 0;

            //! Indices of the StreamBufferViews the draw item will use.
            StreamBufferIndices m_streamIndices;

            //! Shader resource group unique for this draw request
            const DeviceShaderResourceGroup* m_uniqueShaderResourceGroup = nullptr;

            //! The pipeline state assigned to this draw item.
            const DevicePipelineState* m_pipelineState = nullptr;

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

        void SetGeometryView(const DeviceGeometryView* geometryView);

        void SetDrawInstanceArguments(const DrawInstanceArguments& drawInstanceArgs);

        void SetRootConstants(AZStd::span<const uint8_t> rootConstants);

        void SetScissors(AZStd::span<const Scissor> scissors);

        void SetScissor(const Scissor& scissor);

        void SetViewports(AZStd::span<const Viewport> viewports);

        void SetViewport(const Viewport& viewport);

        void AddShaderResourceGroup(const DeviceShaderResourceGroup* shaderResourceGroup);

        void AddDrawItem(const DeviceDrawRequest& request);

        DeviceDrawPacket* End();

        //! Make a copy of an existing DeviceDrawPacket.
        //! Note: the copy will reference the same DrawSrg as the original, so it is not possible to vary the DrawSrg values between the
        //! original draw packet and the cloned one. Only settings that can be modified via the DeviceDrawPacket interface can be changed
        //! after cloning, such as SetRootConstant and SetInstanceCount
        DeviceDrawPacket* Clone(const DeviceDrawPacket* original);

    private:
        void ClearData();

        IAllocator* m_allocator = nullptr;
        const DeviceGeometryView* m_geometryView = nullptr;
        DrawInstanceArguments m_drawInstanceArgs;
        DrawListMask m_drawListMask = 0;
        AZStd::fixed_vector<DeviceDrawRequest, DrawItemCountMax> m_drawRequests;
        AZStd::fixed_vector<const DeviceShaderResourceGroup*, Limits::Pipeline::ShaderResourceGroupCountMax> m_shaderResourceGroups;
        AZStd::span<const uint8_t> m_rootConstants;
        AZStd::fixed_vector<Scissor, Limits::Pipeline::AttachmentColorCountMax> m_scissors;
        AZStd::fixed_vector<Viewport, Limits::Pipeline::AttachmentColorCountMax> m_viewports;
    };
}
