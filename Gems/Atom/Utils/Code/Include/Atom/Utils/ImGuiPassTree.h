/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/AttachmentReadback.h>
#include <Atom/RPI.Public/Pass/Specific/ImageAttachmentPreviewPass.h>

#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace Render
    {
        class ImGuiPassTree
        {
        public:
            ImGuiPassTree() = default;
            ~ImGuiPassTree() = default;

            void Draw(bool& draw, AZ::RPI::Pass* rootPass);

            void Reset();

        private:
            void DrawTreeView(AZ::RPI::Pass* rootPass);

            void ReadbackCallback(const AZ::RPI::AttachmentReadback::ReadbackResult& readbackResult);

            void DrawPassAttachments(AZ::RPI::Pass* pass);

            bool m_previewAttachment = false;
            bool m_showAttachments = false;

            AZ::RPI::Pass* m_selectedPass = nullptr;
            AZ::RPI::Pass* m_lastSelectedPass = nullptr;
            AZ::Name m_selectedPassPath;
            AZ::RHI::AttachmentId m_attachmentId;
            AZ::Name m_slotName;
            bool m_selectedChanged = false;

            AZStd::shared_ptr<AZ::RPI::AttachmentReadback> m_readback;

            AZ::RPI::Ptr<AZ::RPI::ImageAttachmentPreviewPass> m_previewPass;

            AZStd::string m_engineRoot;
            
            AZStd::string m_attachmentReadbackInfo;
        };
    }
}

#include "ImGuiPassTree.inl"
