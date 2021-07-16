/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/NodePropertyDisplay/ReadOnlyDataInterface.h>

#include "ScriptCanvasDataInterface.h"

namespace ScriptCanvasEditor
{
    class ScriptCanvasReadOnlyDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::ReadOnlyDataInterface>        
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasReadOnlyDataInterface, AZ::SystemAllocator, 0);
        ScriptCanvasReadOnlyDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }
        
        ~ScriptCanvasReadOnlyDataInterface() = default;
        
        // ReadOnlyDataInterface
        AZStd::string GetString() const override
        {
            AZStd::string retVal;
            
            const ScriptCanvas::Datum* object = GetSlotObject();

            if (object)
            {
                object->ToString(retVal);
            }
            
            return retVal;
        }
        ////
    };
}
