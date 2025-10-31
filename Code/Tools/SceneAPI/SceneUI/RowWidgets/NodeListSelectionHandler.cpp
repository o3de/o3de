/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneUI/RowWidgets/NodeListSelectionHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(NodeListSelectionHandler, SystemAllocator);

            NodeListSelectionHandler* NodeListSelectionHandler::s_instance = nullptr;

            QWidget* NodeListSelectionHandler::CreateGUI(QWidget* parent)
            {
                NodeListSelectionWidget* instance = aznew NodeListSelectionWidget(parent);
                connect(instance, &NodeListSelectionWidget::valueChanged, this, 
                    [instance]()
                    {
                        EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, instance);
                    });
                return instance;
            }

            u32 NodeListSelectionHandler::GetHandlerName() const
            {
                return AZ_CRC_CE("NodeListSelection");
            }

            bool NodeListSelectionHandler::AutoDelete() const
            {
                return false;
            }

            void NodeListSelectionHandler::ConsumeAttribute(NodeListSelectionWidget* widget, u32 attrib,
                AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
            {
                AZ_TraceContext("Attribute name", debugName);

                if (attrib == AZ_CRC_CE("DisabledOption"))
                {
                    ConsumeDisabledOptionAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC_CE("ClassTypeIdFilter"))
                {
                    ConsumeClassTypeIdAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC_CE("RequiresExactTypeId"))
                {
                    ConsumeRequiredExactTypeIdAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC_CE("UseShortNames"))
                {
                    ConsumeUseShortNameAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC_CE("ExcludeEndPoints"))
                {
                    ConsumeExcludeEndPointsAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC_CE("DefaultToDisabled"))
                {
                    ConsumeDefaultToDisabledAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC_CE("ComboBoxEditable"))
                {
                    bool editable;
                    if (attrValue->Read<bool>(editable))
                    {
                        widget->setEditable(editable);
                    }
                }
            }

            void NodeListSelectionHandler::WriteGUIValuesIntoProperty(size_t /*index*/, NodeListSelectionWidget* GUI,
                property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
            {
                instance = static_cast<property_t>(GUI->GetCurrentSelection());
            }

            bool NodeListSelectionHandler::ReadValuesIntoGUI(size_t /*index*/, NodeListSelectionWidget* GUI, const property_t& instance,
                AzToolsFramework::InstanceDataNode* /*node*/)
            {
                GUI->SetCurrentSelection(instance);
                return false;
            }

            void NodeListSelectionHandler::Register()
            {
                if (!s_instance)
                {
                    s_instance = aznew NodeListSelectionHandler();
                    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, s_instance);
                }
            }

            void NodeListSelectionHandler::Unregister()
            {
                if (s_instance)
                {
                    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, UnregisterPropertyType, s_instance);
                    delete s_instance;
                    s_instance = nullptr;
                }
            }

            void NodeListSelectionHandler::ConsumeDisabledOptionAttribute(NodeListSelectionWidget* widget, AzToolsFramework::PropertyAttributeReader* attrValue)
            {
                AZStd::string disabledOption;
                if (attrValue->Read<AZStd::string>(disabledOption))
                {
                    widget->AddDisabledOption(AZStd::move(disabledOption));
                }
                else
                {
                    AZ_Assert(false, "Failed to read string from 'DisabledOption' attribute.");
                }
            }

            void NodeListSelectionHandler::ConsumeClassTypeIdAttribute(NodeListSelectionWidget* widget, AzToolsFramework::PropertyAttributeReader* attrValue)
            {
                Uuid classTypeId;
                if (attrValue->Read<Uuid>(classTypeId))
                {
                    widget->SetClassTypeId(classTypeId);
                }
                else
                {
                    AZ_Assert(false, "Failed to read uuid from 'ClassTypeIdFilter' attribute.");
                }
            }

            void NodeListSelectionHandler::ConsumeRequiredExactTypeIdAttribute(NodeListSelectionWidget* widget, AzToolsFramework::PropertyAttributeReader* attrValue)
            {
                bool exactMatch;
                if (attrValue->Read<bool>(exactMatch))
                {
                    widget->UseExactClassTypeMatch(exactMatch);
                }
                else
                {
                    AZ_Assert(false, "Failed to read boolean from 'RequiresExactTypeId' attribute.");
                }
            }

            void NodeListSelectionHandler::ConsumeUseShortNameAttribute(NodeListSelectionWidget* widget, AzToolsFramework::PropertyAttributeReader* attrValue)
            {
                bool use;
                if (attrValue->Read<bool>(use))
                {
                    widget->UseShortNames(use);
                }
                else
                {
                    AZ_Assert(false, "Failed to read boolean from 'UseShortNames' attribute.");
                }
            }

            void NodeListSelectionHandler::ConsumeExcludeEndPointsAttribute(NodeListSelectionWidget* widget, AzToolsFramework::PropertyAttributeReader* attrValue)
            {
                bool exclude;
                if (attrValue->Read<bool>(exclude))
                {
                    widget->ExcludeEndPoints(exclude);
                }
                else
                {
                    AZ_Assert(false, "Failed to read boolean from 'ExcludeEndPoints' attribute.");
                }
            }

            void NodeListSelectionHandler::ConsumeDefaultToDisabledAttribute(NodeListSelectionWidget* widget, AzToolsFramework::PropertyAttributeReader* attrValue)
            {
                bool defaultToDisabled;
                if (attrValue->Read<bool>(defaultToDisabled))
                {
                    widget->DefaultToDisabled(defaultToDisabled);
                }
                else
                {
                    AZ_Assert(false, "Failed to read boolean from 'DefaultToDisabled' attribute.");
                }
            }
        } // UI
    } // SceneAPI
} // AZ

#include <RowWidgets/moc_NodeListSelectionHandler.cpp>
