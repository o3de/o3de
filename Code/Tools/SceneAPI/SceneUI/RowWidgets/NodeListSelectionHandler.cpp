/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            AZ_CLASS_ALLOCATOR_IMPL(NodeListSelectionHandler, SystemAllocator, 0)

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
                return AZ_CRC("NodeListSelection", 0x45c54909);
            }

            bool NodeListSelectionHandler::AutoDelete() const
            {
                return false;
            }

            void NodeListSelectionHandler::ConsumeAttribute(NodeListSelectionWidget* widget, u32 attrib,
                AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
            {
                AZ_TraceContext("Attribute name", debugName);

                if (attrib == AZ_CRC("DisabledOption", 0x6cd17278))
                {
                    ConsumeDisabledOptionAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC("ClassTypeIdFilter", 0x21c301f1))
                {
                    ConsumeClassTypeIdAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC("RequiresExactTypeId", 0x9adf9b0d))
                {
                    ConsumeRequiredExactTypeIdAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC("UseShortNames", 0xf6a37fd3))
                {
                    ConsumeUseShortNameAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC("ExcludeEndPoints", 0x53bd29cc))
                {
                    ConsumeExcludeEndPointsAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC("DefaultToDisabled", 0xa2d03bd1))
                {
                    ConsumeDefaultToDisabledAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC("ComboBoxEditable", 0x7ee76669))
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
