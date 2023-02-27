/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GraphCanvas/Components/NodePropertyDisplay/VectorDataInterface.h>

#include "ScriptCanvasDataInterface.h"

namespace ScriptCanvasEditor
{
    template<class Type, int ElementCount>
    class ScriptCanvasVectorizedDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::VectorDataInterface>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasVectorizedDataInterface, AZ::SystemAllocator);
        ScriptCanvasVectorizedDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
        }
        
        ~ScriptCanvasVectorizedDataInterface() = default;
        
        int GetElementCount() const override
        {
            return ElementCount;
        }
        
        double GetValue(int index) const override
        {
            if (index < GetElementCount())
            {
                if (const ScriptCanvas::Datum* object = GetSlotObject())
                {
                    if (const Type* retVal = object->GetAs<Type>())
                    {
                        return aznumeric_cast<double>(static_cast<float>(retVal->GetElement(index)));
                    }
                }
            }
            
            return 0.0;
        }

        void SetValue(int index, double value) override
        {
            if (index < GetElementCount())
            {
                ScriptCanvas::ModifiableDatumView datumView;
                ModifySlotObject(datumView);

                if (datumView.IsValid())
                {
                    Type currentValue = (*datumView.GetAs<Type>());                    
                    currentValue.SetElement(index, aznumeric_cast<float>(value));

                    datumView.SetAs<Type>(currentValue);

                    PostUndoPoint();
                    PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
                }
            }
        }
    };

    template<class Type, int ElementCount>
    class ScriptCanvasVectorDataInterface
        : public ScriptCanvasVectorizedDataInterface<Type, ElementCount>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasVectorDataInterface, AZ::SystemAllocator);
        ScriptCanvasVectorDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasVectorizedDataInterface<Type, ElementCount>(nodeId, slotId)
        {
        }

        ~ScriptCanvasVectorDataInterface() = default;

        const char* GetLabel(int index) const override
        {
            if (index == 0)
            {
                return "X";
            }
            else if (index == 1)
            {
                return "Y";
            }
            else if (index == 2)
            {
                return "Z";
            }
            else if (index == 3)
            {
                return "W";
            }

            return "???";
        }

        AZStd::string GetStyle() const override
        {
            return "vectorized";
        }

        AZStd::string GetElementStyle(int index) const override
        {
            return AZStd::string::format("vector_%i", index);
        }
    };
}
