/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(CreateSplicingNodeMimeEvent, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);
        
        virtual AZ::EntityId CreateSplicingNode(const AZ::EntityId& graphId) = 0;
    };
}
