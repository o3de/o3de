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
#include "DitherGradientModifierNode.h"

namespace LandscapeCanvas
{
    void DitherGradientModifierNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DitherGradientModifierNode, BaseGradientModifierNode>()
                ->Version(0)
                ;
        }
    }

    const char* DitherGradientModifierNode::TITLE = "Dither";

    DitherGradientModifierNode::DitherGradientModifierNode(GraphModel::GraphPtr graph)
        : BaseGradientModifierNode(graph)
    {
    }
} // namespace LandscapeCanvas
