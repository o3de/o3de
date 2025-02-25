/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "VertexContainerDisplay.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace AzToolsFramework
{
    namespace VertexContainerDisplay
    {
        const float DefaultVertexTextSize = 1.5f;
        const AZ::Color DefaultVertexTextColor = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
        const AZ::Vector3 DefaultVertexTextOffset = AZ::Vector3(0.0f, 0.0f, -0.1f);
    } // namespace VertexContainerDisplay
} // namespace AzToolsFramework
