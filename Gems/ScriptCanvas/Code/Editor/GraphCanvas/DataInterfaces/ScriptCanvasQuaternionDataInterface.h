/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Quaternion.h>

#include <GraphCanvas/Components/NodePropertyDisplay/VectorDataInterface.h>

#include "ScriptCanvasDataInterface.h"

namespace ScriptCanvasEditor
{
    class ScriptCanvasQuaternionDataInterface
        : public ScriptCanvasDataInterface<GraphCanvas::VectorDataInterface>
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasQuaternionDataInterface, AZ::SystemAllocator);
        ScriptCanvasQuaternionDataInterface(const AZ::EntityId& nodeId, const ScriptCanvas::SlotId& slotId)
            : ScriptCanvasDataInterface(nodeId, slotId)
        {
            ConvertToEulerValues();
        }
        
        ~ScriptCanvasQuaternionDataInterface() = default;
        
        int GetElementCount() const override
        {
            return 3;
        }
        
        double GetValue(int index) const override
        {
            if (index < GetElementCount())
            {
                const ScriptCanvas::Datum* object = GetSlotObject();

                if (object)
                {
                    const AZ::Quaternion* retVal = object->GetAs<AZ::Quaternion>();

                    if (retVal)
                    {
                        return aznumeric_cast<double>(static_cast<float>(m_eulerAngles.GetElement(index)));
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
                    m_eulerAngles.SetElement(index, aznumeric_cast<float>(value));

                    AZ::Quaternion newValue = AZ::ConvertEulerDegreesToQuaternion(m_eulerAngles);

                    datumView.SetAs(newValue);
                    
                    PostUndoPoint();
                    PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
                }
            }
        }        
        
        const char* GetLabel(int index) const override
        {
            switch (index)
            {
            case 0:
                return "P";
            case 1:
                return "R";
            case 2:
                return "Y";
            default:
                return "???";
            }
        }
        
        AZStd::string GetStyle() const override
        {
            return "vectorized";
        }
        
        AZStd::string GetElementStyle(int index) const override
        {
            return AZStd::string::format("quat_%i", index);
        }
        
        const char* GetSuffix([[maybe_unused]] int index) const override
        {
            return " deg";
        }

        void OnSlotInputChanged(const ScriptCanvas::SlotId& slotId) override
        {
            if (slotId == GetSlotId())
            {
                ConvertToEulerValues();
            }

            ScriptCanvasDataInterface::OnSlotInputChanged(slotId);
        }

    private:
        void ConvertToEulerValues()
        {
            const ScriptCanvas::Datum* object = GetSlotObject();

            if (object)
            {
                const AZ::Quaternion* quat = object->GetAs<AZ::Quaternion>();
            
                if (quat)
                {
                    m_eulerAngles = AZ::ConvertTransformToEulerDegrees(AZ::Transform::CreateFromQuaternion((*quat)));
                }
            }
        }

        AZ::Vector3 m_eulerAngles;
    };
}
