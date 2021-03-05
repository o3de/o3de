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

#include <GraphCanvas/Components/NodePropertyDisplay/BooleanDataInterface.h>

#include "ScriptCanvasDataInterface.h"

namespace ScriptCanvasEditor
{
    class ScriptCanvasBoolDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::BooleanDataInterface>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasBoolDataInterface, AZ::SystemAllocator, 0);
        ScriptCanvasBoolDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }
        
        ~ScriptCanvasBoolDataInterface() = default;
        
        // BooleanDataInterface
        bool GetBool() const override
        {
            const ScriptCanvas::Datum* object = GetSlotObject();            

            if (object)
            {
                const bool* retVal = object->GetAs<bool>();

                if (retVal)
                {
                    return (*retVal);
                }
            }
            
            return false;
        }
        
        void SetBool(bool enabled) override
        {
            ScriptCanvas::ModifiableDatumView datumView;
            ModifySlotObject(datumView);

            if (datumView.IsValid())
            {
                datumView.SetAs(enabled);

                PostUndoPoint();
                PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
            }
        }
    };
}