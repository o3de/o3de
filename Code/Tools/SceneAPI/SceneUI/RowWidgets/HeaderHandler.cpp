/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneUI/RowWidgets/HeaderHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(HeaderHandler, SystemAllocator, 0)

            HeaderHandler* HeaderHandler::s_instance = nullptr;

            QWidget* HeaderHandler::CreateGUI(QWidget* parent)
            {
                return aznew HeaderWidget(parent);
            }

            u32 HeaderHandler::GetHandlerName() const
            {
                return AZ_CRC("Header", 0x6e72a8c1);
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
        } // UI
    } // SceneAPI
} // AZ

#include <RowWidgets/moc_HeaderHandler.cpp>