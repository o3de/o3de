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
#include "SmoothStepGradientModifierNode.h"

namespace LandscapeCanvas
{
    void SmoothStepGradientModifierNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SmoothStepGradientModifierNode, BaseGradientModifierNode>()
                ->Version(0)
                ;
        }
    }

    const char* SmoothStepGradientModifierNode::TITLE = "Smooth-Step";

    SmoothStepGradientModifierNode::SmoothStepGradientModifierNode(GraphModel::GraphPtr graph)
        : BaseGradientModifierNode(graph)
    {
    }
} // namespace LandscapeCanvas
