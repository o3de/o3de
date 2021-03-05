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
            : LayerControllerComponent("CommentLayer")
        {

        }
    };
}