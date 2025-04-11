/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportContent.h>
#include <AtomToolsFramework/Viewport/ViewportInputBehaviorController/ViewportInputBehaviorController.h>
#endif

namespace AtomToolsFramework
{
    //! EntityPreviewViewportInputController is a convenience class that initializes a viewport input behavior controller with DCC like
    //! input controls for navigating the scene, focusing on, and orbiting around different objects
    class EntityPreviewViewportInputController final : public ViewportInputBehaviorController
    {
    public:
        AZ_CLASS_ALLOCATOR(EntityPreviewViewportInputController, AZ::SystemAllocator)
        EntityPreviewViewportInputController(
            const AZ::Crc32& toolId, QWidget* widget, AZStd::shared_ptr<EntityPreviewViewportContent> viewportContent);
        ~EntityPreviewViewportInputController() = default;

    private:
        const AZ::Crc32 m_toolId = {};
        AZStd::shared_ptr<EntityPreviewViewportContent> m_viewportContent;
    };
} // namespace AtomToolsFramework
