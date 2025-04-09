/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(ScriptCanvasStringDataInterface, AZ::SystemAllocator);
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
