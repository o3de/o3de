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
#include "SpawnerAreaNode.h"

namespace LandscapeCanvas
{
    void SpawnerAreaNode::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<SpawnerAreaNode, BaseAreaNode>()
                ->Version(0)
                ;
        }
    }

    const QString SpawnerAreaNode::TITLE = QObject::tr("Vegetation Layer Spawner");

    SpawnerAreaNode::SpawnerAreaNode(GraphModel::GraphPtr graph)
        : BaseAreaNode(graph)
    {
        RegisterSlots();
        CreateSlotData();
    }
} // namespace LandscapeCanvas
