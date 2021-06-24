#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/OverlayWidget.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            // left in for backwards compatibility with original SceneAPI code
            using OverlayWidgetButton = AzQtComponents::OverlayWidgetButton;

            // left in for backwards compatibility with original SceneAPI code
            using OverlayWidgetButtonList = AzQtComponents::OverlayWidgetButtonList;
            
            // left in for backwards compatibility with original SceneAPI code
            class OverlayWidget : public AzQtComponents::OverlayWidget
            {
            public:
                AZ_CLASS_ALLOCATOR(OverlayWidget, SystemAllocator, 0)

                using AzQtComponents::OverlayWidget::OverlayWidget;
            };
        } // namespace SceneUI
    } // namespace SceneAPI
} // namespace AZ
