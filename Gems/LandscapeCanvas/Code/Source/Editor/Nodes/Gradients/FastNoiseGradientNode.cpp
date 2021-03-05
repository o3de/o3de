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

// Qt
#include <QObject>

// AZ
#include <AzCore/Serialization/SerializeContext.h>

// Landscape Canvas
#include "FastNoiseGradientNode.h"
#include <Editor/Core/GraphContext.h>

namespace LandscapeCanvas
{
    void FastNoiseGradientNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<FastNoiseGradientNode, BaseGradientNode>()
                ->Version(0)
                ;
        }
    }

    const QString FastNoiseGradientNode::TITLE = QObject::tr("FastNoise");

    FastNoiseGradientNode::FastNoiseGradientNode(GraphModel::GraphPtr graph)
        : BaseGradientNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }
} // namespace LandscapeCanvas
