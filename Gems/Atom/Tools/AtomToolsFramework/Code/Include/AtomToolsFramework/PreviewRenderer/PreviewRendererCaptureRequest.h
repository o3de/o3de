/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/PreviewRenderer/PreviewContent.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

class QPixmap;

namespace AtomToolsFramework
{
    //! PreviewRendererCaptureRequest describes the size, content, and behavior of a scene to be rendered to an image
    struct PreviewRendererCaptureRequest final
    {
        AZ_CLASS_ALLOCATOR(PreviewRendererCaptureRequest, AZ::SystemAllocator);

        int m_size = 512;
        AZStd::shared_ptr<PreviewContent> m_content;
        AZStd::function<void()> m_captureFailedCallback;
        AZStd::function<void(const QPixmap&)> m_captureCompleteCallback;
    };
} // namespace AtomToolsFramework
