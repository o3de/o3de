/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>

namespace ScriptCanvas
{
    class Node;
}

namespace ScriptCanvasEditor
{
    struct NodeIdPair
    {
        AZ::EntityId m_graphCanvasId;
        AZ::EntityId m_scriptCanvasId;

        NodeIdPair()
            : m_graphCanvasId(AZ::EntityId::InvalidEntityId)
            , m_scriptCanvasId(AZ::EntityId::InvalidEntityId)
        {}
    };

    struct CreateNodeResult
    {
        NodeIdPair nodeIdPair;
        ScriptCanvas::Node* node = nullptr;
    };
}
