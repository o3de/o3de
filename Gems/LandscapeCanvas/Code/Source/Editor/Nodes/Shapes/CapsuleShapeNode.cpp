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
#include "CapsuleShapeNode.h"

namespace LandscapeCanvas
{
    void CapsuleShapeNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<CapsuleShapeNode, BaseShapeNode>()
                ->Version(0)
                ;
        }
    }

    const QString CapsuleShapeNode::TITLE = QObject::tr("Capsule Shape");

    CapsuleShapeNode::CapsuleShapeNode(GraphModel::GraphPtr graph)
        : BaseShapeNode(graph)
    {
    }
} // namespace LandscapeCanvas
