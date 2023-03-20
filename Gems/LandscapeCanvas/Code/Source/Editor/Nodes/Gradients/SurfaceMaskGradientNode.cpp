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
#include "SurfaceMaskGradientNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void SurfaceMaskGradientNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SurfaceMaskGradientNode, BaseGradientNode>()
                ->Version(0)
                ;
        }
    }

    const char* SurfaceMaskGradientNode::TITLE = "Surface Mask";

    SurfaceMaskGradientNode::SurfaceMaskGradientNode(GraphModel::GraphPtr graph)
        : BaseGradientNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }
} // namespace LandscapeCanvas
