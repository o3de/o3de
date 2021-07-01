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
    class BookmarkAnchorLayerControllerComponent
        : public LayerControllerComponent
    {
    public:
        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

            if (serializeContext)
            {
                serializeContext->Class<BookmarkAnchorLayerControllerComponent, LayerControllerComponent>()
                    ->Version(0)
                    ;
            }
        }

        AZ_COMPONENT(BookmarkAnchorLayerControllerComponent, "{AC75ED23-1F88-484A-A7FE-35679A23329B}", LayerControllerComponent);

        BookmarkAnchorLayerControllerComponent()
            : LayerControllerComponent("BookmarkAnchor", BookmarkAnchorOffset)
        {

        }
    };
}
