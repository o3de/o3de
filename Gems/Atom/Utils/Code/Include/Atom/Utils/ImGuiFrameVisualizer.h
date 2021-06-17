/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once
#include "Atom/RHI/FrameEventBus.h"
#include <AzCore/std/string/string.h>
namespace AZ
{
    namespace RHI
    {
        class FrameGraph;
        class Device;
    }
    namespace Render
    {
        class ImGuiFrameVisualizer:
            public RHI::FrameEventBus::Handler
        {
        public:
            struct ScopeAttachmentVisualizerInfo
            {
                AZ::RHI::ScopeId m_scopeId;
            };
            struct FrameAttachmentVisualizeInfo
            {
                AZStd::vector<ScopeAttachmentVisualizerInfo> m_pFirstScopeVisual;
                AZStd::vector<ScopeAttachmentVisualizerInfo> m_pLastScopeVisual;
            };
            ImGuiFrameVisualizer() = default;
            ~ImGuiFrameVisualizer() = default;
            AZStd::vector<FrameAttachmentVisualizeInfo>& GetFrameAttachments();
            void Init(RHI::Device* device);
            void Draw(bool& draw);
            void DrawTreeView();
            void Reset();
        protected:
            AZStd::vector<FrameAttachmentVisualizeInfo> m_framesAttachments;
            RHI::Device* m_device = nullptr;
            bool m_deviceInit = false;
            //////////////////////////////////////////////////////////////////////////
            // FrameEventBus::Handler
            void OnFrameCompileEnd(RHI::FrameGraph& frameGraph);
            //////////////////////////////////////////////////////////////////////////
        };
    }
}
#include "ImGuiFrameVisualizer.inl"
