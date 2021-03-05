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

#include <GraphCanvas/Components/NodePropertyDisplay/StringDataInterface.h>

#include "ScriptCanvasDataInterface.h"

namespace ScriptCanvasEditor
{
    class ScriptCanvasStringDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::StringDataInterface>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasStringDataInterface, AZ::SystemAllocator, 0);
        ScriptCanvasStringDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }
        
        ~ScriptCanvasStringDataInterface() = default;
        
        // StringDataInterface
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
        
        void SetString(const AZStd::string& value) override
        {
            ScriptCanvas::ModifiableDatumView datumView;
            ModifySlotObject(datumView);

            const AZStd::string* string = datumView.GetAs<AZStd::string>();

            if (string->compare(value) != 0)
            {
                datumView.SetAs<AZStd::string>(value);

                PostUndoPoint();
                PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
            }
        }
        ////
    };
}
