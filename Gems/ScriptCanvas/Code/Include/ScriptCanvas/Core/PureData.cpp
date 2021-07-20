/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PureData.h"

namespace ScriptCanvas
{
    const char* PureData::k_getThis("Get");
    const char* PureData::k_setThis("Set");

    PureData::~PureData()
    {

    }
    
    const AZStd::unordered_map<AZStd::string, AZStd::pair<SlotId, SlotId>>& PureData::GetPropertyNameSlotMap() const
    {
        return m_propertyAccount.m_getterSetterIdPairs;
    }

    void PureData::AddInputAndOutputTypeSlot(const Data::Type& type, const void* source)
    {
        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = k_setThis;
            slotConfiguration.SetConnectionType(ConnectionType::Input);
            slotConfiguration.ConfigureDatum(AZStd::move(Datum(type, Datum::eOriginality::Original, source, AZ::Uuid::CreateNull())));

            AddSlot(slotConfiguration);
        }

        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = k_getThis;
            slotConfiguration.SetConnectionType(ConnectionType::Output);            
            slotConfiguration.SetType(type);

            AddSlot(slotConfiguration);
        }
    }

    void PureData::AddInputTypeAndOutputTypeSlot(const Data::Type& type)
    {
        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = k_setThis;
            slotConfiguration.SetConnectionType(ConnectionType::Input);
            slotConfiguration.SetType(type);

            AddSlot(slotConfiguration);
        }

        {
            DataSlotConfiguration slotConfiguration;

            slotConfiguration.m_name = k_getThis;
            slotConfiguration.SetConnectionType(ConnectionType::Output);
            slotConfiguration.SetType(type);

            AddSlot(slotConfiguration);
        }
    }

    void PureData::OnActivate()
    {
        PushThis();

        for (const auto& propertySlotIdsPair : m_propertyAccount.m_getterSetterIdPairs)
        {
            const SlotId& getterSlotId = propertySlotIdsPair.second.first;
            CallGetter(getterSlotId);
        }
    }

    void PureData::OnInputChanged(const Datum& input, [[maybe_unused]] const SlotId& id) 
    {
        if (IsActivated())
        {
            OnOutputChanged(input);
        }
    }

    AZStd::string_view PureData::GetInputDataName() const
    {
        return k_setThis;
    }

    AZStd::string_view PureData::GetOutputDataName() const
    {
        return k_getThis;
    }

    void PureData::CallGetter(const SlotId& getterSlotId)
    {
        auto getterFuncIt = m_propertyAccount.m_gettersByInputSlot.find(getterSlotId);
        Slot* getterSlot = GetSlot(getterSlotId);
        if (getterSlot && getterFuncIt != m_propertyAccount.m_gettersByInputSlot.end())
        {
            AZStd::vector<AZStd::pair<Node*, const SlotId>> outputNodes(ModConnectedNodes(*getterSlot));

            if (!outputNodes.empty())
            {
                auto getterOutcome = getterFuncIt->second.m_getterFunction((*FindDatum(GetSlotId(k_setThis))));
                if (!getterOutcome)
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), getterOutcome.GetError().data());
                    return;
                }


                for (auto& nodePtrSlot : outputNodes)
                {
                    if (nodePtrSlot.first)
                    {
                        Node::SetInput(*nodePtrSlot.first, nodePtrSlot.second, getterOutcome.GetValue());
                    }
                }
            }
        }
    }

    void PureData::SetInput(const Datum& input, const SlotId& id)
    {
        if (id == FindSlotIdForDescriptor(GetInputDataName(), SlotDescriptors::DataIn()))
        {
            // push this value, as usual
            Node::SetInput(input, id);

            // now, call every getter, as every property has (presumably) been changed
            for (const auto& propertyNameSlotIdsPair : m_propertyAccount.m_getterSetterIdPairs)
            {
                const SlotId& getterSlotId = propertyNameSlotIdsPair.second.first;
                CallGetter(getterSlotId);
            }
        }
        else
        {
            SetProperty(input, id);
        }
    }

    void PureData::SetInput(Datum&& input, const SlotId& id)
    {
        if (id == FindSlotIdForDescriptor(GetInputDataName(), SlotDescriptors::DataIn()))
        {
            // push this value, as usual
            Node::SetInput(AZStd::move(input), id);

            if (IsActivated())
            {
                // now, call every getter, as every property has (presumably) been changed
                for (const auto& propertyNameSlotIdsPair : m_propertyAccount.m_getterSetterIdPairs)
                {
                    const SlotId& getterSlotId = propertyNameSlotIdsPair.second.first;
                    CallGetter(getterSlotId);
                }
            }
        }
        else
        {
            SetProperty(AZStd::move(input), id);
        }
    }

    void PureData::SetProperty(const Datum& input, const SlotId& setterId)
    {
        auto methodBySlotIter = m_propertyAccount.m_settersByInputSlot.find(setterId);
        if (methodBySlotIter == m_propertyAccount.m_settersByInputSlot.end())
        {
            AZ_Error("Script Canvas", false, "BehaviorContextObject SlotId %s did not route to a setter", setterId.m_id.ToString<AZStd::string>().data());
            return;
        }
        if (!methodBySlotIter->second.m_setterFunction)
        {
            AZ_Error("Script Canvas", false, "BehaviorContextObject setter is not invocable for SlotId %s is nullptr", setterId.m_id.ToString<AZStd::string>().data());
            return;
        }

        ModifiableDatumView datumView;
        FindModifiableDatumView(GetSlotId(k_setThis), datumView);

        Datum* datum = datumView.ModifyDatum();

        auto setterOutcome = methodBySlotIter->second.m_setterFunction((*datum), input);

        if (!setterOutcome)
        {
            SCRIPTCANVAS_REPORT_ERROR((*this), setterOutcome.TakeError().data());
            return;
        }

        datumView.SignalModification();

        PushThis();

        auto getterSetterIt = m_propertyAccount.m_getterSetterIdPairs.find(methodBySlotIter->second.m_propertyName);

        if (getterSetterIt != m_propertyAccount.m_getterSetterIdPairs.end())
        {
            CallGetter(getterSetterIt->second.first);
        }
    }


    void PureData::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);
        if (serializeContext)
        {
            serializeContext->Class<PureData, Node>()
                ->Version(0)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PureData>("PureData", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ;
            }
        }
    }
}
