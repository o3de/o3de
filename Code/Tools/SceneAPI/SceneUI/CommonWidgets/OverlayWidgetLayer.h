#pragma once

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
