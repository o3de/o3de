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
#include "PolygonPrismShapeNode.h"

namespace LandscapeCanvas
{
    void PolygonPrismShapeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PolygonPrismShapeNode, BaseShapeNode>()
                ->Version(0)
                ;
        }
    }

    const char* PolygonPrismShapeNode::TITLE = "Polygon Prism Shape";

    PolygonPrismShapeNode::PolygonPrismShapeNode(GraphModel::GraphPtr graph)
        : BaseShapeNode(graph)
    {
    }
} // namespace LandscapeCanvas
