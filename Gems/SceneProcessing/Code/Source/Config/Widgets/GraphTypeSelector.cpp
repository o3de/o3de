/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QMenu>
#include <QEvent>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <Config/Widgets/GraphTypeSelector.h>

namespace AZ
{
    namespace SceneProcessingConfig
    {
        AZ_CLASS_ALLOCATOR_IMPL(GraphTypeSelector, SystemAllocator);

        GraphTypeSelector* GraphTypeSelector::s_instance = nullptr;

        QWidget* GraphTypeSelector::CreateGUI(QWidget* parent)
        {
            QPushButton* base = new QPushButton("Select required graph types", parent);
            QMenu* menu = new QMenu(base);
            menu->setLayoutDirection(Qt::LeftToRight);
            menu->setStyleSheet("border: none; background-color: #333333;");

            SerializeContext* context = nullptr;
            ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(context, "Unable to find valid serialize context.");

            context->EnumerateDerived<SceneAPI::DataTypes::IGraphObject>(
                [menu](const SerializeContext::ClassData* data, const Uuid& typeId) -> bool
                {
                    AZ_UNUSED(typeId);
                    QAction* action = menu->addAction(data->m_name);
                    action->setCheckable(true);
                    return true;
                });

            base->setMenu(menu);
            base->installEventFilter(this);

            return base;
        }

        bool GraphTypeSelector::eventFilter(QObject* object, QEvent* event)
        {
            // Using FocusIn instead of FocusOut because after pressing the button the menu gets focus but after
            //      a selection is made the focus goes back to the button, so at that point saving needs to happen.
            if (event->type() == QEvent::FocusIn)
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, qobject_cast<QPushButton*>(object));
            }
            else if (event->type() == QEvent::Show)
            {
                QPushButton* button = qobject_cast<QPushButton*>(object);
                button->menu()->setFixedWidth(button->width());
            }
            else if (event->type() == QEvent::Resize)
            {
                QPushButton* button = qobject_cast<QPushButton*>(object);
                button->menu()->setFixedWidth(button->width());
            }

            return QObject::eventFilter(object, event);
        }

        u32 GraphTypeSelector::GetHandlerName() const
        {
            return AZ_CRC_CE("GraphTypeSelector");
        }

        bool GraphTypeSelector::AutoDelete() const
        {
            return false;
        }

        bool GraphTypeSelector::IsDefaultHandler() const
        {
            return false;
        }

        void GraphTypeSelector::ConsumeAttribute(QPushButton* widget, u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
        {
            AZ_UNUSED(widget);
            AZ_UNUSED(attrib);
            AZ_UNUSED(attrValue);
            AZ_UNUSED(debugName);
        }

        void GraphTypeSelector::WriteGUIValuesIntoProperty(size_t index, QPushButton* GUI,
            property_t& instance, AzToolsFramework::InstanceDataNode* node)
        {
            AZ_UNUSED(index);
            AZ_UNUSED(node);

            instance.GetGraphTypes().clear();

            QMenu* menu = GUI->menu();
            for (QAction* action : menu->actions())
            {
                if (action->isChecked())
                {
                    instance.GetGraphTypes().emplace_back(action->text().toUtf8().constData());
                }
            }
        }

        bool GraphTypeSelector::ReadValuesIntoGUI(size_t index, QPushButton* GUI, const property_t& instance,
            AzToolsFramework::InstanceDataNode* node)
        {
            AZ_UNUSED(index);
            AZ_UNUSED(node);

            QMenu* menu = GUI->menu();
            for (const auto& it : instance.GetGraphTypes())
            {
                for (QAction* action : menu->actions())
                {
                    if (AzFramework::StringFunc::Equal(action->text().toUtf8().constData(), it.GetName().c_str()))
                    {
                        action->setChecked(true);
                        break;
                    }
                }
            }
            return true;
        }

        void GraphTypeSelector::Register()
        {
            using namespace AzToolsFramework;

            if (!s_instance)
            {
                s_instance = aznew GraphTypeSelector();
                PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, s_instance);
            }
        }

        void GraphTypeSelector::Unregister()
        {
            using namespace AzToolsFramework;

            if (s_instance)
            {
                PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::UnregisterPropertyType, s_instance);
                delete s_instance;
                s_instance = nullptr;
            }
        }
    } // namespace SceneProcessingConfig
} // namespace AZ

#include <Source/Config/Widgets/moc_GraphTypeSelector.cpp>
