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