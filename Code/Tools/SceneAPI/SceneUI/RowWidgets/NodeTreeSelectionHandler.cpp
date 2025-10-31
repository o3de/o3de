/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneUI/RowWidgets/NodeTreeSelectionHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(NodeTreeSelectionHandler, SystemAllocator);

            NodeTreeSelectionHandler* NodeTreeSelectionHandler::s_instance = nullptr;

            QWidget* NodeTreeSelectionHandler::CreateGUI(QWidget* parent)
            {
                NodeTreeSelectionWidget* instance = aznew NodeTreeSelectionWidget(parent);
                connect(instance, &NodeTreeSelectionWidget::valueChanged, this,
                    [instance]()
                    {
                        EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, instance);
                    });
                return instance;
            }

            u32 NodeTreeSelectionHandler::GetHandlerName() const
            {
                return AZ_CRC_CE("NodeTreeSelection");
            }

            bool NodeTreeSelectionHandler::AutoDelete() const
            {
                return false;
            }

            bool NodeTreeSelectionHandler::IsDefaultHandler() const
            {
                return true;
            }

            void NodeTreeSelectionHandler::ConsumeAttribute(NodeTreeSelectionWidget* widget, u32 attrib,
                AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
            {
                AZ_TraceContext("Attribute name", debugName);

                if (attrib == AZ_CRC_CE("FilterName"))
                {
                    ConsumeFilterNameAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC_CE("FilterType"))
                {
                    ConsumeFilterTypeAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC_CE("FilterVirtualType"))
                {
                    ConsumeFilterVirtualTypeAttribute(widget, attrValue);
                }
                else if (attrib == AZ_CRC_CE("NarrowSelection"))
                {
                    ConsumeNarrowSelectionAttribute(widget, attrValue);
                }
            }

            void NodeTreeSelectionHandler::WriteGUIValuesIntoProperty(size_t /*index*/, NodeTreeSelectionWidget* GUI,
                property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
            {
                GUI->CopyListTo(instance);
                GUI->UpdateSelectionLabel();
            }

            bool NodeTreeSelectionHandler::ReadValuesIntoGUI(size_t /*index*/, NodeTreeSelectionWidget* GUI, const property_t& instance,
                AzToolsFramework::InstanceDataNode* /*node*/)
            {
                GUI->SetList(instance);
                GUI->UpdateSelectionLabel();
                return false;
            }

            void NodeTreeSelectionHandler::Register()
            {
                if (!s_instance)
                {
                    s_instance = aznew NodeTreeSelectionHandler();
                    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, s_instance);
                }
            }

            void NodeTreeSelectionHandler::Unregister()
            {
                if (s_instance)
                {
                    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, UnregisterPropertyType, s_instance);
                    delete s_instance;
                    s_instance = nullptr;
                }
            }

            void NodeTreeSelectionHandler::ConsumeFilterNameAttribute(NodeTreeSelectionWidget* widget, AzToolsFramework::PropertyAttributeReader* attrValue)
            {
                AZStd::string filterName;
                if (attrValue->Read<AZStd::string>(filterName))
                {
                    widget->SetFilterName(AZStd::move(filterName));
                }
                else
                {
                    AZ_Assert(false, "Failed to read string from 'FilterName' attribute.");
                }
            }

            void NodeTreeSelectionHandler::ConsumeFilterTypeAttribute(NodeTreeSelectionWidget* widget, AzToolsFramework::PropertyAttributeReader* attrValue)
            {
                Uuid filterType;
                if (attrValue->Read<Uuid>(filterType))
                {
                    widget->AddFilterType(filterType);
                }
                else
                {
                    AZ_Assert(false, "Failed to read Uuid from 'FilterType' attribute.");
                }
            }

            void NodeTreeSelectionHandler::ConsumeFilterVirtualTypeAttribute(NodeTreeSelectionWidget* widget, AzToolsFramework::PropertyAttributeReader* attrValue)
            {
                Crc32 filterVirtualType;
                if (!attrValue->Read<Crc32>(filterVirtualType))
                {
                    AZStd::string filterFilterTypeName;
                    if (attrValue->Read<AZStd::string>(filterFilterTypeName))
                    {
                        filterVirtualType = Crc32(filterFilterTypeName.c_str());
                    }
                    else
                    {
                        AZ_Assert(false, "Failed to read crc value or string from 'VirtualFilterName' attribute.");
                        return;
                    }
                }
                widget->AddFilterVirtualType(filterVirtualType);
            }

            void NodeTreeSelectionHandler::ConsumeNarrowSelectionAttribute(NodeTreeSelectionWidget* widget, AzToolsFramework::PropertyAttributeReader* attrValue)
            {
                bool narrowSelection;
                if (attrValue->Read<bool>(narrowSelection))
                {
                    widget->UseNarrowSelection(narrowSelection);
                }
                else
                {
                    AZ_Assert(false, "Failed to read boolean from 'NarrowSelection' attribute.");
                }
            }
        } // UI
    } // SceneAPI
} // AZ

#include <RowWidgets/moc_NodeTreeSelectionHandler.cpp>
