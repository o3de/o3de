/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyVectorCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyDoubleSpinCtrl.hxx>
#include <SceneAPI/SceneUI/RowWidgets/TransformRowHandler.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            AZ_CLASS_ALLOCATOR_IMPL(TranformRowHandler, SystemAllocator);

            TranformRowHandler* TranformRowHandler::s_instance = nullptr;

            QWidget* TranformRowHandler::CreateGUI(QWidget* parent)
            {
                return aznew TransformRowWidget(parent);
            }

            u32 TranformRowHandler::GetHandlerName() const
            {
                return AZ_CRC_CE("TranformRow");
            }

            bool TranformRowHandler::AutoDelete() const
            {
                return false;
            }

            bool TranformRowHandler::IsDefaultHandler() const
            {
                return true;
            }

            void TranformRowHandler::ConsumeAttribute(TransformRowWidget* widget, u32 attrib,
                AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
            {
                if (attrib == AZ::Edit::Attributes::ReadOnly)
                {
                    bool value;
                    if (attrValue->Read<bool>(value))
                    {
                        widget->SetEnableEdit(!value);
                    }
                }
                else
                {
                    AzToolsFramework::Vector3PropertyHandler vector3Handler;
                    vector3Handler.ConsumeAttribute(widget->GetTranslationWidget(), attrib, attrValue, debugName);
                    vector3Handler.ConsumeAttribute(widget->GetRotationWidget(), attrib, attrValue, debugName);
                    AzToolsFramework::doublePropertySpinboxHandler spinboxHandler;
                    spinboxHandler.ConsumeAttribute(widget->GetScaleWidget(), attrib, attrValue, debugName);
                }
            }

            void TranformRowHandler::WriteGUIValuesIntoProperty(size_t /*index*/, TransformRowWidget* GUI,
                property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
            {
                GUI->GetTransform(instance);
            }

            bool TranformRowHandler::ReadValuesIntoGUI(size_t /*index*/, TransformRowWidget* GUI, const property_t& instance,
                AzToolsFramework::InstanceDataNode* /*node*/)
            {
                GUI->SetTransform(instance);
                return false;
            }

            void TranformRowHandler::Register()
            {
                using namespace AzToolsFramework;

                if (!s_instance)
                {
                    s_instance = aznew TranformRowHandler();
                    PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, s_instance);
                }
            }

            void TranformRowHandler::Unregister()
            {
                using namespace AzToolsFramework;

                if (s_instance)
                {
                    PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::UnregisterPropertyType, s_instance);
                    delete s_instance;
                    s_instance = nullptr;
                }
            }

            void TranformRowHandler::ConsumeFilterTypeAttribute(TransformRowWidget* /*widget*/,
                AzToolsFramework::PropertyAttributeReader* /*attrValue*/)
            {
            }
        } // namespace SceneUI
    } // namespace SceneAPI
} // namespace AZ

#include <RowWidgets/moc_TransformRowHandler.cpp>
