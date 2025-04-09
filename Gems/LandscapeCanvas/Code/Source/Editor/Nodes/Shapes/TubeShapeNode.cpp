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
#include "TubeShapeNode.h"

namespace LandscapeCanvas
{
    void TubeShapeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TubeShapeNode, BaseShapeNode>()
                ->Version(0)
                ;
        }
    }

    const char* TubeShapeNode::TITLE = "Tube Shape";

    TubeShapeNode::TubeShapeNode(GraphModel::GraphPtr graph)
        : BaseShapeNode(graph)
    {
    }
} // namespace LandscapeCanvas
