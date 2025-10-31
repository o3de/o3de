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

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

namespace ScriptCanvasEditor
{
    class ScriptCanvasCRCDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::StringDataInterface>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasCRCDataInterface, AZ::SystemAllocator);
        ScriptCanvasCRCDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }

        ~ScriptCanvasCRCDataInterface() = default;

        // StringDataInterface
        AZStd::string GetString() const override
        {
            AZStd::string retVal;

            const ScriptCanvas::Datum* object = GetSlotObject();

            if (object && object->IS_A<AZ::Crc32>())
            {
                AZ::Crc32 crcValue = (*object->GetAs<AZ::Crc32>());
                EditorGraphRequestBus::EventResult(retVal, GetScriptCanvasId(), &EditorGraphRequests::DecodeCrc, crcValue);

                if (retVal.empty() && crcValue != AZ::Crc32())
                {
                    AZ_Warning("ScriptCanvas", false, "Unknown CRC value. Cannot display cached string.");
                    retVal = AZStd::string::format("0x%X", static_cast<AZ::u32>(crcValue));
                }
            }

            return retVal;
        }
        
        void SetString(const AZStd::string& value) override
        {
            ScriptCanvas::ModifiableDatumView datumView;            
            ModifySlotObject(datumView);

            if (datumView.IsValid())
            {
                AZ::Crc32 newCrc = AZ::Crc32(value.c_str());
                AZ::Crc32 oldCrc = (*datumView.GetAs<AZ::Crc32>());

                if (oldCrc != newCrc)
                {
                    EditorGraphRequestBus::Event(GetScriptCanvasId(), &EditorGraphRequests::RemoveCrcCache, oldCrc);
                    EditorGraphRequestBus::Event(GetScriptCanvasId(), &EditorGraphRequests::AddCrcCache, newCrc, value);

                    datumView.SetAs<ScriptCanvas::Data::CRCType>(newCrc);

                    PostUndoPoint();
                    PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
                }
            }
        }
        ////
    };
}
