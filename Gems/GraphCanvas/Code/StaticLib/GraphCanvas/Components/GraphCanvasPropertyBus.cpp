/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphCanvas/Components/GraphCanvasPropertyBus.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace GraphCanvas
{
    void GraphCanvasPropertyComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphCanvasPropertyComponent, AZ::Component>()
                ->Version(1);
        }
    }
}
