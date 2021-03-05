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
            : LayerControllerComponent("BookmarkAnchor")
        {

        }
    };
}