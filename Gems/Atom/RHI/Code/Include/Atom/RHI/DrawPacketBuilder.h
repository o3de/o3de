/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI.Reflect/Scissor.h>
#include <Atom/RHI.Reflect/Viewport.h>

namespace AZ
{
    class IAllocatorAllocate;

    namespace RHI
    {
        class DrawPacketBuilder
        {
        public:
            struct DrawRequest
            {
                DrawRequest() = default;

                //! The filter tag used to direct the draw item.
                DrawListTag m_listTag;

                //! The stencil ref value used for this draw item.
                uint8_t m_stencilRef = 0;

                //! The array of stream buffers to bind for this draw item.
                AZStd::array_view<StreamBufferView> m_streamBufferViews;

                //! Shader resource group unique for this draw request
                const ShaderResourceGroup* m_uniqueShaderResourceGroup = nullptr;

                //! The pipeline state assigned to this draw item.
                const PipelineState* m_pipelineState = nullptr;

                //! The sort key assigned to this draw item.
                DrawItemSortKey m_sortKey = 0;

                //! The filter associated to this draw item. 
                DrawFilterMask m_drawFilterMask = DrawFilterMaskDefaultValue;
            };

            // NOTE: This is configurable; just used to control the amount of memory held by the builder.
            static const size_t DrawItemCountMax = 16;

            void Begin(IAllocatorAllocate* allocator);

            void SetDrawArguments(const DrawArguments& drawArguments);

            void SetIndexBufferView(const IndexBufferView& indexBufferView);

            void SetRootConstants(AZStd::array_view<uint8_t> rootConstants);

            void SetScissors(AZStd::array_view<Scissor> scissors);

            void SetScissor(const Scissor& scissor);

            void SetViewports(AZStd::array_view<Viewport> viewports);

            void SetViewport(const Viewport& viewport);

            void AddShaderResourceGroup(const ShaderResourceGroup* shaderResourceGroup);

            void SetDrawFilterMask(DrawFilterMask filterMask);

            void AddDrawItem(const DrawRequest& request);

            const DrawPacket* End();

        private:
            void ClearData();

            IAllocatorAllocate* m_allocator = nullptr;
            DrawArguments m_drawArguments;
            DrawListMask m_drawListMask = 0;
            DrawFilterMask m_drawFilterMask = DrawFilterMaskDefaultValue;
            size_t m_streamBufferViewCount = 0;
            IndexBufferView m_indexBufferView;
            AZStd::fixed_vector<DrawRequest, DrawItemCountMax> m_drawRequests;
            AZStd::fixed_vector<const ShaderResourceGroup*, Limits::Pipeline::ShaderResourceGroupCountMax> m_shaderResourceGroups;
            AZStd::array_view<uint8_t> m_rootConstants;
            AZStd::fixed_vector<Scissor, Limits::Pipeline::AttachmentColorCountMax> m_scissors;
            AZStd::fixed_vector<Viewport, Limits::Pipeline::AttachmentColorCountMax> m_viewports;
        };
    }
}
