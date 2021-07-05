/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/Components/LayerControllerComponent.h>

namespace GraphCanvas
{
    class NodeLayerControllerComponent
        : public LayerControllerComponent
    {
    public:
        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

            if (serializeContext)
            {
                serializeContext->Class<NodeLayerControllerComponent, LayerControllerComponent>()
                    ->Version(0)
                    ;
            }
        }

        AZ_COMPONENT(NodeLayerControllerComponent, "{ECD262DC-D350-44B4-832E-67FCA88D8D3E}", LayerControllerComponent);        
        
        NodeLayerControllerComponent()
            : LayerControllerComponent("NodeLayer", NodeOffset)
        {
        }
    };
}
