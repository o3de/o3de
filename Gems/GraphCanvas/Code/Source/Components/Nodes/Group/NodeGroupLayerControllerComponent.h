/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/Components/LayerControllerComponent.h>

namespace GraphCanvas
{
    class NodeGroupLayerControllerComponent
        : public LayerControllerComponent
    {
    public:
        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

            if (serializeContext)
            {
                serializeContext->Class<NodeGroupLayerControllerComponent, LayerControllerComponent>()
                    ->Version(0)
                    ;
            }
        }

        AZ_COMPONENT(NodeGroupLayerControllerComponent, "{7EAA5B66-D918-4284-B25C-E0811809CA05}", LayerControllerComponent);

        NodeGroupLayerControllerComponent()
            : LayerControllerComponent("NodeGroupLayer", NodeGroupOffset)
        {

        }
    };
}
