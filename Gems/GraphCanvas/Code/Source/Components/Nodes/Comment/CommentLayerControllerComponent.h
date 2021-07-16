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
    class CommentLayerControllerComponent
        : public LayerControllerComponent
    {
    public:
        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

            if (serializeContext)
            {
                serializeContext->Class<CommentLayerControllerComponent, LayerControllerComponent>()
                    ->Version(0)
                    ;
            }
        }

        AZ_COMPONENT(CommentLayerControllerComponent, "{E6E6A329-40DA-4F95-B42E-6843DF2B6E2E}", LayerControllerComponent);

        CommentLayerControllerComponent()
            : LayerControllerComponent("CommentLayer", CommentOffset)
        {

        }
    };
}
