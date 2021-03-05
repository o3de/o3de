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