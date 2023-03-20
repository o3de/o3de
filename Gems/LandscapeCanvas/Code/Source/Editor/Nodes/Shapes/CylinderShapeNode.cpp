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
#include "CylinderShapeNode.h"

namespace LandscapeCanvas
{
    void CylinderShapeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CylinderShapeNode, BaseShapeNode>()
                ->Version(0)
                ;
        }
    }

    const char* CylinderShapeNode::TITLE = "Cylinder Shape";

    CylinderShapeNode::CylinderShapeNode(GraphModel::GraphPtr graph)
        : BaseShapeNode(graph)
    {
    }
} // namespace LandscapeCanvas
