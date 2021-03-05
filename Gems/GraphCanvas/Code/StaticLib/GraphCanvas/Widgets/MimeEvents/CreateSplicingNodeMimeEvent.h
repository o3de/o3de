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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>

#include <GraphCanvas/Widgets/GraphCanvasMimeEvent.h>

namespace GraphCanvas
{
    class CreateSplicingNodeMimeEvent
        : public GraphCanvasMimeEvent
    {
    public:
        AZ_RTTI(CreateSplicingNodeMimeEvent, "{5191EFF0-BD91-48BF-8A95-9471B8E671A4}", GraphCanvasMimeEvent);
        AZ_CLASS_ALLOCATOR(CreateSplicingNodeMimeEvent, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);
        
        virtual AZ::EntityId CreateSplicingNode(const AZ::EntityId& graphId) = 0;
    };
}