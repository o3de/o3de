/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Data/Data.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/PureData.h>
#include <AzCore/std/typetraits/function_traits.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        template<typename t_Node, typename t_Datum>
        class NativeDatumNode
            : public PureData
        {
        public:
            using t_ThisType = NativeDatumNode<t_Node, t_Datum>;

            AZ_RTTI(((NativeDatumNode<t_Node, t_Datum>), "{B7D8D8D6-B2F1-481A-A712-B07D1C19555F}", t_Node, t_Datum), PureData, AZ::Component);
            AZ_COMPONENT_INTRUSIVE_DESCRIPTOR_TYPE(NativeDatumNode);
            AZ_COMPONENT_BASE(NativeDatumNode, PureData);

            static void Reflect(AZ::ReflectContext* reflection) 
            {
                if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                {
                    serializeContext->Class<t_ThisType, PureData>()
                        ->Version(0)
                        ;

                    if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                    {
                        editContext->Class<t_ThisType>("NativeDatumNode", "")
                            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ;
                    }
                }
            }

            ~NativeDatumNode() override = default;

        protected:
            virtual void ConfigureSetters()
            {
                Data::SetterContainer setterWrappers = Data::ExplodeToSetters(Data::FromAZType(Data::Traits<t_Datum>::GetAZType()));
                for (const auto& setterWrapperPair : setterWrappers)
                {
                    SlotId setterSlotId;
                    const Data::SetterWrapper& setterWrapper = setterWrapperPair.second;
                    const AZStd::string argName = AZStd::string::format("%s: %s", Data::GetName(setterWrapper.m_propertyType).data(), setterWrapper.m_propertyName.data());
                    AZStd::string_view argumentTooltip;
                    // Add the slot if it doesn't exist
                    setterSlotId = FindSlotIdForDescriptor(argName, SlotDescriptors::DataIn());

                    if (!setterSlotId.IsValid())
                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = argName;
                        slotConfiguration.m_toolTip = argumentTooltip;
                        slotConfiguration.SetType(setterWrapper.m_propertyType);
                        slotConfiguration.SetConnectionType(ConnectionType::Input);

                        setterSlotId = AddSlot(slotConfiguration);
                    }

                    if (setterSlotId.IsValid())
                    {
                        m_propertyAccount.m_getterSetterIdPairs[setterWrapperPair.first].second = setterSlotId;
                        m_propertyAccount.m_settersByInputSlot.emplace(setterSlotId, setterWrapperPair.second);
                    }
                }
            }

            virtual void ConfigureGetters()
            {
                Data::GetterContainer getterWrappers = Data::ExplodeToGetters(Data::FromAZType(Data::Traits<t_Datum>::GetAZType()));
                for (const auto& getterWrapperPair : getterWrappers)
                {
                    SlotId getterSlotId;

                    const Data::GetterWrapper& getterWrapper = getterWrapperPair.second;
                    const AZStd::string resultSlotName(AZStd::string::format("%s: %s", getterWrapper.m_propertyName.data(), Data::GetName(getterWrapper.m_propertyType).data()));
                    // Add the slot if it doesn't exist

                    getterSlotId = FindSlotIdForDescriptor(resultSlotName, SlotDescriptors::DataOut());

                    if (!getterSlotId.IsValid())
                    {
                        DataSlotConfiguration slotConfiguration;

                        slotConfiguration.m_name = resultSlotName;
                        slotConfiguration.SetType(getterWrapper.m_propertyType);
                        slotConfiguration.SetConnectionType(ConnectionType::Output);                        

                        getterSlotId = AddSlot(slotConfiguration);
                    }

                    if (getterSlotId.IsValid())
                    {
                        m_propertyAccount.m_getterSetterIdPairs[getterWrapperPair.first].first = getterSlotId;
                        m_propertyAccount.m_gettersByInputSlot.emplace(getterSlotId, getterWrapperPair.second);
                    }
                }
            }

            virtual void ConfigureProperties()
            {
                if (IsConfigured())
                {
                    return;
                }

                ConfigureGetters();
                ConfigureSetters();
                m_configured = true;
            }

            void OnInit() override
            {
                AddInputAndOutputTypeSlot(Data::FromAZType<t_Datum>());
                ConfigureProperties();
            }
        };
    }
}
