#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/OverlayWidgetLayer.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            // left in for backwards compatibility with original SceneAPI code
            class OverlayWidgetLayer : public AzQtComponents::OverlayWidgetLayer
            {
            public:
                AZ_CLASS_ALLOCATOR(OverlayWidgetLayer, SystemAllocator, 0)

                using AzQtComponents::OverlayWidgetLayer::OverlayWidgetLayer;
            };
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ
