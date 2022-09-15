/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Qt
#include <QObject>

// AZ
#include <AzCore/Serialization/SerializeContext.h>

// Landscape Canvas
#include "DistanceBetweenFilterNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void DistanceBetweenFilterNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DistanceBetweenFilterNode, BaseAreaFilterNode>()
                ->Version(0)
                ;
        }
    }

    const char* DistanceBetweenFilterNode::TITLE = "Distance Between Filter";

    DistanceBetweenFilterNode::DistanceBetweenFilterNode(GraphModel::GraphPtr graph)
        : BaseAreaFilterNode(graph)
    {
        CreateSlotData();
    }
} // namespace LandscapeCanvas
