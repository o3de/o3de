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
#include "LmbrCentral_precompiled.h"
#include "CompoundShapeComponent.h"
#include "EditorCompoundShapeComponent.h"
#include <QMessageBox>

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

namespace LmbrCentral
{
    void EditorCompoundShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<EditorCompoundShapeComponent, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Configuration", &EditorCompoundShapeComponent::m_configuration)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<EditorCompoundShapeComponent>(
                    "Compound Shape", "The Compound Shape component allows two or more shapes to be combined to create more complex shapes")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Shape")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/Compound_Shape.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/Sphere.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-shapes.html")
                    ->DataElement(0, &EditorCompoundShapeComponent::m_configuration, "Configuration", "Compound Shape Configuration")
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorCompoundShapeComponent::ConfigurationChanged)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20))
                    ;
            }
        }
    }

    void EditorCompoundShapeComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        auto component = gameEntity->CreateComponent<CompoundShapeComponent>();
        if (component)
        {
            component->m_configuration = m_configuration;
        }
    }

    void EditorCompoundShapeComponent::Init()
    {
        // setup the contained runtime component so that it can manage the child entities in the editor.
        m_component.m_configuration = m_configuration;
        m_component.Init();
    }

    void EditorCompoundShapeComponent::Activate()
    {
        // before activation, remove any bad, circular references that would cause infinite loops
        ValidateConfiguration();

        /* As the compound shape doesn't have a shared impl (like the other shapes) that the runtime and editor components
           can use the editor component directly wraps a runtime component to manage the child entities.
           Launcher will load the runtime component instance created by BuildGameEntity.
         */
        m_component.m_configuration = m_configuration;
        m_component.SetEntity(GetEntity());
        m_component.Activate();
        CompoundShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        CompoundShapeComponentHierarchyRequestsBus::Handler::BusConnect(GetEntityId());

        EditorBaseShapeComponent::Activate();
    }

    void EditorCompoundShapeComponent::Deactivate()
    {
        EditorBaseShapeComponent::Deactivate();

        m_component.Deactivate();
        m_component.SetEntity(nullptr); // remove the entity association, in case the parent component is being removed, otherwise the component will be reactivated
        CompoundShapeComponentRequestsBus::Handler::BusDisconnect();
        CompoundShapeComponentHierarchyRequestsBus::Handler::BusDisconnect();
    }

    bool EditorCompoundShapeComponent::HasChildId(const AZ::EntityId& entityId)
    {
        for (const AZ::EntityId& childEntityId : m_configuration.GetChildEntities())
        {
            if (childEntityId.IsValid())
            {
                bool isCircularReference = childEntityId == entityId;
                if (!isCircularReference)
                {
                    CompoundShapeComponentHierarchyRequestsBus::EventResult(isCircularReference, childEntityId,
                        &CompoundShapeComponentHierarchyRequestsBus::Events::HasChildId, entityId);
                }

                if (isCircularReference)
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool EditorCompoundShapeComponent::ValidateChildIds()
    {
        bool isValid = true;

        AZStd::list<AZ::EntityId>& childEntities = const_cast<AZStd::list<AZ::EntityId>&>(m_configuration.GetChildEntities());
        for (AZ::EntityId& childEntityId : childEntities)
        {
            bool isCircularReference = childEntityId == GetEntityId();
            if (!isCircularReference)
            {
                CompoundShapeComponentHierarchyRequestsBus::EventResult(isCircularReference, childEntityId,
                    &CompoundShapeComponentHierarchyRequestsBus::Events::HasChildId, GetEntityId());
            }
            if (isCircularReference)
            {
                childEntityId = AZ::EntityId();
                isValid = false;
            }
        }

        return isValid;
    }

    bool EditorCompoundShapeComponent::ValidateConfiguration()
    {
        if (!ValidateChildIds())
        {
            QMessageBox(QMessageBox::Warning,
                "Endless Loop Warning",
                "Having circular references within a compound shape results in an endless loop!  Clearing the reference.",
                QMessageBox::Ok, AzToolsFramework::GetActiveWindow()).exec();
            SetDirty();
            return false;
        }
        return true;
    }

    AZ::u32 EditorCompoundShapeComponent::ConfigurationChanged()
    {
        AZ::u32 returnValue = AZ::Edit::PropertyRefreshLevels::None;

        if (!ValidateConfiguration())
        {
            returnValue = AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
        }

        m_component.Deactivate();
        m_component.m_configuration = m_configuration;
        m_component.Activate();

        ShapeComponentNotificationsBus::Event(GetEntityId(),
            &ShapeComponentNotificationsBus::Events::OnShapeChanged,
            ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

        return returnValue;
    }

} // namespace LmbrCentral
