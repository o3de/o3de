/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <SceneAPI/SceneUI/RowWidgets/HeaderHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(HeaderHandler, SystemAllocator);

            HeaderHandler* HeaderHandler::s_instance = nullptr;

            QWidget* HeaderHandler::CreateGUI(QWidget* parent)
            {
                return aznew HeaderWidget(parent);
            }

            u32 HeaderHandler::GetHandlerName() const
            {
                return AZ_CRC_CE("Header");
            }

            bool HeaderHandler::AutoDelete() const
            {
                return false;
            }

            bool HeaderHandler::IsDefaultHandler() const
            {
                return true;
            }

            void HeaderHandler::ConsumeAttribute(HeaderWidget* /*widget*/, u32 /*attrib*/,
                AzToolsFramework::PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
            {
                // No attributes are used by this handler, but the function is mandatory.
            }

            void HeaderHandler::WriteGUIValuesIntoProperty(size_t /*index*/, HeaderWidget* GUI,
                property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
            {
                instance = *GUI->GetManifestObject();
            }

            bool HeaderHandler::ReadValuesIntoGUI(size_t /*index*/, HeaderWidget* GUI, const property_t& instance,
                AzToolsFramework::InstanceDataNode* /*node*/)
            {
                GUI->SetManifestObject(&instance);
                return false;
            }

            void HeaderHandler::Register()
            {
                if (!s_instance)
                {
                    s_instance = aznew HeaderHandler();
                    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, RegisterPropertyType, s_instance);
                }
            }

            void HeaderHandler::Unregister()
            {
                if (s_instance)
                {
                    EBUS_EVENT(AzToolsFramework::PropertyTypeRegistrationMessages::Bus, UnregisterPropertyType, s_instance);
                    delete s_instance;
                    s_instance = nullptr;
                }
            }

            bool HeaderHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
            {
                HeaderWidget* headerWidget = qobject_cast<HeaderWidget*>(widget);
                if (!headerWidget)
                {
                    return false;
                }

                return headerWidget->ModifyTooltip(toolTipString);
                
            }
        } // UI
    } // SceneAPI
} // AZ

#include <RowWidgets/moc_HeaderHandler.cpp>
