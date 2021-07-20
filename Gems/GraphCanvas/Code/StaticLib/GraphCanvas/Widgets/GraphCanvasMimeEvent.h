/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/RTTI/RTTI.h>

#include <GraphCanvas/Types/Types.h>
#include <GraphCanvas/Editor/EditorTypes.h>

namespace GraphCanvas
{
    class GraphCanvasMimeEvent
    {
    public:
        AZ_RTTI(GraphCanvasMimeEvent, "{89AA505F-D6E7-425F-B5C0-A6599FAD71EE}");        
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        GraphCanvasMimeEvent() = default;
        virtual ~GraphCanvasMimeEvent() = default;
        
        virtual bool CanGraphHandleEvent(const GraphId& graphId) const
        {
            AZ_UNUSED(graphId);
            return true;
        }

        virtual bool ExecuteEvent(const AZ::Vector2& sceneMousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& sceneId) = 0;

        const NodeId& GetCreatedNodeId() const;
        
    protected:
        NodeId m_createdNodeId;
    };
}
